#include "shared/edid.h"

int _tmain(int argc, _TCHAR *argv[]) {

    // Identify the HMONITOR of interest via the callback MonitorFoundCallback
    EnumDisplayMonitors(NULL, NULL, MonitorFoundCallback, NULL);
}
