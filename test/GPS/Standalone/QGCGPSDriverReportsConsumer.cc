#include <cmath>
#include <iostream>
#include <type_traits>

#include "GPSDriverReports.h"

// This consumer must not require a PX4 adapter or transport implementation.
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
    return 0;
}
