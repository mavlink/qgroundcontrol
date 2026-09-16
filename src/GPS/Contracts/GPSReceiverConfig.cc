#include "GPSReceiverConfig.h"

#include <cmath>
#include <cstdint>
#include <limits>

#include <QtCore/QCoreApplication>

QString GPSReceiverConfig::validationError() const
{
    const auto tr = [](const char* text) { return QCoreApplication::translate("GPSReceiverConfig", text); };
    if (role != Role::RTKBase && role != Role::Position) {
        return tr("Select a valid receiver role");
    }
    if ((outputProtocol != OutputProtocol::Native && outputProtocol != OutputProtocol::NMEA) ||
        (outputProtocol == OutputProtocol::NMEA && role != Role::Position)) {
        return tr("Select a valid receiver output protocol");
    }
    if (!std::isfinite(headingOffsetDeg)) {
        return tr("Enter a finite receiver heading offset");
    }
    if (role != Role::RTKBase) {
        return {};
    }
    constexpr double MAX_UNSIGNED_VALUE = (std::numeric_limits<uint32_t>::max)();
    if (base.useFixedBase) {
        const double altitudeCm = static_cast<double>(base.fixedBaseAltitudeMeters) * 100.0;
        // Match legacy float conversions before checking wire limits.
        const double accuracyUnits = static_cast<double>((base.fixedBaseAccuracyMeters * 1000.0f) * 10.0f);
        if (!std::isfinite(base.fixedBaseLatitude) || std::abs(base.fixedBaseLatitude) > 90.0 ||
            !std::isfinite(base.fixedBaseLongitude) || std::abs(base.fixedBaseLongitude) > 180.0 ||
            !std::isfinite(altitudeCm) || altitudeCm < (std::numeric_limits<int32_t>::min)() ||
            altitudeCm > (std::numeric_limits<int32_t>::max)() || !std::isfinite(accuracyUnits) || accuracyUnits < 0 ||
            accuracyUnits > MAX_UNSIGNED_VALUE) {
            return tr("Enter a valid fixed base position and accuracy");
        }
    } else {
        const double accuracyUnits = base.surveyInAccMeters * 10000.0;
        if (!std::isfinite(accuracyUnits) || accuracyUnits < 1 || accuracyUnits > MAX_UNSIGNED_VALUE ||
            base.surveyInDurationSecs < 1 || base.surveyInDurationSecs > (std::numeric_limits<uint32_t>::max)()) {
            return tr("Enter a valid survey-in accuracy and duration");
        }
    }
    return {};
}
