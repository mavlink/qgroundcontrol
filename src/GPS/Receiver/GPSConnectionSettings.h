#pragma once

#include "GPSConnectionConfig.h"

class RTKSettings;

namespace GPSConnectionSettings {
GPSConnectionConfig fromSettings(RTKSettings& settings);
}
