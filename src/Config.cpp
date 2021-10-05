#include "Config.h"

#include <string>

#include "toml.hpp"

#include "Log.h"

#define FMT_HEADER_ONLY
#include <fmt\format.h>

void  CConfig::LoadSettings(int num_monitors)
{
    int default_left_padding = 0;
    int default_right_padding = 0;
    int default_top_padding = 0;
    int default_bottom_padding = 0;
    
    std::string d = "display";

    // TOML will throw a std::runtime_error if there's a problem opening the file 
    auto data = toml::parse("settings.toml");

    // Find the general settings table
    auto& general_settings_table = toml::find(data, "general");

    // find_or will return a default if parameter not found
    /*profile_ID = toml::find_or<int>(general_settings_table, "profile_ID", 13302);*/
    bWatchdog = toml::find_or<bool>(general_settings_table, "watchdog_enabled", 0);
    display_profile = toml::find<int>(general_settings_table, "profile");

    //Optionally the user can specify the location to the trackIR dll
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

    // load in the global default padding table if available
    try {
        auto& default_padding_table = toml::find(data, "default_padding");
        default_left_padding = toml::find<int>(default_padding_table, "left");
        default_right_padding = toml::find<int>(default_padding_table, "right");
        default_top_padding = toml::find<int>(default_padding_table, "top");
        default_bottom_padding = toml::find<int>(default_padding_table, "bottom");

    }
    catch (std::out_of_range e) {
        logToWix(fmt::format("Exception with the default padding table."));
        logToWix(fmt::format("TOML Non Crititcal Exception Thrown.\n{}\n", e.what()));
    }

    logToWix(fmt::format("\n{:-^50}\n", "User Mapping Info"));

    // Find the profiles table that contains all profiles
    auto& display_mapping_profiles = toml::find(data, "profiles");

    // Find the profile table which is currently enabled enabled
    std::string profile_table_name = std::to_string(display_profile);
    auto& profile_data = toml::find(display_mapping_profiles, profile_table_name);

    // Load in current profile dependent settings
    profile_ID = toml::find_or<int>(profile_data, "profile_ID", 13302);

    // Load in Display Mappings
    // Find the display mapping table for the given profile
    auto& display_mapping = toml::find(profile_data, "display");

    logToWix(fmt::format("Padding\n"));

    for (int i = 0; i < num_monitors; i++) {
        std::string tname = std::to_string(i);
        try {
            const auto& toml_display = toml::find(display_mapping, tname);

            bounds[i].left = toml::find<float>(toml_display, "left");
            bounds[i].right = toml::find<float>(toml_display, "right");
            bounds[i].top = toml::find<float>(toml_display, "top");
            bounds[i].bottom = toml::find<float>(toml_display, "bottom");

            // I return an ungodly fake high padding number,
            // so that I can tell of one was found in the toml config file
            // without producing an exception if a value was not found.
            // Padding values are not critical the program operation.
            int left_padding = toml::find_or<int>(toml_display, "left_padding", 5555);
            int right_padding = toml::find_or<int>(toml_display, "right_padding", 5555);
            int top_padding = toml::find_or<int>(toml_display, "top_padding", 5555);
            int bottom_padding = toml::find_or<int>(toml_display, "bottom_padding", 5555);

            if (left_padding != 5555) {
                logToWix(fmt::format("Display {} Left:     {:>12}\n", i, left_padding));
                bounds[i].pad_left = left_padding;
            }
            else {
                logToWix(fmt::format("Display {} Left:     {:>12} (Default)\n", i, default_left_padding));
                bounds[i].pad_left = default_left_padding;
            }
            if (right_padding != 5555) {
                logToWix(fmt::format("Display {} Right:    {:>12}\n", i, right_padding));
                bounds[i].pad_right = right_padding;
            }
            else {
                logToWix(fmt::format("Display {} Right:    {:>12} (Default)\n", i, default_right_padding));
                bounds[i].pad_right = default_right_padding;
            }
            if (top_padding != 5555) {
                logToWix(fmt::format("Display {} Top:      {:>12}\n", i, top_padding));
                bounds[i].pad_top = top_padding;
            }
            else {
                logToWix(fmt::format("Display {} Top:      {:>12} (Default)\n", i, default_top_padding));
                bounds[i].pad_top = default_top_padding;
            }
            if (bottom_padding != 5555) {
                logToWix(fmt::format("Display {} Bottom:   {:>12}\n", i, bottom_padding));
                bounds[i].pad_bottom = bottom_padding;
            }
            else {
                logToWix(fmt::format("Display {} Bottom:   {:>12} (Default)\n", i, default_bottom_padding));
                bounds[i].pad_bottom = default_bottom_padding;
            }
        }
        catch (std::out_of_range e)
        {
            logToWix(fmt::format("TOML Exception Thrown!\nIncorrect configuration of display:{}\n{}\n", i, e.what()));
            // I wanted to throw std::runtime_error, but i haven't figured out how yet
            throw 23;
        }
    }

}