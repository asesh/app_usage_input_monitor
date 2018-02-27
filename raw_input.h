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

protected:

	uint64_t mouse_start_time;
	uint64_t keyboard_start_time;

};