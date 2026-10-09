#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The passive family: it refuses receiver configuration, decodes NMEA, UBX or SBF navigation, corrections and
/// satellites, and never writes to the receiver.
class PassiveProtocolTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _scenario_data();
    void _scenario();
};
