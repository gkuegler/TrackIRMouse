#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>

#include <Windows.h>

#include "Constants.h"
#include "Display.h"
#include "Config.h"
#include "toml.hpp"

#define USHORT_MAX_VAL 65535 // SendInput with absolute mouse movement takes a short int

// Set up an array to hold display objects.
// Each display object contains info to do cursor position logic.
// Shameful, I know, to use a global raw array,
// but I did not want to pass around pointers and
// do casting to and from lparams with windows api's
static CDisplay displays[DEFAULT_MAX_DISPLAYS];

// Global Variables
static signed int virt_origin_x = 0; // move to static class member
static signed int virt_origin_y = 0; // move to static class member
static float x_PxToABS; // move to static class member
static float y_PxToABS; // move to static class member

static MOUSEINPUT mi = {
    0,
    0,
    0,
    MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK,
    0,
    0
};

static INPUT ip = {
    INPUT_MOUSE,
    mi
};



// Make a Windows API call to get the virtual boundaries of each monitor.
// Assigned those values to parameters within each display object
// lparam not used
BOOL CALLBACK PopulateVirtMonitorBounds(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM)
{
    static int count{ 0 };
    MONITORINFOEX Monitor;
    Monitor.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &Monitor);

    displays[count].pix_left = static_cast <unsigned int> (Monitor.rcMonitor.left); // from long
    displays[count].pix_right = static_cast <unsigned int> (Monitor.rcMonitor.right); // from long
    displays[count].pix_top = static_cast <unsigned int> (Monitor.rcMonitor.top); // from long
    displays[count].pix_bottom = static_cast <unsigned int> (Monitor.rcMonitor.bottom); // from long

    printf("MON Name:      %ls\n", Monitor.szDevice);
    printf("MON %d Left:   %13d\n", count, displays[count].pix_left);
    printf("MON %d Right:  %13d\n", count, displays[count].pix_right);
    printf("MON %d Top:    %13d\n", count, displays[count].pix_top);
    printf("MON %d Bottom: %13d\n", count, displays[count].pix_bottom);

    if (Monitor.rcMonitor.left < virt_origin_x)
    {
        virt_origin_x = Monitor.rcMonitor.left;
    }
    if (Monitor.rcMonitor.top < virt_origin_y)
    {
        virt_origin_y = Monitor.rcMonitor.top;
    }

    count++;
    return true;
};

int WinSetup()
{

    printf("\n--------Windows Environment Info----------\n");

    int num_monitors = GetSystemMetrics(SM_CMONITORS);
    int VM_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);  // width of total bounds of all screens
    int VM_height = GetSystemMetrics(SM_CYVIRTUALSCREEN); // height of total bounds of all screens

    printf("%d Monitors Found\n", num_monitors);
    printf("Width of Virtual Desktop: %d\n", VM_width);
    printf("Height of Virtual Desktop: %d\n", VM_height);

    if (DEFAULT_MAX_DISPLAYS < num_monitors)
    {
        printf("More Than %d Displays Found.\nIncrease max number of displays.\n", DEFAULT_MAX_DISPLAYS);
    }

    DISPLAY_DEVICEA display_device_info;
    display_device_info.cb = sizeof(DISPLAY_DEVICEA);

    for (int i = 0; ; i++)
    {
        BOOL result = EnumDisplayDevicesA(
            0,
            i,
            &display_device_info,
            0
        );

        if (result)
        {
            std::cout << i << " Name: " << display_device_info.DeviceName << "  DeviceString: " << display_device_info.DeviceString << std::endl;
        }
        else
        {
            break;
        }
    }

    x_PxToABS = USHORT_MAX_VAL / static_cast<float>(VM_width);
    y_PxToABS = USHORT_MAX_VAL / static_cast<float>(VM_height);

    printf("\nVirtual Desktop Pixel Bounds\n");
    EnumDisplayMonitors(NULL, NULL, PopulateVirtMonitorBounds, reinterpret_cast<LPARAM>(&displays));

    printf("Virtual Origin Offset Horizontal: %5d\n", virt_origin_x);
    printf("Virtual Origin Offset Vertical:   %5d\n\n", virt_origin_y);

    return num_monitors;
}

