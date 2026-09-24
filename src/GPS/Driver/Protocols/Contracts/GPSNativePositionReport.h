#pragma once

#include "GPSDriverReports.h"

struct GPSNativePositionReport
{
    GPSNavigationValues navigation{};
    bool velocityValid{};
};
