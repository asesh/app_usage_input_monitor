//
//

#include "stdafx.h"
#include "raw_input.h"

CRawInput::CRawInput()
{
	m_keydown_counter = 0;

	reset_hardware_usage_time();
}

CRawInput::~CRawInput()
{

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

	return true;
}

bool CRawInput::read_input_data(LPARAM lparam)
{
	UINT input_size = 0;

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
				std::lock_guard<std::mutex> input_mutex(m_input_mutex);

				// A user could have continuously pressed one or more keys so let's filter it out here
				if (std::find(m_keydown_virtual_keys.begin(), m_keydown_virtual_keys.end(), raw_input->data.keyboard.VKey) == m_keydown_virtual_keys.end())
				{
					// This key was not down before
					m_keydown_virtual_keys.push_back(raw_input->data.keyboard.VKey);
					m_keydown_counter++;
					std::wstring message;
					message.append(L"\n\tKey is down; **counter: " + std::to_wstring(m_keydown_counter));
					::OutputDebugString(message.data());

					// Initially if a key is down, let's assign the current time
					if (m_keydown_counter == 1)
					{
						m_keyboard_start_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
						return true;
					}
				}
			}
			else if (raw_input->data.keyboard.Flags == RI_KEY_BREAK) // Key is up
			{
				std::lock_guard<std::mutex> input_mutex(m_input_mutex);

				// If a key was previously down, let's check and remove it from the container
				if (std::find(m_keydown_virtual_keys.begin(), m_keydown_virtual_keys.end(), raw_input->data.keyboard.VKey) != m_keydown_virtual_keys.end())
				{
					// This key was down before so let's remove it from the container
					m_keydown_virtual_keys.remove(raw_input->data.keyboard.VKey);
					m_keydown_counter--;
					std::wstring message;
					message.append(L"\n\tKey is up; **counter: " + std::to_wstring(m_keydown_counter));
					::OutputDebugString(message.data());

					// Check if all the keys from the keyboard have been released
					if (m_keydown_counter == 0)
					{
						// This means all the pressed keys have been released
						m_keyboard_usage_accumulated_time += std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::high_resolution_clock().now().time_since_epoch()).count() - m_keyboard_start_time;

						m_keyboard_start_time = 0; // Reset keyboard start time

						std::wstring message;
						message.append(L"\n\t**None of the keys are down: **" + std::to_wstring(m_keyboard_usage_accumulated_time));
						::OutputDebugString(message.data());
					}
				}
			}
			break;

		case RIM_TYPEMOUSE: // So we have mouse data
			if (raw_input->data.mouse.usFlags == MOUSE_ATTRIBUTES_CHANGED)
			{
				::OutputDebugString(L"\n\tMouse attributes changed");
			}
			//if (raw_input->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
			//{
			//	::OutputDebugString(L"\n\tMouse movement data is relative");
			//}
			//if (raw_input->data.mouse.usFlags == MOUSE_MOVE_ABSOLUTE)
			//{
			//	::OutputDebugString(L"\n\tMouse movement data is absolute");
			//}
			switch (raw_input->data.mouse.usButtonFlags)
			{
			case RI_MOUSE_LEFT_BUTTON_DOWN:
				::OutputDebugString(L"\n\tLeft mouse button down");
				break;

			case RI_MOUSE_LEFT_BUTTON_UP:
				::OutputDebugString(L"\n\tLeft mouse button up");
				break;

			case RI_MOUSE_MIDDLE_BUTTON_DOWN:
				::OutputDebugString(L"\n\tMiddle mouse button down");
				break;

			case RI_MOUSE_MIDDLE_BUTTON_UP:
				::OutputDebugString(L"\n\tMiddle mouse button up");
				break;

			case RI_MOUSE_RIGHT_BUTTON_DOWN:
				::OutputDebugString(L"\n\tRight mouse button down");
				break;

			case RI_MOUSE_RIGHT_BUTTON_UP:
				::OutputDebugString(L"\n\tRight mouse button up");
				break;

			case RI_MOUSE_WHEEL:
			{
				std::wstring mouse_wheel_data;
				mouse_wheel_data  = L"\n\tMouse wheel: " + std::to_wstring(raw_input->data.mouse.usButtonData);
				::OutputDebugString(mouse_wheel_data.c_str());
				break;
			}
			}

			if (raw_input->data.mouse.lLastX)
			{
				std::wstring mouse_position_data;
				mouse_position_data = L"\n\tMouse X: " + std::to_wstring(raw_input->data.mouse.lLastX);
				::OutputDebugString(mouse_position_data.c_str());
			}
			if (raw_input->data.mouse.lLastY)
			{
				std::wstring mouse_position_data;
				mouse_position_data = L"\n\tMouse Y: " + std::to_wstring(raw_input->data.mouse.lLastY);
				::OutputDebugString(mouse_position_data.c_str());
			}
			break;
		}
	}

	return true;
}

void CRawInput::on_app_switched()
{
	std::lock_guard<std::mutex> input_mutex(m_input_mutex);

	// Assign the initial time
	m_mouse_start_time = m_keyboard_start_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock().now().time_since_epoch()).count();
}

void CRawInput::reset_hardware_usage_time()
{
	m_mouse_start_time = m_keyboard_start_time = m_keyboard_usage_accumulated_time = m_mouse_usage_accumulated_time = 0;
}