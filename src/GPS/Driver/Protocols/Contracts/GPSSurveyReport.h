#pragma once

#include <cstdint>

struct GPSSurveyReport
{
    enum class AltitudeDatum
    {
        Unknown,
        Ellipsoid,
        MeanSeaLevel
    };
    AltitudeDatum altitudeDatum = AltitudeDatum::Unknown;
    bool accuracyKnown = false;
    uint64_t timestamp = 0;
    double latitude = 0;
    double longitude = 0;
    float altitude = 0;
    uint32_t mean_accuracy = 0;
    uint32_t duration = 0;
    uint8_t flags = 0;
};
