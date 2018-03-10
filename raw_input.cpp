//
//

#include "stdafx.h"
#include "raw_input.h"

#define CHRONO_TIME_SINCE_EPOCH_COUNT \
							std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock().now().time_since_epoch()).count()

CRawInput::CRawInput()
{
	m_key_down_counter = 0;

	m_last_accumulated_time = 0;

	m_input_hardware_start_time = m_input_hardware_accumulated_time = m_last_accumulated_time = 0;
}

CRawInput::~CRawInput()
{
	destroy_input_monitor_timer_queue();
}

bool CRawInput::init(HWND window_handle)
{
	// We will only be monitoring inputs from keyboard and mouse so let's register those devices
	constexpr int number_of_devices = 2;
	RAWINPUTDEVICE raw_input_devices[number_of_devices] = { 0 };

	// Keyboard
	raw_input_devices[0].usUsagePage = 0x1;
	raw_input_devices[0].usUsage = 0x2;
	raw_input_devices[0].dwFlags = RIDEV_INPUTSINK;
	raw_input_devices[0].hwndTarget = window_handle;

	// Mouse
	raw_input_devices[1].usUsagePage = 0x1;
	raw_input_devices[1].usUsage = 0x6;
	raw_input_devices[1].dwFlags = RIDEV_INPUTSINK;
	raw_input_devices[1].hwndTarget = window_handle;

	// Register keyboard and mouse devices
	if (!::RegisterRawInputDevices(raw_input_devices, number_of_devices, sizeof(raw_input_devices[0])))
	{
		return false;
	}

	create_input_monitor_timer_queue();

	return true;
}

bool CRawInput::read_input_data(LPARAM lparam)
{
	UINT input_size = 0;

	std::wstring message;

	// Get the size of raw input data
	::GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, nullptr, &input_size, sizeof(RAWINPUTHEADER));
	if (input_size == 0)
	{
		return false;
	}

	std::unique_ptr<byte> input_data(new byte[input_size]);

	if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, input_data.get(), &input_size, sizeof(RAWINPUTHEADER)) == input_size)
	{
		RAWINPUT *raw_input = reinterpret_cast<RAWINPUT *>(input_data.get());
		switch (raw_input->header.dwType)
		{
		case RIM_TYPEKEYBOARD: // So we have keyboard data
			if (raw_input->data.keyboard.Flags == RI_KEY_MAKE) // Key is down
			{
				std::lock_guard<std::mutex> input_mutex(m_input_hardware_mutex);

				// A user could have continuously pressed one or more keys so let's filter it out here
				if (std::find(m_keydown_virtual_keys.begin(), m_keydown_virtual_keys.end(), raw_input->data.keyboard.VKey) == m_keydown_virtual_keys.end())
				{
					// If no key is down, let's assign the current time
					if (is_keyboard_activity_inactive() && is_mouse_activity_inactive())
					{
						m_input_hardware_start_time = CHRONO_TIME_SINCE_EPOCH_COUNT;
					}

					// This key was not down before
					m_keydown_virtual_keys.push_back(raw_input->data.keyboard.VKey);
					m_key_down_counter++;

#ifdef _DEBUG
					message.append(L"\n\tKey is down; **counter: " + std::to_wstring(m_key_down_counter));
					::OutputDebugString(message.data());

#endif // _DEBUG
				}
			}
			else if (raw_input->data.keyboard.Flags == RI_KEY_BREAK) // Key is up
			{
				std::lock_guard<std::mutex> input_mutex(m_input_hardware_mutex);

				// If a key was previously down, let's check and remove it from the container
				if (std::find(m_keydown_virtual_keys.begin(), m_keydown_virtual_keys.end(), raw_input->data.keyboard.VKey) != m_keydown_virtual_keys.end())
				{
					// This key was down before so let's remove it from the container
					m_keydown_virtual_keys.remove(raw_input->data.keyboard.VKey);
					m_key_down_counter--;

#ifdef _DEBUG
					message.append(L"\n\tKey is up; **counter: " + std::to_wstring(m_key_down_counter));
					::OutputDebugString(message.data());
#endif // _DEBUG

					// Check if all the keys from the keyboard have been released
					if (is_keyboard_activity_inactive() && is_mouse_activity_inactive())
					{
						// This means all the pressed keys have been released
						m_input_hardware_accumulated_time += CHRONO_TIME_SINCE_EPOCH_COUNT - 
							(m_input_hardware_start_time == 0 ? CHRONO_TIME_SINCE_EPOCH_COUNT : m_input_hardware_start_time);
						m_last_accumulated_time = m_input_hardware_accumulated_time;

						m_input_hardware_start_time = 0; // Reset keyboard start time

#ifdef _DEBUG
						message.append(L"\n\t\t**None of the keys are down: **" + std::to_wstring(m_input_hardware_accumulated_time));
						::OutputDebugString(message.data());
#endif
					}
				}
			}
			break;

		case RIM_TYPEMOUSE: // So we have mouse data

			switch (raw_input->data.mouse.usButtonFlags)
			{
			case RI_MOUSE_LEFT_BUTTON_DOWN:
				on_mouse_activated(static_cast<uint16_t>(RI_MOUSE_LEFT_BUTTON_DOWN));
				break;

			case RI_MOUSE_LEFT_BUTTON_UP:
				on_mouse_deactivated(static_cast<uint16_t>(RI_MOUSE_LEFT_BUTTON_DOWN));
				break;

			case RI_MOUSE_MIDDLE_BUTTON_DOWN:
				on_mouse_activated(static_cast<uint16_t>(RI_MOUSE_MIDDLE_BUTTON_DOWN));
				break;

			case RI_MOUSE_MIDDLE_BUTTON_UP:
				on_mouse_deactivated(static_cast<uint16_t>(RI_MOUSE_MIDDLE_BUTTON_DOWN));
				break;

			case RI_MOUSE_RIGHT_BUTTON_DOWN:
				on_mouse_activated(static_cast<uint16_t>(RI_MOUSE_RIGHT_BUTTON_DOWN));
				break;

			case RI_MOUSE_RIGHT_BUTTON_UP:
				on_mouse_deactivated(static_cast<uint16_t>(RI_MOUSE_RIGHT_BUTTON_DOWN));
				break;

			case RI_MOUSE_WHEEL:
				on_mouse_wheel_scroll(static_cast<uint16_t>(RI_MOUSE_WHEEL));
				break;
			}

			if (raw_input->data.mouse.lLastX || raw_input->data.mouse.lLastY)
			{
				on_mouse_movement(MOUSE_CURSOR_MOVEMENT_MESSAGE);
			}
			break;
		}
	}

	return true;
}

