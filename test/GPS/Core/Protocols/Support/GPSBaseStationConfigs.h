#pragma once

#include <chrono>

#include "GPSReceiverConfig.h"

// The base-station configurations the GPS suites request. A suite passes one as the base of the request its layer
// takes: GPSConfig, GPSReceiverConfig or the receiver's configuration.
namespace GPSTest {

/// A base that surveys its position in for at least @a duration, until it is known to @a accuracyMeters.
inline GPSBaseStationConfig surveyInBase(double accuracyMeters, std::chrono::seconds duration)
{
    return {.mode = GPSBaseStationConfig::SurveyIn{.accuracyMeters = accuracyMeters, .duration = duration}};
}

/// A base at a known WGS84 position, @a altitudeMeters above the ellipsoid, which is known to @a accuracyMeters.
inline GPSBaseStationConfig fixedBaseAt(double latitudeDegrees, double longitudeDegrees, double altitudeMeters,
                                        float accuracyMeters = 0.0f)
{
    return {.mode = GPSBaseStationConfig::Fixed{.position = {.latitudeDegrees = latitudeDegrees,
                                                             .longitudeDegrees = longitudeDegrees,
                                                             .altitudeMeters = altitudeMeters},
                                                .accuracyMeters = accuracyMeters}};
}

/// A base whose receiver averages its own position for at most @a maximumDuration.
inline GPSBaseStationConfig averagingBase(std::chrono::seconds maximumDuration)
{
    return {.mode = GPSBaseStationConfig::ReceiverAveraging{.maximumDuration = maximumDuration}};
}

}  // namespace GPSTest
