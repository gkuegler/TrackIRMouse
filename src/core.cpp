#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>

#include <Windows.h>

#include "Constants.h"
#include "Display.h"
#include "Config.h"
#include "toml.hpp"
#include "Log.h"

#define FMT_HEADER_ONLY
#include <fmt\format.h>
#include <fmt\xchar.h>

#define USHORT_MAX_VAL 65535 // SendInput with absolute mouse movement takes a short int

// Set up an array to hold display objects.
// Each display object contains info to do cursor position logic.
// Shameful, I know, to use a global raw array,
// but I did not want to pass around pointers and
// do casting to and from lparams with windows api's
static CDisplay g_displays[DEFAULT_MAX_DISPLAYS];

// Global Variables
static signed int g_virt_origin_x = 0; // move to static class member
static signed int g_virt_origin_y = 0; // move to static class member
static float g_x_PxToABS; // move to static class member
static float g_y_PxToABS; // move to static class member

// Make a Windows API call to get the virtual boundaries of each monitor.
// Assigned those values to parameters within each display object
// lparam not used
BOOL CALLBACK PopulateVirtMonitorBounds(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM)
{
    static int count{ 0 };
    MONITORINFOEX Monitor;
    Monitor.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &Monitor);

    g_displays[count].pix_left = static_cast <unsigned int> (Monitor.rcMonitor.left); // from long
    g_displays[count].pix_right = static_cast <unsigned int> (Monitor.rcMonitor.right); // from long
    g_displays[count].pix_top = static_cast <unsigned int> (Monitor.rcMonitor.top); // from long
    g_displays[count].pix_bottom = static_cast <unsigned int> (Monitor.rcMonitor.bottom); // from long

    // Remember to not use {s} for string types. This causes an error for some reason.
    logToWix(fmt::format(L"MON Name:{:>15}\n", Monitor.szDevice));

    logToWix(fmt::format("MON {} Left:   {:>10}\n", count, g_displays[count].pix_left));
    logToWix(fmt::format("MON {} Right:  {:>10}\n", count, g_displays[count].pix_right));
    logToWix(fmt::format("MON {} Top:    {:>10}\n", count, g_displays[count].pix_top));
    logToWix(fmt::format("MON {} Bottom: {:>10}\n", count, g_displays[count].pix_bottom));

    if (Monitor.rcMonitor.left < g_virt_origin_x)
    {
        g_virt_origin_x = Monitor.rcMonitor.left;
    }
    if (Monitor.rcMonitor.top < g_virt_origin_y)
    {
        g_virt_origin_y = Monitor.rcMonitor.top;
    }

    count++;
    return true;
};

int WinSetup()
{
    logToWix(fmt::format("\n{:-^50}\n", "Windows Environment Info"));

    int num_monitors = GetSystemMetrics(SM_CMONITORS);
    int VM_width = GetSystemMetrics(SM_CXVIRTUALSCREEN);  // width of total bounds of all screens
    int VM_height = GetSystemMetrics(SM_CYVIRTUALSCREEN); // height of total bounds of all screens

    logToWix(fmt::format("{} Monitors Found\n", num_monitors));
    logToWix(fmt::format("Width of Virtual Desktop:  {:>5}\n", VM_width));
    logToWix(fmt::format("Height of Virtual Desktop: {:>5}\n", VM_height));

    if (DEFAULT_MAX_DISPLAYS < num_monitors)
    {
        logToWix(fmt::format("More Than {} Displays Found.\nIncrease max number of g_displays.\n", DEFAULT_MAX_DISPLAYS));
    }

    /* #################################################################
    For a future feature to automatically detect monitor configurations
    and select the correct profile.

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
            //std::cout << i << " Name: " << display_device_info.DeviceName << "  DeviceString: " << display_device_info.DeviceString << std::endl;
            logToWix(fmt::format("{} Name: {} DeviceString: {}\n", i, display_device_info.DeviceName, display_device_info.DeviceString));
        }
        else
        {
            break;
        }
    }
    ######################################################################## */

    g_x_PxToABS = USHORT_MAX_VAL / static_cast<float>(VM_width);
    g_y_PxToABS = USHORT_MAX_VAL / static_cast<float>(VM_height);

    logToWix(fmt::format("\nVirtual Desktop Pixel Bounds\n"));
    EnumDisplayMonitors(NULL, NULL, PopulateVirtMonitorBounds, reinterpret_cast<LPARAM>(&g_displays));

    logToWix(fmt::format("\nVirtual Origin Offset Horizontal: {:d}\n", g_virt_origin_x));
    logToWix(fmt::format("Virtual Origin Offset Vertical:   {:d}\n", g_virt_origin_y));

    return num_monitors;
}

