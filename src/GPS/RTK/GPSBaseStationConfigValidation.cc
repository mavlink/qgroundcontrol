#include "GPSBaseStationConfigValidation.h"

#include <cmath>
#include <cstdint>
#include <limits>

#include <QtCore/QCoreApplication>

QString gpsBaseStationConfigError(const GPSBaseStationConfig& config)
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSReceiverConfig", text); };
    constexpr double MAX_UNSIGNED_VALUE = (std::numeric_limits<uint32_t>::max)();
    if (config.useFixedBase) {
        const double altitudeCm = static_cast<double>(config.fixedBaseAltitudeMeters) * 100.0;
        // Match legacy float conversions before checking wire limits.
        const double accuracyUnits = static_cast<double>((config.fixedBaseAccuracyMeters * 1000.0f) * 10.0f);
        if (!std::isfinite(config.fixedBaseLatitude) || std::abs(config.fixedBaseLatitude) > 90.0 ||
            !std::isfinite(config.fixedBaseLongitude) || std::abs(config.fixedBaseLongitude) > 180.0 ||
            !std::isfinite(altitudeCm) || altitudeCm < (std::numeric_limits<int32_t>::min)() ||
            altitudeCm > (std::numeric_limits<int32_t>::max)() || !std::isfinite(accuracyUnits) || accuracyUnits < 0 ||
            accuracyUnits > MAX_UNSIGNED_VALUE) {
            return tr("Enter a valid fixed base position and accuracy");
        }
    } else {
        const double accuracyUnits = config.surveyInAccMeters * 10000.0;
        if (!std::isfinite(accuracyUnits) || accuracyUnits < 1 || accuracyUnits > MAX_UNSIGNED_VALUE ||
            config.surveyInDurationSecs < 1 || config.surveyInDurationSecs > (std::numeric_limits<uint32_t>::max)()) {
            return tr("Enter a valid survey-in accuracy and duration");
        }
    }
    return {};
}
