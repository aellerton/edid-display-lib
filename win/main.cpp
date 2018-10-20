#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <atltypes.h>
#include "shared/edid.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct GlobalState
{
    DisplayGroup displays;
    DisplayInfo this_display;
    bool need_refresh;
    int refresh_count;
    HMONITOR most_recent_hmon;
    DisplayRect win_pos;
    HFONT font = nullptr;

    GlobalState()
        : need_refresh(true)
        , refresh_count(0)
        , most_recent_hmon(NULL)
    {}

    DisplayEnquiryCode refresh(HWND hwnd)
    {
        DisplayEnquiryCode r = get_all_displays(displays);
        get_display_for_hwnd(hwnd, this_display);
        need_refresh = false; // TODO should check result
        refresh_count++;
        return r;
    }

    void invalidate(HWND hwnd)
    {
        need_refresh = true;
        InvalidateRect(hwnd, NULL, FALSE);
    }

    void checkMonitorChange(HWND hwnd)
    {
        HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
        if (hMonitor != most_recent_hmon)
        {
            most_recent_hmon = hMonitor;
            invalidate(hwnd);
        }
    }
};

CRect scale_rect(double scale, int left, int top, const DisplayRect & src, const DisplayRect & full)
{
    CPoint pt(
        left + static_cast<int>((src.left - full.left) * scale),
        top + static_cast<int>((src.top - full.top) * scale)
    );
    //pt.Offset(mini_display_top_left);
    CSize sz(
        static_cast<int>(src.width()*scale),
        static_cast<int>(src.height()*scale)
    );

    return CRect(pt, sz);
}

HFONT get_system_font()
{
    // Get the system message box font
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);

/*
    // If we're compiling with the Vista SDK or later, the NONCLIENTMETRICS struct
    // will be the wrong size for previous versions, so we need to adjust it.
#if(_MSC_VER >= 1500 && WINVER >= 0x0600)
    if (!SystemInfo::IsVistaOrLater())
    {
        // In versions of Windows prior to Vista, the iPaddedBorderWidth member
        // is not present, so we need to subtract its size from cbSize.
        ncm.cbSize -= sizeof(ncm.iPaddedBorderWidth);
    }
#endif
*/
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
    HFONT hDlgFont = CreateFontIndirect(&(ncm.lfMessageFont));

    // Set the dialog to use the system message box font
    //SetFont(m_DlgFont, TRUE);
    //SendMessage(hWnd, WM_SETFONT, (WPARAM)hDlgFont, MAKELPARAM(FALSE, 0));
    return hDlgFont; // not going to free it. Hopefully that's ok.
}
GlobalState central;