void CRawInput::on_mouse_activated(uint16_t button_flag)
{
	std::lock_guard<std::mutex> mouse_mutex(m_input_hardware_mutex);

	// Check to prevent insertion of button flag more than once from mouse and touchpad
	if (std::find(m_mouse_activity.begin(), m_mouse_activity.end(), button_flag) == m_mouse_activity.end()) // This button hasn't been pressed
	{
		// Let's check if this is an initial mouse activity. If it is then we start tracking it's duration
		if (m_mouse_activity.empty())
		{
			m_input_hardware_start_time = CHRONO_TIME_SINCE_EPOCH_COUNT;
		}

		m_mouse_activity.push_back(button_flag);
	}
	else // This button was already being pressed before so it could have been pressed again from either touchpad or mouse
	{
		// Insert this duplicate mouse button down data in a different container
		m_duplicate_mouse_activity.push_back(button_flag);
	}

#ifdef _DEBUG
	switch (button_flag)
	{
	case RI_MOUSE_LEFT_BUTTON_DOWN:
		::OutputDebugString(L"\n\tLeft mouse button down");
		break;

	case RI_MOUSE_MIDDLE_BUTTON_DOWN:
		::OutputDebugString(L"\n\tMiddle mouse button down");
		break;

	case RI_MOUSE_RIGHT_BUTTON_DOWN:
		::OutputDebugString(L"\n\tRight mouse button down");
		break;
	}
#endif // _DEBUG
}

