#ifndef TRACKIRMOUSE_ENVIROMENT_H
#define TRACKIRMOUSE_ENVIROMENT_H

#include <vector>

namespace env {

using HardwareDisplays = std::vector<std::vector<int>>;

typedef struct HardwareDisplayInfo_
{
  HardwareDisplays displays;
  int height;
  int width;
} HardwareDisplayInfo;

HardwareDisplayInfo
GetHardwareDisplayInfo();
} // namespace env

#endif /* TRACKIRMOUSE_ENVIROMENT_H */
