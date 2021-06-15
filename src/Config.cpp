#include "Config.h"

#include <string>
#include "toml.hpp"


void  CConfig::LoadSettings(int num_monitors)
{
    int default_left_padding = 0;
    int default_right_padding = 0;
    int default_top_padding = 0;
    int default_bottom_padding = 0;
    
    std::string d = "display";

    // TOML will throw a std::runtime_error if there's a problem opening the file 
    auto data = toml::parse("settings.toml");

    profile_ID = toml::find_or<int>(data, "profile_ID", 13302);
    bWatchdog = toml::find_or<bool>(data, "watchdog_enabled", 0);

    sTrackIR_dll_location = toml::find_or<std::string>(data, "TrackIR_dll_directory", "C:\\Program Files (x86)\\NaturalPoint\\TrackIR5");

    if (sTrackIR_dll_location.back() != '\\')
    {
        sTrackIR_dll_location.push_back('\\');
    }

    #if defined(_WIN64) || defined(__amd64__)
        sTrackIR_dll_location.append("NPClient64.dll");
    #else	    
        sTrackIR_dll_location.append("NPClient.dll");
    #endif

    // load in the default padding table if available
    try {
        auto default_padding = toml::find(data, "default_padding");
        default_left_padding = toml::find<int>(default_padding, "left");
        default_right_padding = toml::find<int>(default_padding, "right");
        default_top_padding = toml::find<int>(default_padding, "top");
        default_bottom_padding = toml::find<int>(default_padding, "bottom");

    }
    catch (std::out_of_range e) {
        printf("Exception with the default padding table.");
        printf("TOML Non Crititcal Exception Thrown.\n%s\n", e.what());
    }

    printf("\n--------User Mapping Info----------------------\n");
    for (int i = 0; i < num_monitors; i++) {
        std::string tname = d + std::to_string(i);
        try {
            const auto& toml_display = toml::find(data, tname);

            bounds[i].left = toml::find<float>(toml_display, "left");
            bounds[i].right = toml::find<float>(toml_display, "right");
            bounds[i].top = toml::find<float>(toml_display, "top");
            bounds[i].bottom = toml::find<float>(toml_display, "bottom");

            // I return an ungodly fake high padding number
            // So that I can tell of one was found in the toml config file
            int left_padding = toml::find_or<int>(toml_display, "left_padding", 5555);
            int right_padding = toml::find_or<int>(toml_display, "right_padding", 5555);
            int top_padding = toml::find_or<int>(toml_display, "top_padding", 5555);
            int bottom_padding = toml::find_or<int>(toml_display, "bottom_padding", 5555);

            printf("Padding\n");

            if (left_padding != 5555) {
                printf("Display %d Left:   %d\n", i, left_padding);
                bounds[i].pad_left = left_padding;
            }
            else {
                printf("Display %d Left:   %d (Default)\n", i, default_left_padding);
                bounds[i].pad_left = default_left_padding;
            }
            if (right_padding != 5555) {
                printf("Display %d Right:  %d\n", i, right_padding);
                bounds[i].pad_right = right_padding;
            }
            else {
                printf("Display %d Right:  %d (Default)\n", i, default_right_padding);
                bounds[i].pad_right = default_right_padding;
            }
            if (top_padding != 5555) {
                printf("Display %d Top:    %d\n", i, top_padding);
                bounds[i].pad_top = top_padding;
            }
            else {
                printf("Display %d Top:    %d (Default)\n", i, default_top_padding);
                bounds[i].pad_top = default_top_padding;
            }
            if (bottom_padding != 5555) {
                printf("Display %d Bottom:   %d\n", i, bottom_padding);
                bounds[i].pad_bottom = bottom_padding;
            }
            else {
                printf("Display %d Bottom:   %d (Default)\n", i, default_bottom_padding);
                bounds[i].pad_bottom = default_bottom_padding;
            }
        }
        catch (std::out_of_range e)
        {
            printf("TOML Exception Thrown!\nIncorrect configuration of display:%d\n%s\n", i, e.what());
        }
    }

}