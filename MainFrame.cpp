//
//

#include "stdafx.h"
#include "file_writer.h"
#include "raw_input.h"

// Switched to a new app
bool on_app_switched(HWND window_handle)
{
	wchar_t image_path[MAX_PATH * 2] = { 0 };
	DWORD buffer_size = sizeof(image_path) / sizeof(wchar_t);

	static std::wstring old_app_path;
	std::wstring current_app_path;

	DWORD process_id = 0;
	::GetWindowThreadProcessId(window_handle, &process_id);
	if (process_id)
	{
		HANDLE process_handle = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
		if (process_handle)
		{
			// Get the buffer size required to hold full process image name
			// There's no need to check if it managed to get the absolute path of app as we have already zeroed out the buffer
			::QueryFullProcessImageName(process_handle, 0, image_path, &buffer_size);
			current_app_path = image_path;
			::OutputDebugString(std::wstring(L"\n\t\t***Switched to a new app: " + std::wstring(image_path)).data());

			::CloseHandle(process_handle);
		}
		g_raw_input->on_app_switched(current_app_path);

		return true;
	}
	return false;
}

void CALLBACK win_event_callback(
	HWINEVENTHOOK win_event_hook,
	DWORD window_event,
	HWND window_handle,
	LONG id_object,
	LONG id_child,
	DWORD event_thread,
	DWORD event_time)
{
	switch (window_event)
	{
	case EVENT_SYSTEM_FOREGROUND:
		on_app_switched(window_handle);
		break;
	}
}

LRESULT CALLBACK window_proc(HWND window_handle, UINT window_message, WPARAM wparam, LPARAM lparam)
{
	switch (window_message)
	{
	case WM_PAINT:
		PAINTSTRUCT paint_struct;
		::BeginPaint(window_handle, &paint_struct);
		::EndPaint(window_handle, &paint_struct);
		break;

	case WM_CLOSE:
		::DestroyWindow(window_handle);
		break;

	case WM_DESTROY:
		::PostQuitMessage(0);
		break;

	case WM_INPUT:
		g_raw_input->read_input_data(lparam);
		break;

	default:
		return ::DefWindowProc(window_handle, window_message, wparam, lparam);
	}

	return 0;
}

int WINAPI wWinMain(HINSTANCE current_instance, HINSTANCE previous_instance, LPWSTR cmd, int show_command)
{
	wchar_t window_class_name[] = L"window_class_name";

	// Register the window class
	WNDCLASSEX window_class_ex = { 0 };
	window_class_ex.cbSize = sizeof(WNDCLASSEX);
	window_class_ex.lpfnWndProc = window_proc;
	window_class_ex.lpszClassName = window_class_name;
	window_class_ex.hInstance = current_instance;
	window_class_ex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
	if (!::RegisterClassEx(&window_class_ex))
	{
		return 1;
	}

	// Create an overlapped window
	HWND window_handle = ::CreateWindow(
		window_class_name,
		window_class_name,
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		200,
		200,
		nullptr,
		nullptr,
		current_instance,
		0);
	if (!window_handle)
	{
		::UnregisterClass(window_class_name, current_instance);
		return 1;
	}

	if (!g_raw_input->init(window_handle))
	{
		return 1;
	}

	auto window_event_hook = ::SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND,
		EVENT_SYSTEM_FOREGROUND,
		nullptr,
		win_event_callback,
		0,
		0,
		WINEVENT_OUTOFCONTEXT);

	::ShowWindow(window_handle, show_command);

	MSG message = { 0 };
	while (::GetMessage(&message, nullptr, 0, 0))
	{
		::DispatchMessage(&message);
	}

	::UnhookWinEvent(window_event_hook);

	return static_cast<int>(message.wParam);
}