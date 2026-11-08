#include "IconDrawer.h"

#include <Windows.h>

// 窗口类名。注册和创建窗口时用同一个名字。
constexpr const wchar_t* kWindowClass = L"CoreRefinerIconDrawer";

// 处理这个空窗口的消息。关掉窗口时退出程序。
// hwnd 是窗口。msg 是消息编号。wParam、lParam 是这条消息自带的参数。
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// 程序入口。弹出空窗口，直到用户关掉它。
// instance 是本程序实例，用来注册窗口类。show 是启动时窗口怎么显示。
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
	// 画布规则不对就不出窗口。通过时不影响后面的空窗口。
	if (!IconSheet::SelfCheck())
	{
		return -1;
	}

	// wc 描述窗口类：消息函数、实例、类名、光标和背景。
	WNDCLASSW wc{};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = instance;
	wc.lpszClassName = kWindowClass;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	if (RegisterClassW(&wc) == 0)
	{
		return -1;
	}

	// hwnd 是刚创建的空窗口。标题固定为 IconDrawer。
	HWND hwnd = CreateWindowExW(
		0,
		kWindowClass,
		L"IconDrawer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		960,
		640,
		nullptr,
		nullptr,
		instance,
		nullptr);
	if (hwnd == nullptr)
	{
		return -1;
	}

	ShowWindow(hwnd, show);
	UpdateWindow(hwnd);

	// message 是当前取出的一条窗口消息。
	MSG message{};
	while (GetMessageW(&message, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	return static_cast<int>(message.wParam);
}