int wmain()
{
    wWinMain(nullptr, nullptr, nullptr, 0);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    CPoint pt(CW_USEDEFAULT, CW_USEDEFAULT);
    CPoint sz(CW_USEDEFAULT, CW_USEDEFAULT);
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

    // Window size... ought to make this smarter
    sz.x = 800;
    sz.y = 300;

	auto hwnd = CreateWindowEx(0, wc.lpszClassName, L"Display Info", WS_OVERLAPPEDWINDOW, 
        pt.x, pt.y, sz.x, sz.y,
        nullptr, nullptr, hInstance, nullptr);
	if (hwnd == nullptr)
	{
		//std::cerr << "window couldn't be created" << std::endl;
		return -1;
	}

    //SetThreadDpiAwarenessContext(
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
    HFONT origFont = (HFONT)SelectObject(hdc, central.font); // TODO: should check result
	FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW));
    CRect textRect(ps.rcPaint);
    CString message = L"Display info";
    int height, left, top;
    //int height = DrawText(hdc, message.GetString(), message.GetLength(), &textRect, DT_LEFT | DT_TOP);

    if (central.need_refresh)
    {
        central.refresh(hwnd);
    }
    int pixelInset = -3; // number of pixels to inset text
    int offset = (int)central.this_display.px_for_mm(2); // pixels, but calculated from mm
    int px_per_in = (int)central.this_display.px_for_inch(1.0);
    int px_per_cm = (int)central.this_display.px_for_mm(10.0);
    CRect inch(0, 0, px_per_in, px_per_in);
    inch.MoveToXY(offset, offset);
    Rectangle(hdc, inch.left, inch.top, inch.right, inch.bottom);

    CRect cm(0, 0, px_per_cm, px_per_cm);
    cm.MoveToXY(offset, offset*2 + inch.Height());
    Rectangle(hdc, cm.left, cm.top, cm.right, cm.bottom);

    int right_of_measures = inch.right + offset;
    
    SetBkMode(hdc, TRANSPARENT);
    //SetTextColor(hdc, RGB(0, 255, 0));
    SetTextColor(hdc, RGB(80, 80, 80));

    //inch.InflateRect(1, 1);
    //cm.InflateRect(1, 1);
    message = "1 inch";
    //message.Format(_T("1 inch = %d px"), px_per_in);
    DrawText(hdc, message.GetString(), message.GetLength(), &inch, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    message = "1 cm"; // message.Format(_T("1 cm = %d px"), px_per_cm);
    height = DrawText(hdc, message.GetString(), message.GetLength(), &cm, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    textRect.SetRect(offset, cm.bottom + offset, ps.rcPaint.right - offset, ps.rcPaint.bottom);
    message.Format(_T(
        "Virtual desktop:\n"
        "  L: %d\n"
        "  T: %d\n"
        "  R: %d\n"
        "  B: %d\n"
        "  W: %d\n"
        "  H: %d\n"
        "  Refreshes: %d"),
        central.displays.virtual_px.left, central.displays.virtual_px.top,
        central.displays.virtual_px.right, central.displays.virtual_px.bottom,
        central.displays.virtual_px.width(), central.displays.virtual_px.height(),
        central.refresh_count
        );

    height = DrawText(hdc, message.GetString(), message.GetLength(), &textRect, DT_LEFT| DT_TOP);
    // for center multiline: http://forums.codeguru.com/showthread.php?253508-How-to-align-a-text-in-vertical-center

    top = offset;
    left = right_of_measures; // TODO: should be max(right_of_measures, right_of_text)

    // Scale displays to fit in window width
    int available_width = ps.rcPaint.right - offset - left;
    double scale = available_width / (double)central.displays.virtual_px.width();

    //CPoint mini_display_top_left(left, top);

    for (DisplayInfo & d : central.displays.displays)
    {
        /*
        CPoint pt(
            static_cast<int>((d.full_px.left - central.displays.virtual_px.left) * scale),
            static_cast<int>((d.full_px.top - central.displays.virtual_px.top) * scale)
        );
        pt.Offset(mini_display_top_left);
        CSize sz(
            static_cast<int>(d.full_px.width()*scale),
            static_cast<int>(d.full_px.height()*scale)
        );

        CRect mon_rect(pt, sz);*/
        CRect mon_rect = scale_rect(scale, left, top, d.full_px, central.displays.virtual_px);
        Rectangle(hdc, mon_rect.left, mon_rect.top, mon_rect.right, mon_rect.bottom);
        message.Format(_T(
                "Display %d\n"
                // Don't display name/device because the edid.cpp file is encoding it wrong.
                //"Name: %s\n"
                //"Device: %s\n"
                "Full LTRB: %d %d %d %d px\n"
                "Work LTRB: %d %d %d %d px\n"
                "Size: %d x %d px\n"
                "Notes: %s %s\n"
                "Density: %1.1f px per mm\n"
            ),
            d.seq,
            //d.name.c_str(),
            //d.device_name.c_str(),
            d.full_px.left, d.full_px.top, d.full_px.right, d.full_px.bottom,
            d.work_px.left, d.work_px.top, d.work_px.right, d.work_px.bottom,
            d.full_px.width(), d.full_px.height(),
            (d.is_primary ? _T("Primary") : _T("Non-primary")),
            (d.is_active ? _T("Active") : _T("Inactive")),
            d.px_per_mm
        );
        mon_rect.InflateRect(pixelInset, pixelInset);
        DrawText(hdc, message.GetString(), message.GetLength(), &mon_rect, DT_LEFT | DT_TOP);
        //left = mon_rect.right;
    }
    //SetDCBrushColor(hdc, RGB(150, 150, 250));
    HBRUSH b = CreateSolidBrush(RGB(150, 150, 250));
    //
    //CBrush b(CreateSolidBrush(COLOREF()
    //SelectObject(ps.hdc, GetStockObject(GRAY_BRUSH));
    CRect win_rect = scale_rect(scale, left, top, central.win_pos, central.displays.virtual_px);
    FillRect(hdc, &win_rect, b);
    //Rectangle(hdc, win_rect.left, win_rect.top, win_rect.right, win_rect.bottom);
    message.Format(_T("This window\nW: %d\nH: %d"), central.win_pos.width(), central.win_pos.height());
    win_rect.InflateRect(pixelInset, pixelInset);
    DrawText(hdc, message.GetString(), message.GetLength(), &win_rect, DT_LEFT | DT_TOP);
    
    SelectObject(hdc, origFont); // Restore the original font
    DeleteObject(b);
    EndPaint(hwnd, &ps);
    return 0;
}

void HandleWinMoved(HWND hwnd)
{
    CRect r;
    BOOL ok = GetWindowRect(hwnd, &r);
    if (ok == TRUE)
    {
        central.win_pos = DisplayRect(r);
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
    case WM_CREATE:
        // grab the initial size
        HandleWinMoved(hwnd);
        break; // fall through
	case WM_ERASEBKGND:
		return 1;
	case WM_DESTROY:
		return HandleDestroy(hwnd);
	case WM_PAINT:
		return HandlePaint(hwnd);
    case WM_DPICHANGED: 
        // not sure if this is different to other handlers
        // https://docs.microsoft.com/en-us/windows/desktop/hidpi/wm-dpichanged
        //HandleNeedRefresh(hwnd);
        central.invalidate(hwnd);
        break; // fall through
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE: // e.g. hide/show taskbar
        HandleWinMoved(hwnd); // this happens when primary monitor is changed
        central.invalidate(hwnd);
        break; // fall through
    case WM_WINDOWPOSCHANGED: 
        HandleWinMoved(hwnd); // flcker?
        central.checkMonitorChange(hwnd);
        //return 0;
        break; // fall through
    case WM_SYSCOMMAND: // for maximise.
    case WM_EXITSIZEMOVE: // only capture END of window move to avoid flicker.
        HandleWinMoved(hwnd);
        break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}