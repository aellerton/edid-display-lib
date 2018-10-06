#include "shared/edid.h"

int _tmain(int argc, _TCHAR *argv[])
{

    // Identify the HMONITOR of interest via the callback MonitorFoundCallback
    //EnumDisplayMonitors(NULL, NULL, MonitorFoundCallback, NULL);

    DisplayGroup all_displays;
    DisplayEnquiryCode code = get_all_displays(all_displays);
    if (code != DISPLAY_ENQUIRY_OK)
    {
        printf("Failed with error %d\n", code);
        exit(code);
    }

    printf("Found %d display device%s\n", (int)all_displays.displays.size(), all_displays.displays.size() == 1 ? "" : "s");

    printf("\nVirtual desktop size:\n");
    printf("  LTRB: %d %d %d %d\n",
        all_displays.virtual_px.left, all_displays.virtual_px.top,
        all_displays.virtual_px.right, all_displays.virtual_px.bottom);
    printf("  W x H: %d x %d\n", all_displays.virtual_px.width(), all_displays.virtual_px.height());

    for (DisplayInfo & d : all_displays.displays) {
        printf("\nDisplay device:\n");
        printf("  seq: %d\n", d.seq);
        printf("  name: [%s]\n", d.name.c_str());
        printf("  device name: [%s]\n", d.device_name.c_str());
        printf("  device str: [%s]\n", d.device_str.c_str());
        printf("\n");
        printf("  status: %s\n", (d.is_primary ? "Primary" : "Not-primary"));
        printf("  is-active: %s\n", (d.is_active ? "true" : "false"));
        printf("\n");
        printf("  full rect (LTRB): %d %d %d %d\n", d.full_px.left, d.full_px.top, d.full_px.right, d.full_px.bottom);
        printf("  work rect (LTRB): %d %d %d %d\n", d.work_px.left, d.work_px.top, d.work_px.right, d.work_px.bottom);
        printf("  physical width x height: %dmm  x  %dmm\n", d.physical_mm.width, d.physical_mm.height);
        printf("\n");
        printf("  density %1.1f px per mm\n", d.px_per_mm);
        printf("  density %1.3f mm per px\n", d.mm_per_px);
        printf("  1.0cm on this screen is %1.1f px\n", d.px_for_mm(10.0));
        printf("  1.0in on this screen is %1.1f px\n", d.px_for_inch(1.0));
    }
}
