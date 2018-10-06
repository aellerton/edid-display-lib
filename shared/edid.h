#pragma once
#include <string>
#include <list>

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined (WIN32_LEAN_AND_MEAN)
#define IS_WINDOWS 1
#endif

#if IS_WINDOWS
#include <atlstr.h>
#include <SetupApi.h>
#include <cfgmgr32.h> // for MAX_DEVICE_ID_LEN
#pragma comment(lib, "setupapi.lib")
#endif

constexpr auto MM_PER_INCH = 25.4;

enum DisplayEnquiryCode
{
    DISPLAY_ENQUIRY_OK = 0,
    DISPLAY_ENQUIRY_DEVICE_NOT_FOUND,
    DISPLAY_ENQUIRY_SIZE_NOT_FOUND,
    DISPLAY_ENQUIRY_NO_MATCH
};

struct DisplayRect
{
    // NOT unsigned. Could be negative.
    int left, top, right, bottom;

    DisplayRect()
        : left(0)
        , top(0)
        , right(0)
        , bottom(0)
    {}

    DisplayRect(int left, int top, int right, int bottom)
        : left(left)
        , top(top)
        , right(right)
        , bottom(bottom)
    {}
#if IS_WINDOWS
    DisplayRect(const RECT &r)
        : left(r.left)
        , top(r.top)
        , right(r.right)
        , bottom(r.bottom)
    {}
#endif

    int width() const
    {
        return right - left;
    }

    int height() const
    {
        return bottom - top;
    }

    void clear()
    {
        left = top = right = bottom = 0;
    }
};

struct DisplaySize
{
    int width, height;
    DisplaySize()
        : width(0)
        , height(0)
    {}
    DisplaySize(int width, int height)
        : width(width)
        , height(height)
    {}
};

struct DisplayInfo
{
    short seq;
    std::string name;
    std::string device_name;
    std::string device_str;
    bool is_primary;
    bool is_active;
    DisplayRect full_px;
    DisplayRect work_px;
    DisplaySize physical_mm;
    double px_per_mm;
    double mm_per_px;

    DisplayInfo()
        : seq(0)
        , is_primary(false)
        , is_active(false)
        , px_per_mm(0.0)
        , mm_per_px(0.0)
    {}

    double px_for_mm(double mm_length)
    {
        return mm_length * px_per_mm;
    }

    double px_for_inch(double inch_length)
    {
        return inch_length * MM_PER_INCH * px_per_mm;
    }
};

struct DisplayGroup
{
    std::list<DisplayInfo> displays;
    DisplayRect virtual_px;

    DisplayGroup() {}
    void clear()
    {
        displays.clear();
        virtual_px.clear();
    }
};

DisplayEnquiryCode get_all_displays(DisplayGroup & group);

#if IS_WINDOWS
DisplayEnquiryCode get_display_for_hwnd(HWND, DisplayInfo & info);
#endif
