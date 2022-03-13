#ifndef TRACKIRMOUSE_TRACK_HPP
#define TRACKIRMOUSE_TRACK_HPP

#include <windows.h>

#include "config.hpp"
#include "types.hpp"

namespace track {
retcode Initialize(HWND hWnd, config::Profile, std::string dllpath);
retcode TrackStart();
void TrackToggle();
void TrackStop();
}  // namespace track

#endif /* TRACKIRMOUSE_TRACK_HPP */
