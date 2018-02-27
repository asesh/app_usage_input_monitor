//
//

#include "stdafx.h"
#include "raw_input.h"

CRawInput::CRawInput()
{

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

	if (!::RegisterRawInputDevices(raw_input_devices, number_of_devices, sizeof(raw_input_devices[0])))
	{
		return false;
	}

	return true;
}

bool CRawInput::read_input_data(LPARAM lparam)
{
	UINT input_size = 0;

	static bool last_key_down = false;

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
				last_key_down = false;
				::OutputDebugString(L"\n\tKey is down");
			}
			else if (raw_input->data.keyboard.Flags == RI_KEY_BREAK) // Key is up
			{
				::OutputDebugString(L"\n\tKey is up");
			}
			break;

		case RIM_TYPEMOUSE: // So we have mouse data
			if (raw_input->data.mouse.usFlags == MOUSE_ATTRIBUTES_CHANGED)
			{
				::OutputDebugString(L"\n\tMouse attributes changed");
			}
			if (raw_input->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
			{
				::OutputDebugString(L"\n\tMouse movement data is relative");
			}
			if (raw_input->data.mouse.usFlags == MOUSE_MOVE_ABSOLUTE)
			{
				::OutputDebugString(L"\n\tMouse movement data is absolute");
			}
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
			//::OutputDebugString(L"\n\tMouse activity detected");
			break;
		}
	}

	return true;
}