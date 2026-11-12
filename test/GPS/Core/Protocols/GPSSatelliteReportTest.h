#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The satellites a receiver has in view, from its messages to the report GPSDriver publishes.
class GPSSatelliteReportTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _nmeaEpochIsOneReport();
};
