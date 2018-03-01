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

protected:

	int16_t m_keydown_activity_counter;
	int16_t m_mouse_activity_counter;

	std::mutex m_input_mutex;

	std::list<uint16_t> m_keydown_virtual_keys;
	std::list<uint16_t> m_mouse_button_down;

	bool m_keyboard_active;
	bool m_mouse_active;

	uint64_t mouse_start_time;
	uint64_t keyboard_start_time;

};