void CRawInput::on_mouse_deactivated(uint16_t button_flag)
{
	std::lock_guard<std::mutex> mouse_mutex(m_input_hardware_mutex);

	std::wstring mouse_activated_time;

	// Check if this button flag is already present in the container
	if (std::find(m_mouse_activity.begin(), m_mouse_activity.end(), button_flag) != m_mouse_activity.end()) // This button flag is present
	{
		// One or more buttons could have been pressed from both mouse and touchpad

		// There could have been same mouse buttons down: button click from mouse and another button click from touchpad. So a user could have released one
		// button while the other button is still down.
		if (std::find(m_duplicate_mouse_activity.begin(), m_duplicate_mouse_activity.end(), button_flag) != m_duplicate_mouse_activity.end()) // Duplicate data exists
		{
			m_duplicate_mouse_activity.remove(button_flag);
		}
		else // No duplicate button down exists for this mouse button
		{
			// Remove this button flag from the container as it has been released by the user
			m_mouse_activity.remove(button_flag);
		}

		// If the mouse activity container is empty then that means we should accumulate it's usage time
		if (is_mouse_activity_inactive() && is_keyboard_activity_inactive())
		{
			m_input_hardware_accumulated_time = CHRONO_TIME_SINCE_EPOCH_COUNT - 
				(m_input_hardware_start_time == 0 ? CHRONO_TIME_SINCE_EPOCH_COUNT : m_input_hardware_start_time);;
			m_last_accumulated_time = m_input_hardware_accumulated_time;

#ifdef _DEBUG
			mouse_activated_time.append(L"\n\t\t**Mouse activated time: " + std::to_wstring(m_input_hardware_accumulated_time));
			::OutputDebugString(mouse_activated_time.data());
#endif // _DEBUG
		}
	}

#ifdef _DEBUG
	switch (button_flag)
	{
	case RI_MOUSE_LEFT_BUTTON_DOWN:
		::OutputDebugString(L"\n\tLeft mouse button up");
		break;

	case RI_MOUSE_MIDDLE_BUTTON_DOWN:
		::OutputDebugString(L"\n\tMiddle mouse button up");
		break;

	case RI_MOUSE_RIGHT_BUTTON_DOWN:
		::OutputDebugString(L"\n\tRight mouse button up");
		break;
	}
#endif // _DEBUG
}

void CRawInput::on_mouse_wheel_scroll(uint16_t mouse_wheel_flag)
{
	std::lock_guard<std::mutex> mouse_mutex(m_input_hardware_mutex);

	// Let's check if mouse wheel scroll activity is already being tracked
	if (is_mouse_activity_active()) // Mouse wheel scroll activity is already being tracked
	{
		// Check if this mouse wheel wheel flag is already present in the container
		if (std::find(m_mouse_activity.begin(), m_mouse_activity.end(), mouse_wheel_flag) != m_mouse_activity.end()) // This mouse wheel flag is present
		{
			// This could mean that it's second mouse wheel message so we calculate the time elapsed from the first mouse wheel message
			// but only if keyboard activity is not present
			if (is_keyboard_activity_inactive()) // There's no keyboard activity
			{
				m_input_hardware_accumulated_time = CHRONO_TIME_SINCE_EPOCH_COUNT - m_input_hardware_start_time;
				m_last_accumulated_time = m_input_hardware_accumulated_time;

#ifdef _DEBUG
				::OutputDebugString(std::wstring(L"\n\t\t**Mouse wheel scroll ended: Accumulated time: " + std::to_wstring(m_input_hardware_accumulated_time)).data());
#endif // _DEBUG
			}

			m_mouse_activity.remove(mouse_wheel_flag);
		}
		else // This mouse activity data is not present in our container
		{
			// So, let's add it
			m_mouse_activity.push_back(mouse_wheel_flag);
		}
	}

	// Check if this mouse wheel flag is already present in the container
	else if (std::find(m_mouse_activity.begin(), m_mouse_activity.end(), mouse_wheel_flag) == m_mouse_activity.end()) // This mouse wheel flag isn't present
	{
		// We only assign initial time when both keyboard and mouse data are inactive
		if (is_keyboard_activity_inactive() && is_mouse_activity_inactive())
		{
			// Assign an initial time
			m_input_hardware_start_time = CHRONO_TIME_SINCE_EPOCH_COUNT;
		}
		
		// Either keyboard or mouse activity is already being tracked
		m_mouse_activity.push_back(mouse_wheel_flag);

#ifdef _DEBUG
		::OutputDebugString(L"\n\tMouse wheel scroll");
#endif // _DEBUG
	}
}

