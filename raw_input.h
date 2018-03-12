//
//

#pragma once

#define INPUT_MONITOR_RESET_THRESHOLD	10 * 1000

#define MOUSE_CURSOR_MOVEMENT_MESSAGE	3000

class CFileWriter;

void CALLBACK queueable_timer_rountine(void *arguments, BYTE timer_or_wait_fired);

class CRawInput
{
public:
	CRawInput(); // Default constructor
	~CRawInput(); // Destructor

	bool init(HWND window_handle);

	bool read_input_data(LPARAM lparam);

	void on_mouse_activated(uint16_t button_flag);
	void on_mouse_deactivated(uint16_t button_flag);
	void on_mouse_wheel_scroll(uint16_t mouse_wheel_flag);
	void on_mouse_movement(uint16_t mouse_scroll_flag);
	void on_app_switched(std::wstring &switched_app_path);

	bool is_keyboard_activity_active() const;
	bool is_keyboard_activity_inactive() const;
	bool is_mouse_activity_active() const;
	bool is_mouse_activity_inactive() const;

	void reset_hardware_usage_time();

private:

	bool create_input_monitor_timer_queue();
	void destroy_input_monitor_timer_queue();

protected:

	int16_t m_key_down_counter;
	int16_t m_mouse_activity_counter;

	HANDLE	m_input_monitor_timer_queue;
	HANDLE	m_input_monitor_timer_queue_timer;

	std::mutex m_input_hardware_mutex;

	std::list<uint16_t> m_keydown_virtual_keys;
	std::list<uint16_t> m_mouse_activity;
	std::list<uint16_t> m_duplicate_mouse_activity;

	bool m_keyboard_active;
	bool m_mouse_active;

	uint64_t m_input_hardware_start_time, m_input_hardware_accumulated_time;

	std::list<uint64_t> m_accumulated_input_duration;
	uint64_t m_last_accumulated_time;

	CFileWriter m_file_writer;

	std::wstring m_recently_used_app_path;
};

extern CRawInput *g_raw_input;