void DisplaySetup(int num_monitors, CConfig& Config)
{
    for (int i = 0; i < num_monitors; i++)
    {
        displays[i].rot_left = Config.bounds[i].left;
        displays[i].rot_right = Config.bounds[i].right;
        displays[i].rot_top = Config.bounds[i].top;
        displays[i].rot_bottom = Config.bounds[i].bottom;

        displays[i].left_padding = Config.bounds[i].pad_left;
        displays[i].right_padding = Config.bounds[i].pad_right;
        displays[i].top_padding = Config.bounds[i].pad_top;
        displays[i].bottom_padding = Config.bounds[i].pad_bottom;

        displays[i].setAbsBounds(virt_origin_x, virt_origin_y, x_PxToABS, y_PxToABS);
    }
    printf("\nVirtual Desktop Pixel Bounds (abs)\n");
    for (int i = 0; i < num_monitors; i++)
    {
        printf("MON %d pix_abs_left:   %5d\n", i, displays[i].pix_abs_left);
        printf("MON %d pix_abs_right:  %5d\n", i, displays[i].pix_abs_right);
        printf("MON %d pix_abs_top:    %5d\n", i, displays[i].pix_abs_top);
        printf("MON %d pix_abs_bottom: %5d\n", i, displays[i].pix_abs_bottom);
    }
    printf("\n16-bit Coordinate Bounds\n");
    for (int i = 0; i < num_monitors; i++)
    {
        printf("MON %d abs_left:   %7.1f\n", i, displays[i].abs_left);
        printf("MON %d abs_right:  %7.1f\n", i, displays[i].abs_right);
        printf("MON %d abs_top:    %7.1f\n", i, displays[i].abs_top);
        printf("MON %d abs_bottom: %7.1f\n", i, displays[i].abs_bottom);
    }
    printf("\nRotational Bounds\n");
    for (int i = 0; i < num_monitors; i++)
    {
        printf("MON %d rot_left:   %7.2f\n", i, displays[i].rot_left);
        printf("MON %d rot_right:  %7.2f\n", i, displays[i].rot_right);
        printf("MON %d rot_top:    %7.2f\n", i, displays[i].rot_top);
        printf("MON %d rot_bottom: %7.2f\n", i, displays[i].rot_bottom);
    }

    printf("\n------------------------------------\n");

    return;
}

inline void SendMyInput(float x, float y)
{
    
    ip.mi.dx = static_cast<LONG>(x);
    ip.mi.dy = static_cast<LONG>(y);

    SendInput(1, &ip, sizeof(INPUT));

    return;
}

void MouseMove(int num_monitors, float yaw, float pitch) {

    // set the last screen initially equal to the main display for me
    // TODO: find a way to specify in settings or get from windows
    static int last_screen = 1;

    // variables used for linear interpolation
    static float rl;
    static float al;
    static float mx;
    static float x;
    static float rt;
    static float at;
    static float my;
    static float y;

    // Check if the head is pointing to a screen
    // The return statement is never reached if the head is pointing outside the bounds of any of the screens

    for (int i = 0; i <= num_monitors - 1; i++)
    {
        if ((yaw > displays[i].rot_s15bit_left) && (yaw < displays[i].rot_s15bit_right) && (pitch < displays[i].rot_s15bit_top) && (pitch > displays[i].rot_s15bit_bottom))
        {
            // I wrote it out for maintainability
            // its plenty fast anyway for a 60hz limited display
            rl = displays[i].rot_s15bit_left;
            al = displays[i].abs_left;
            mx = displays[i].xSlope;
            x = mx * (yaw - rl) + al;
            rt = displays[i].rot_s15bit_top;
            at = displays[i].abs_top;
            my = displays[i].ySlope;
            y = my * (rt - pitch) + at;
            // load the coordinates into my input structure
            // need to cast to an integer because resulting calcs are floats
            SendMyInput(x, y);
            last_screen = i;
            //printf("(%f, %f)", y, x); // for testing
            return;
        }
    }

    // If the head is pointing outside of the bounds of a screen the mouse should snap to the breached edge
    // It could either be the pitch or the yaw axis that is too great or too little
    // To do this assume the pointer came from the last screen, just asign the mouse position to the absolute limit from the screen it came from
    if (yaw < displays[last_screen].rot_s15bit_left)
    {
        x = displays[last_screen].abs_left + displays[last_screen].left_padding * x_PxToABS;
    }
    else if (yaw > displays[last_screen].rot_s15bit_right)
    {
        x = displays[last_screen].abs_right - displays[last_screen].right_padding * x_PxToABS;
    }
    else
    {
        rl = displays[last_screen].rot_s15bit_left;
        al = displays[last_screen].abs_left;
        mx = displays[last_screen].xSlope;
        x = mx * (yaw - rl) + al;
    }

    if (pitch > displays[last_screen].rot_s15bit_top)
    {
        y = displays[last_screen].abs_top + displays[last_screen].top_padding * y_PxToABS;
    }
    else if (pitch < displays[last_screen].rot_s15bit_bottom)
    {
        y = displays[last_screen].abs_bottom - displays[last_screen].bottom_padding * y_PxToABS;
    }
    else
    {
        rt = displays[last_screen].rot_s15bit_top;
        at = displays[last_screen].abs_top;
        my = displays[last_screen].ySlope;
        y = my * (rt - pitch) + at;
    }

    // printf("off monitors");
    SendMyInput(x, y);

    return;

}