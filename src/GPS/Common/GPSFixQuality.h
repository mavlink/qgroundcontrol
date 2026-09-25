#pragma once

enum class GPSFixQuality
{
    Unknown = 0,
    NoFix = 1,
    Fix2D = 2,
    Fix3D = 3,
    Differential = 4,
    RTKFloat = 5,
    RTKFixed = 6,
    Extrapolated = 8,
};

constexpr GPSFixQuality gpsFixQualityFromValue(int value)
{
    return (value >= static_cast<int>(GPSFixQuality::Unknown) && value <= static_cast<int>(GPSFixQuality::RTKFixed)) ||
                   value == static_cast<int>(GPSFixQuality::Extrapolated)
               ? static_cast<GPSFixQuality>(value)
               : GPSFixQuality::Unknown;
}
