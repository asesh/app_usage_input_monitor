//
//

#pragma once

class CRawInput
{
public:
	CRawInput(); // Default constructor
	~CRawInput(); // Destructor

	bool init(HWND window_handle);

	bool read_input_data(LPARAM lparam);

	void on_app_switched();

	void reset_hardware_usage_time();

protected:

	int16_t m_keydown_counter;
	int16_t m_mouse_keys_down_counter;

	std::mutex m_input_mutex;

	std::list<uint16_t> m_keydown_virtual_keys;
	std::list<uint16_t> m_mouse_button_down;

	bool m_keyboard_active;
	bool m_mouse_active;

	uint64_t m_mouse_start_time, m_mouse_usage_accumulated_time;
	uint64_t m_keyboard_start_time, m_keyboard_usage_accumulated_time;
};