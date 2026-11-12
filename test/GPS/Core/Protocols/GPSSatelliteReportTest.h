#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The satellites a receiver has in view, from its messages to the report GPSDriver publishes: u-blox NAV-SAT and
/// NAV-SVINFO, NMEA GSV with GSA, and how the report keeps and bounds them.
class GPSSatelliteReportTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _nmeaEpochIsOneReport();
    void _ubxCaptureSatellites_data();
    void _ubxCaptureSatellites();
    void _ubxLegacyNumbering();
    void _nmeaCaptureSatellites();
    void _reportKeepsEachConstellation();
    void _reportIsBounded();
};
