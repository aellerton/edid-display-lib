#define WIN32_LEAN_AND_MEAN

#include <windows.h>
//#include <iostream>
#include <atltypes.h>
#include "shared/edid.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct GlobalState
{
    DisplayGroup displays;
    DisplayInfo this_display;
    bool need_refresh;

    GlobalState()
        : need_refresh(true)
    {}

    DisplayEnquiryCode refresh(HWND hwnd)
    {
        DisplayEnquiryCode r = get_all_displays(displays);
        get_display_for_hwnd(hwnd, this_display);
        need_refresh = false; // TODO should check result
        return r;
    }
};

GlobalState central;

int wmain()
{
    wWinMain(nullptr, nullptr, nullptr, 0);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	WNDCLASSEX wc = { 0 };
	wc.hInstance = hInstance;
	wc.lpszClassName = L"DisplayInfoMain";
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	auto regRes = RegisterClassEx(&wc);
	if (!regRes)
	{
		//std::cerr << "window registration failed" << std::endl;
		return regRes;
	}
	auto hwnd = CreateWindowEx(0, wc.lpszClassName, L"Display Info",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);
	if (hwnd == nullptr)
	{
		//std::cerr << "window couldn't be created" << std::endl;
		return -1;
	}

	ShowWindow(hwnd, SW_SHOWNORMAL);

	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

LRESULT HandleDestroy(HWND hWnd)
{
	PostQuitMessage(0);
	return 0;
}

LRESULT HandlePaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	auto hdc = BeginPaint(hwnd, &ps);
	FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW));
    CRect textRect(ps.rcPaint);
    CString message = L"Display info";
    //int height = DrawText(hdc, message.GetString(), message.GetLength(), &textRect, DT_LEFT | DT_TOP);

    if (central.need_refresh)
    {
        central.refresh(hwnd);
    }
    int offset = (int)central.this_display.px_for_mm(2);
    int px_per_in = (int)central.this_display.px_for_inch(1.0);
    int px_per_cm = (int)central.this_display.px_for_mm(10.0);
    CRect inch(0, 0, px_per_in, px_per_in);
    inch.MoveToXY(offset, offset);
    message.Format(_T("1 inch = %d px"), px_per_in);
    Rectangle(hdc, inch.left, inch.top, inch.right, inch.bottom);
    DrawText(hdc, message.GetString(), message.GetLength(), &inch, DT_CENTER | DT_VCENTER);

    CRect cm(0, 0, px_per_cm, px_per_cm);
    cm.MoveToXY(offset, offset*2 + inch.Height());
    message.Format(_T("1 cm = %d px"), px_per_cm);
    Rectangle(hdc, cm.left, cm.top, cm.right, cm.bottom);
    DrawText(hdc, message.GetString(), message.GetLength(), &cm, DT_LEFT | DT_VCENTER);



	EndPaint(hwnd, &ps);
	return 0;
}

void HandleNeedRefresh(HWND hwnd)
{
    central.need_refresh = true;
    InvalidateRect(hwnd, NULL, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return 1;
	case WM_DESTROY:
		return HandleDestroy(hwnd);
	case WM_PAINT:
		return HandlePaint(hwnd);
    case WM_DISPLAYCHANGE:
        HandleNeedRefresh(hwnd);
        break; // fall through
    case WM_WINDOWPOSCHANGED:
        HandleNeedRefresh(hwnd);
        return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}