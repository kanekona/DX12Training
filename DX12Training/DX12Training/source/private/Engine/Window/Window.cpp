#include "Engine\Window\Window.h"

LRESULT Window::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}