#pragma once

#include <QtCore/QDebug>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QStringView>

#include <array>

Q_DECLARE_LOGGING_CATEGORY(GPSTypesLog)

enum class GPSType : int {
    UBlox      = 0,
    Trimble    = 1,
    Septentrio = 2,
    Femto      = 3,
    NMEA       = 4,
    MTK        = 5,
    EmlidReach = 6
};
Q_DECLARE_METATYPE(GPSType)

struct GPSTypeInfo {
    GPSType type;
    const char* matchString;
    const char* displayName;
    int manufacturerId;  ///< Matches GPSDeviceFlags::Manufacturer enum values
    int deviceFlag;      ///< Bitmask matching GPSDeviceFlags::Flag (0 = aliased to Nmea at lookup)
};

inline constexpr std::array<GPSTypeInfo, 7> kGPSTypeRegistry {{
    // type              match         display       mfgId  flag
    {GPSType::Trimble,    "trimble",    "Trimble",    1,     0x01}, // GPSDeviceFlags::Trimble
    {GPSType::Septentrio, "septentrio", "Septentrio", 2,     0x02}, // GPSDeviceFlags::Septentrio
    {GPSType::Femto,      "femtomes",   "Femtomes",   3,     0x04}, // GPSDeviceFlags::Femtomes
    {GPSType::UBlox,      "blox",       "U-blox",     4,     0x08}, // GPSDeviceFlags::Ublox
    {GPSType::NMEA,       "nmea",       "NMEA",       5,     0x10}, // GPSDeviceFlags::Nmea
    {GPSType::MTK,        "mtk",        "MTK",        6,     0x10}, // MTK treated as generic NMEA
    {GPSType::EmlidReach, "emlid",      "Emlid Reach", 7,    0x10}, // Emlid treated as generic NMEA
}};

inline GPSType gpsTypeFromString(QStringView str, int* manufacturerId = nullptr)
{
    for (const auto& entry : kGPSTypeRegistry) {
        if (str.contains(QLatin1String(entry.matchString), Qt::CaseInsensitive)) {
            if (manufacturerId) *manufacturerId = entry.manufacturerId;
            return entry.type;
        }
    }
    qCWarning(GPSTypesLog) << "Unrecognized GPS type string:" << str << ", defaulting to U-blox";
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
