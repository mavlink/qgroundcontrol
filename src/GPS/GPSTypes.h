#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QStringView>

#include <array>

enum class GPSType : int {
    UBlox      = 0,
    Trimble    = 1,
    Septentrio = 2,
    Femto      = 3,
    NMEA       = 4
};
Q_DECLARE_METATYPE(GPSType)

struct GPSTypeInfo {
    GPSType type;
    const char* matchString;
    const char* displayName;
    int manufacturerId;
};

inline constexpr std::array<GPSTypeInfo, 5> kGPSTypeRegistry {{
    {GPSType::Trimble,    "trimble",    "Trimble",    1},
    {GPSType::Septentrio, "septentrio", "Septentrio", 2},
    {GPSType::Femto,      "femtomes",   "Femtomes",   3},
    {GPSType::UBlox,      "blox",       "U-blox",     4},
    {GPSType::NMEA,       "nmea",       "NMEA",       5},
}};

inline GPSType gpsTypeFromString(QStringView str, int* manufacturerId = nullptr)
{
    for (const auto& entry : kGPSTypeRegistry) {
        if (str.contains(QLatin1String(entry.matchString), Qt::CaseInsensitive)) {
            if (manufacturerId) *manufacturerId = entry.manufacturerId;
            return entry.type;
        }
    }
    if (manufacturerId) *manufacturerId = 4;
    return GPSType::UBlox;
}

inline const char* gpsTypeDisplayName(GPSType type)
{
    for (const auto& entry : kGPSTypeRegistry) {
        if (entry.type == type) return entry.displayName;
    }
    return "Unknown";
}
