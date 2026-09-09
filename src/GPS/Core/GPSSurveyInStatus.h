#pragma once

#include <QtCore/QMetaType>

#include <cstdint>

/// Survey-in progress, translated from the px4 SurveyInStatus.
struct GPSSurveyInStatus
{
    double latitude = 0.0;
    double longitude = 0.0;
    float altitude = 0.0f;
    uint32_t meanAccuracyMM = 0;
    uint32_t durationSecs = 0;
    bool valid = false;
    bool active = false;
};
Q_DECLARE_METATYPE(GPSSurveyInStatus)
