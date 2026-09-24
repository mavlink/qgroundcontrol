#pragma once

#include <cstdint>

#include "GPSDriverReports.h"

struct GPSNativeSurveyReport
{
    uint64_t timestamp = 0;
    GPSSurveyReport survey{};
};