void DisplaySetup(int num_monitors, CConfig* config)
{
    for (int i = 0; i < num_monitors; i++)
    {
        g_displays[i].rot_left = (config)->bounds[i].left;
        g_displays[i].rot_right = (config)->bounds[i].right;
        g_displays[i].rot_top = (config)->bounds[i].top;
        g_displays[i].rot_bottom = (config)->bounds[i].bottom;

        g_displays[i].left_padding = (config)->bounds[i].pad_left;
        g_displays[i].right_padding = (config)->bounds[i].pad_right;
        g_displays[i].top_padding = (config)->bounds[i].pad_top;
        g_displays[i].bottom_padding = (config)->bounds[i].pad_bottom;

        g_displays[i].setAbsBounds(g_virt_origin_x, g_virt_origin_y, g_x_PxToABS, g_y_PxToABS);
    }
    logToWix(fmt::format("\nVirtual Desktop Pixel Bounds (abs)\n"));
    for (int i = 0; i < num_monitors; i++)
    {
        logToWix(fmt::format("MON {} pix_abs_left:   {:>10d}\n", i, g_displays[i].pix_abs_left));
        logToWix(fmt::format("MON {} pix_abs_right:  {:>10d}\n", i, g_displays[i].pix_abs_right));
        logToWix(fmt::format("MON {} pix_abs_top:    {:>10d}\n", i, g_displays[i].pix_abs_top));
        logToWix(fmt::format("MON {} pix_abs_bottom: {:>10d}\n", i, g_displays[i].pix_abs_bottom));
    }
    logToWix(fmt::format("\n16-bit Coordinate Bounds\n"));
    for (int i = 0; i < num_monitors; i++)
    {
        logToWix(fmt::format("MON {} abs_left:       {:>12.1f}\n", i, g_displays[i].abs_left));
        logToWix(fmt::format("MON {} abs_right:      {:>12.1f}\n", i, g_displays[i].abs_right));
        logToWix(fmt::format("MON {} abs_top:        {:>12.1f}\n", i, g_displays[i].abs_top));
        logToWix(fmt::format("MON {} abs_bottom:     {:>12.1f}\n", i, g_displays[i].abs_bottom));
    }
    logToWix(fmt::format("\nRotational Bounds\n"));
    for (int i = 0; i < num_monitors; i++)
    {
        logToWix(fmt::format("MON {} rot_left:       {:>13.2f}\n", i, g_displays[i].rot_left));
        logToWix(fmt::format("MON {} rot_right:      {:>13.2f}\n", i, g_displays[i].rot_right));
        logToWix(fmt::format("MON {} rot_top:        {:>13.2f}\n", i, g_displays[i].rot_top));
        logToWix(fmt::format("MON {} rot_bottom:     {:>13.2f}\n", i, g_displays[i].rot_bottom));
    }

    logToWix(fmt::format("\n{:-^}\n", ""));

    return;
}

inline void SendMyInput(float x, float y)
{
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
        if ((yaw > g_displays[i].rot_s15bit_left) && (yaw < g_displays[i].rot_s15bit_right) && (pitch < g_displays[i].rot_s15bit_top) && (pitch > g_displays[i].rot_s15bit_bottom))
        {
            // I wrote it out for maintainability
            // its plenty fast anyway for a 60hz limited display
            rl = g_displays[i].rot_s15bit_left;
            al = g_displays[i].abs_left;
            mx = g_displays[i].xSlope;
            x = mx * (yaw - rl) + al;
            rt = g_displays[i].rot_s15bit_top;
            at = g_displays[i].abs_top;
            my = g_displays[i].ySlope;
            y = my * (rt - pitch) + at;
            // load the coordinates into my input structure
            // need to cast to an integer because resulting calcs are floats
            SendMyInput(x, y);
            last_screen = i;
            //logToWix(fmt::format("(%f, %f)", y, x); // for testing
            return;
        }
    }

    // If the head is pointing outside of the bounds of a screen the mouse should snap to the breached edge
    // It could either be the pitch or the yaw axis that is too great or too little
    // To do this assume the pointer came from the last screen, just asign the mouse position to the absolute limit from the screen it came from
    if (yaw < g_displays[last_screen].rot_s15bit_left)
    {
        x = g_displays[last_screen].abs_left + g_displays[last_screen].left_padding * g_x_PxToABS;
    }
    else if (yaw > g_displays[last_screen].rot_s15bit_right)
    {
        x = g_displays[last_screen].abs_right - g_displays[last_screen].right_padding * g_x_PxToABS;
    }
    else
    {
        rl = g_displays[last_screen].rot_s15bit_left;
        al = g_displays[last_screen].abs_left;
        mx = g_displays[last_screen].xSlope;
        x = mx * (yaw - rl) + al;
    }

    if (pitch > g_displays[last_screen].rot_s15bit_top)
    {
        y = g_displays[last_screen].abs_top + g_displays[last_screen].top_padding * g_y_PxToABS;
    }
    else if (pitch < g_displays[last_screen].rot_s15bit_bottom)
    {
        y = g_displays[last_screen].abs_bottom - g_displays[last_screen].bottom_padding * g_y_PxToABS;
    }
    else
    {
        rt = g_displays[last_screen].rot_s15bit_top;
        at = g_displays[last_screen].abs_top;
        my = g_displays[last_screen].ySlope;
        y = my * (rt - pitch) + at;
    }

    // logToWix(fmt::format("off monitors"));
    SendMyInput(x, y);

    return;

}