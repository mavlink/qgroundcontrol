#pragma once

#include "UnitTest.h"

/// The pipe to a link another thread serves: GPSPipe, GPSPipeTransport and GPSPipeLink, through the checks of
/// Transport/Support/PipeTransportChecks.h, which test/GPS/Standalone runs too.
class GPSPipeTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _checks_data();
    void _checks();
};