void CRawInput::on_mouse_movement(uint16_t mouse_movement_flag) 
{
	std::lock_guard<std::mutex> mouse_mutex(m_input_hardware_mutex);

	// Let's check if mouse activity is already being tracked
	if (is_mouse_activity_active()) // Mouse activity is already being tracked
	{
		// Check if this mouse movement flag is already present in the container
		if (std::find(m_mouse_activity.begin(), m_mouse_activity.end(), mouse_movement_flag) != m_mouse_activity.end()) // This mouse movement flag is present
		{
			// This could mean that it's second mouse movement message so we calculate the elapsed time from the first mouse movement 
			// but only if keyboard activity is not present
			if (is_keyboard_activity_inactive()) // There's no keyboard activity
			{
				m_input_hardware_accumulated_time = CHRONO_TIME_SINCE_EPOCH_COUNT - m_input_hardware_start_time;
				m_last_accumulated_time = m_input_hardware_accumulated_time;

#ifdef _DEBUG
				::OutputDebugString(std::wstring(L"\n\t\t**Mouse movement ended: Accumulated time: " + std::to_wstring(m_input_hardware_accumulated_time)).data());
#endif // _DEBUG
			}

			m_mouse_activity.remove(mouse_movement_flag);
		}
		else // This mouse activity data is not present in our container
		{
			// So, let's add it
			m_mouse_activity.push_back(mouse_movement_flag);
		}
	}

	// Check if this mouse movement flag is already present in the container
	else if (std::find(m_mouse_activity.begin(), m_mouse_activity.end(), mouse_movement_flag) == m_mouse_activity.end()) // This mouse movement isn't present
	{
		// We only assign initial time when both keyboard and mouse data are inactive
		if (is_keyboard_activity_inactive() && is_mouse_activity_inactive())
		{
			// Assign an initial time
			m_input_hardware_start_time = CHRONO_TIME_SINCE_EPOCH_COUNT;
		}

		// Either keyboard or mouse activity is already being tracked
		m_mouse_activity.push_back(mouse_movement_flag);

#ifdef _DEBUG
		::OutputDebugString(L"\n\tMouse movement");
#endif // _DEBUG
	}
}

void CRawInput::on_app_switched()
{
	std::lock_guard<std::mutex> input_mutex(m_input_hardware_mutex);

	// Assign an initial time
	m_input_hardware_start_time = CHRONO_TIME_SINCE_EPOCH_COUNT;
}

bool CRawInput::is_keyboard_activity_active() const
{
	return m_key_down_counter != 0;
}

bool CRawInput::is_keyboard_activity_inactive() const
{
	return m_key_down_counter == 0;
}

bool CRawInput::is_mouse_activity_active() const
{
	return !m_mouse_activity.empty();
}

bool CRawInput::is_mouse_activity_inactive() const
{
	return m_mouse_activity.empty();
}

void CRawInput::reset_hardware_usage_time()
{
	std::lock_guard<std::mutex> hardware_usage_mutex(m_input_hardware_mutex);

#ifdef _DEBUG
	::OutputDebugString(std::wstring(L"\n\t\t\t**Timer fired. Total duration: " + std::to_wstring(m_last_accumulated_time)).data());
#endif // _DEBUG

	m_input_hardware_start_time = m_input_hardware_accumulated_time = m_last_accumulated_time = 0;
}

bool CRawInput::create_input_monitor_timer_queue()
{
	// Create timer queue
	m_input_monitor_timer_queue = ::CreateTimerQueue();
	if (!::CreateTimerQueueTimer(
		&m_input_monitor_timer_queue_timer,
		m_input_monitor_timer_queue,
		queueable_timer_rountine,
		nullptr,
		INPUT_MONITOR_RESET_THRESHOLD,
		INPUT_MONITOR_RESET_THRESHOLD,
		0))
	{
		return false;
	}

	return true;
}

void CRawInput::destroy_input_monitor_timer_queue()
{
	// Delete timer queue
	if (m_input_monitor_timer_queue)
	{
		::DeleteTimerQueue(m_input_monitor_timer_queue);
		m_input_monitor_timer_queue = nullptr;
		m_input_monitor_timer_queue_timer = nullptr;
	}
}

void CALLBACK queueable_timer_rountine(void *arguments, BYTE timer_or_wait_fired)
{
	g_raw_input->reset_hardware_usage_time();
}

CRawInput raw_input;
CRawInput *g_raw_input = &raw_input;