#include "GPSReceiverConfig.h"

#include <QtCore/QCoreApplication>

#include <cmath>
#include <cstdint>
#include <limits>

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
    if (base.useFixedBase) {
        if (!std::isfinite(base.fixedBaseLatitude) || std::abs(base.fixedBaseLatitude) > 90.0 ||
            !std::isfinite(base.fixedBaseLongitude) || std::abs(base.fixedBaseLongitude) > 180.0 ||
            !std::isfinite(base.fixedBaseAltitudeMeters) ||
            std::abs(static_cast<double>(base.fixedBaseAltitudeMeters) * 100.0) >
                (std::numeric_limits<int32_t>::max)() ||
            !std::isfinite(base.fixedBaseAccuracyMeters) || base.fixedBaseAccuracyMeters < 0.0f ||
            static_cast<double>(base.fixedBaseAccuracyMeters * 1000.0f * 10.0f) >
                (std::numeric_limits<uint32_t>::max)()) {
            return tr("Enter a valid fixed base position and accuracy");
        }
    } else if (!std::isfinite(base.surveyInAccMeters) || base.surveyInAccMeters <= 0.0 ||
               base.surveyInAccMeters * 10000.0 > (std::numeric_limits<uint32_t>::max)() ||
               base.surveyInDurationSecs <= 0) {
        return tr("Enter a valid survey-in accuracy and duration");
    }
    return {};
}
