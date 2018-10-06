#include "edid.h"

// Credit: http://ofekshilon.com/2014/06/19/reading-specific-monitor-dimensions/

#define NAME_SIZE 128

const GUID GUID_CLASS_MONITOR = { 0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 };

// Callback called when EnumDisplayMonitors() finds a monitor to process
BOOL CALLBACK MonitorFoundCallback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM pData);

// Given a handle to a monitor, get a DISPLAY_DEVICE struct (which contains the DeviceID)
BOOL DisplayDeviceFromHMonitor(HMONITOR hMonitor, DISPLAY_DEVICE &ddMonOut);

// Take a device instance ID that points to a monitor and write its dimensions to arguments 2 and 3
bool GetSizeForDevID(const CString &TargetDevID, short &WidthMm, short &HeightMm);

// Parse EDID from given registry key to get width and height parameters, assumes hEDIDRegKey is valid
bool GetMonitorSizeFromEDID(const HKEY hEDIDRegKey, short &WidthMm, short &HeightMm);

DisplayEnquiryCode DisplayFromHMonitor(HMONITOR hMonitor, DisplayInfo & info);

// should probably use STL:
// Get the second slash block, used to match DeviceIDs to instances, example:
// DeviceID: MONITOR\GSM4B85\{4d36e96e-e325-11ce-bfc1-08002be10318}\0011
// Instance: DISPLAY\GSM4B85\5&273756F2&0&UID1048833
CString Get2ndSlashBlock(const CString &sIn)
{
    int FirstSlash = sIn.Find(_T('\\'));
    CString sOut = sIn.Right(sIn.GetLength() - FirstSlash - 1);
    FirstSlash = sOut.Find(_T('\\'));
    sOut = sOut.Left(FirstSlash);
    return sOut;
}

struct WinEphemeralResult
{
    DisplayEnquiryCode code;
    DisplayGroup * pGroup;
    //DisplayInfo * pInfo;
    RECT rcCombined;

    WinEphemeralResult()
        : code(DISPLAY_ENQUIRY_OK)
        , pGroup(0)
        //, pInfo(0)
    {
        SetRectEmpty(&rcCombined);
    }

    WinEphemeralResult(DisplayGroup & group)
        : code(DISPLAY_ENQUIRY_OK)
        , pGroup(&group)
        //, pInfo(0)
    {
        SetRectEmpty(&rcCombined);
    }
};

DisplayEnquiryCode get_all_displays(DisplayGroup & group)
{
    // with thanks to https://stackoverflow.com/a/18112853
    WinEphemeralResult result(group);    
    EnumDisplayMonitors(0, 0, MonitorFoundCallback, (LPARAM)&result);
    group.virtual_px = DisplayRect(result.rcCombined);
    return result.code;
}

DisplayEnquiryCode get_display_for_hwnd(HWND hwnd, DisplayInfo & info)
{
    WinEphemeralResult result;
    //result.pInfo = &info;
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (NULL == hMonitor)
    {
        return DISPLAY_ENQUIRY_NO_MATCH;
    }
    return DisplayFromHMonitor(hMonitor, info);
    //return display_from_hmonitor(hMon);
}

BOOL CALLBACK MonitorFoundCallback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM pData)
{
    WinEphemeralResult *pResult = reinterpret_cast<WinEphemeralResult*>(pData);

    // Update the full virtual screen space
    UnionRect(&pResult->rcCombined, &pResult->rcCombined, lprcMonitor);

    DisplayInfo display;
    DisplayEnquiryCode code = DisplayFromHMonitor(hMonitor, display);
    if (code != DISPLAY_ENQUIRY_OK)
    {
        pResult->code = code;
        return TRUE; // or should this be false?
    }

    if (pResult->pGroup)
    {
        // caller provided a group, so append this new display
        display.seq = static_cast<short>(pResult->pGroup->displays.size());
        pResult->pGroup->displays.push_back(display);
    }
    return TRUE;
}

DisplayEnquiryCode DisplayFromHMonitor(HMONITOR hMonitor, DisplayInfo & info)
{
    MONITORINFOEX mi;
    mi.cbSize = sizeof( MONITORINFOEX );
    GetMonitorInfo( hMonitor, &mi );

    DISPLAY_DEVICE ddMon;
    if (DisplayDeviceFromHMonitor(hMonitor, ddMon) == FALSE) {
        return DISPLAY_ENQUIRY_DEVICE_NOT_FOUND;
    }

    CString DeviceID;
    DeviceID.Format(_T("%s"), ddMon.DeviceID);
    DeviceID = Get2ndSlashBlock(DeviceID);

    short WidthMm = -1, HeightMm = -1;
    bool bFoundDevice = GetSizeForDevID(DeviceID, WidthMm, HeightMm);
    if (!bFoundDevice)
    {
        return DISPLAY_ENQUIRY_SIZE_NOT_FOUND;
    }

    CT2A deviceName(mi.szDevice);
    CT2A deviceId(ddMon.DeviceID);
    CT2A deviceStr(ddMon.DeviceString);

    info.seq = 0; // fill in later
    
    info.name = deviceName; // TODO: is this ok or need conversion or CT2A?
    info.device_name = deviceId;
    info.device_str = deviceStr;
    info.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) > 0 ? true : false;
    info.is_active = (ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE) > 0 ? true : false;
    info.full_px = DisplayRect(mi.rcMonitor);
    info.work_px = DisplayRect(mi.rcWork);
    info.physical_mm = DisplaySize(WidthMm, HeightMm);
    info.px_per_mm = (mi.rcMonitor.right - mi.rcMonitor.left) / (double)WidthMm;
    info.mm_per_px = WidthMm / (float)(mi.rcMonitor.right - mi.rcMonitor.left);
    return DISPLAY_ENQUIRY_OK;
}

