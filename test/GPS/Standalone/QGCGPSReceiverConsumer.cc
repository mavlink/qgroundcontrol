#include <cmath>
#include <iostream>
#include <type_traits>

#include <QtCore/QVariant>

#include "GPSDriverReports.h"
#include "GPSReceiverCapabilities.h"
#include "GPSReceiverConfig.h"

#if defined(QT_NETWORK_LIB) || defined(QT_POSITIONING_LIB) || defined(QT_QML_LIB) || defined(QT_SERIALPORT_LIB)
#error Receiver contracts must not inherit transport, positioning, or application dependencies.
#endif

static_assert(std::is_copy_constructible_v<GPSPositionReport>);
static_assert(std::is_copy_constructible_v<GPSSatelliteReport>);
static_assert(std::is_copy_constructible_v<GPSSurveyReport>);

int main()
{
    GPSPositionReport position;
    if (position.fixType != GPSPositionReport::FixType::Unknown || position.satellitesUsed ||
        !std::isnan(position.latitudeDegrees) || !std::isnan(position.longitudeDegrees) ||
        !std::isnan(position.altitudeMslMeters) || !std::isnan(position.altitudeEllipsoidMeters)) {
        std::cerr << "Default native position must not manufacture a fix or altitude\n";
        return 1;
    }
    const auto& integrity = position.integrity;
    if (integrity.timestampUs != 0 || integrity.jamming != GPSIntegrityReport::JammingState::Unknown ||
        integrity.spoofing != GPSIntegrityReport::SpoofingState::Unknown ||
        integrity.correctionUse != GPSIntegrityReport::CorrectionUse::Unknown || integrity.noisePerMillisecond ||
        integrity.automaticGainControl || integrity.jammingIndicator || integrity.correctionCrcFailed) {
        std::cerr << "Unreported integrity fields must remain unknown\n";
        return 2;
    }
    position.integrity.correctionCrcFailed = false;
    position.integrity.noisePerMillisecond = 0;
    const auto snapshot = position;
    position.integrity.noisePerMillisecond = 42;
    if (snapshot.integrity.noisePerMillisecond != 0 || snapshot.integrity.correctionCrcFailed != false) {
        std::cerr << "Known zero/false diagnostics must survive owning report copies\n";
        return 3;
    }
    GPSSatelliteReport satellites;
    if (satellites.count != 0 || satellites.satellites.front().used || satellites.satellites.front().signalStrength) {
        std::cerr << "Empty native satellite reports must not claim usage or signal evidence\n";
        return 4;
    }
    GPSSurveyReport survey;
    if (!std::isnan(survey.latitudeDegrees) || !std::isnan(survey.longitudeDegrees) ||
        !std::isnan(survey.altitudeEllipsoidMeters) || survey.meanAccuracyMeters || survey.valid || survey.active) {
        std::cerr << "Empty native survey reports must not manufacture a base or accuracy\n";
        return 5;
    }
    const auto transported = QVariant::fromValue(snapshot).value<GPSPositionReport>();
    if (transported.integrity.noisePerMillisecond != 0 || transported.integrity.correctionCrcFailed != false) {
        std::cerr << "Receiver reports must own their Qt metatype declarations and copy semantics\n";
        return 6;
    }

    using Error = GPSReceiverConfigError;
    using Role = GPSReceiverConfig::Role;
    if (gpsValidateBaseStationConfig({}) != Error::InvalidSurveyIn ||
        gpsValidateReceiverConfig(GPSType::ublox, {}) != Error::InvalidSurveyIn) {
        std::cerr << "Default receiver configuration must not manufacture a valid survey request\n";
        return 7;
    }
    GPSReceiverConfig config{.role = Role::Position, .dynamicModel = 0};
    if (!gpsReceiverCapabilities(GPSType::ublox, config.role).dynamicModel ||
        gpsValidateReceiverConfig(GPSType::ublox, config) != Error::None) {
        std::cerr << "Supported u-blox positioning settings must remain valid\n";
        return 8;
    }
    config.headingOffsetRadians = 0.0f;
    return gpsValidateReceiverConfig(GPSType::ublox, config) == Error::UnsupportedHeadingOffset ? 0 : 9;
}
