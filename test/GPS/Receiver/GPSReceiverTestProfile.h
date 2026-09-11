#pragma once

#include "GPSReceiverProfile.h"

inline GPSReceiverProfile gpsReceiverTestProfile(GPSReceiverConfig config = {.base = {.surveyInAccMeters = 2.0,
                                                                                      .surveyInDurationSecs = 180}},
                                                 GPSType type = GPSType::u_blox)
{
    return {.endpoint = {.kind = GPSReceiverProfile::Endpoint::Kind::Serial, .discoverSerialDevice = true},
            .configurationPolicy = GPSReceiverProfile::ConfigurationPolicy::Configure,
            .driverType = type,
            .receiver = config};
}
