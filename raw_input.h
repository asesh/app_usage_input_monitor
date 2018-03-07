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

	void on_mouse_activated(uint16_t button_flag);
	void on_mouse_deactivated(uint16_t button_flag);
	void on_app_switched();

	void reset_hardware_usage_time();

protected:

	int16_t m_keydown_counter;
	int16_t m_mouse_activity_counter;

	std::mutex m_keyboard_mutex, m_mouse_mutex;

	std::list<uint16_t> m_keydown_virtual_keys;
	std::list<uint16_t> m_mouse_activity;
	std::list<uint16_t> m_duplicate_mouse_activity;

	bool m_keyboard_active;
	bool m_mouse_active;

	uint64_t m_mouse_usage_start_time, m_mouse_usage_accumulated_time;
	uint64_t m_keyboard_usage_start_time, m_keyboard_usage_accumulated_time;
};