BOOL DisplayDeviceFromHMonitor( HMONITOR hMonitor, DISPLAY_DEVICE &ddMonOut ) {

    MONITORINFOEX mi;
    mi.cbSize = sizeof( MONITORINFOEX );
    GetMonitorInfo( hMonitor, &mi );
    
    DISPLAY_DEVICE dd;
    dd.cb = sizeof( dd );
    DWORD devIdx = 0; // device index
    
    CString DeviceID;
    bool bFoundDevice = false;
    
    while( EnumDisplayDevices( 0, devIdx, &dd, 0 ) ) {
    
        devIdx++;
        
        if( 0 != _tcscmp( dd.DeviceName, mi.szDevice ) ) {
            continue;
        }
        
        DISPLAY_DEVICE ddMon;
        ZeroMemory( &ddMon, sizeof( ddMon ) );
        ddMon.cb = sizeof( ddMon );
        DWORD MonIdx = 0;
        
        while( EnumDisplayDevices( dd.DeviceName, MonIdx, &ddMon, 0 ) ) {
        
            MonIdx++;
            
            ddMonOut = ddMon;
            return TRUE;
            
            ZeroMemory( &ddMon, sizeof( ddMon ) );
            ddMon.cb = sizeof( ddMon );
        }
        
        ZeroMemory( &dd, sizeof( dd ) );
        dd.cb = sizeof( dd );
    }
    return FALSE;
}


bool GetSizeForDevID( const CString &TargetDevID, short &WidthMm, short &HeightMm ) {
    HDEVINFO devInfo = SetupDiGetClassDevsEx(
                           &GUID_CLASS_MONITOR,             // Class GUID
                           NULL,                            // Enumerator
                           NULL,                            // HWND
                           DIGCF_PRESENT | DIGCF_PROFILE,   // Flags //DIGCF_ALLCLASSES|
                           NULL,                            // Device info, create a new one.
                           NULL,                            // Machine name, local machine
                           NULL );                          // Reserved
                           
    if( NULL == devInfo ) {
        return false;
    }
    
    bool bRes = false;
    
    for( ULONG i = 0; ERROR_NO_MORE_ITEMS != GetLastError(); ++i ) {
    
        SP_DEVINFO_DATA devInfoData;
        memset( &devInfoData, 0, sizeof( devInfoData ) );
        devInfoData.cbSize = sizeof( devInfoData );
        
        if( SetupDiEnumDeviceInfo( devInfo, i, &devInfoData ) ) {
        
            TCHAR Instance[MAX_DEVICE_ID_LEN];
            SetupDiGetDeviceInstanceId( devInfo, &devInfoData, Instance, MAX_PATH, NULL );
            
            CString sInstance( Instance );
            
            if( -1 == sInstance.Find( TargetDevID ) ) {
                continue;
            }
            
            HKEY hEDIDRegKey = SetupDiOpenDevRegKey( devInfo, &devInfoData,
                               DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ );
                               
            if( !hEDIDRegKey || ( hEDIDRegKey == INVALID_HANDLE_VALUE ) ) {
                continue;
            }
            
            bRes = GetMonitorSizeFromEDID( hEDIDRegKey, WidthMm, HeightMm );
            
            RegCloseKey( hEDIDRegKey );
            
        }
        
    }
    
    SetupDiDestroyDeviceInfoList( devInfo );
    return bRes;
}

bool GetMonitorSizeFromEDID( const HKEY hEDIDRegKey, short &WidthMm, short &HeightMm ) {

    DWORD dwType, AcutalValueNameLength = NAME_SIZE;
    TCHAR valueName[NAME_SIZE];
    
    BYTE EDIDdata[1024];
    DWORD edidsize = sizeof( EDIDdata );
    
    for( LONG i = 0, retValue = ERROR_SUCCESS; retValue != ERROR_NO_MORE_ITEMS; ++i ) {
        retValue = RegEnumValue( hEDIDRegKey, i, &valueName[0],
                                 &AcutalValueNameLength, NULL, &dwType,
                                 EDIDdata, // buffer
                                 &edidsize ); // buffer size
                                 
        if( retValue != ERROR_SUCCESS || 0 != _tcscmp( valueName, _T( "EDID" ) ) ) {
            continue;
        }
        
        WidthMm = ( ( EDIDdata[68] & 0xF0 ) << 4 ) + EDIDdata[66];
        HeightMm = ( ( EDIDdata[68] & 0x0F ) << 8 ) + EDIDdata[67];
        
        return true; // valid EDID found
    }
    return false; // EDID not found
}
