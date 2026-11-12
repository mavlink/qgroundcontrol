#pragma once

#include "UnitTest.h"

/// The satellites NMEA describes: each one's direction and signal from GSV, and whether it is used from GSA.
class NMEASatellitesTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _gsvSatellites_data();
    void _gsvSatellites();
    void _epochSatellites();
    void _epochWithoutUse();
};
