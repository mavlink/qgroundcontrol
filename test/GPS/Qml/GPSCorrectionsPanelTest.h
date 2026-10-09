#pragma once

#include "UnitTest.h"

/// The panels of the RTK Corrections settings page, loaded outside the main window: correction status and the NTRIP
/// connection and mountpoint controls.
class GPSCorrectionsPanelTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _correctionsStatusShowsSelectedStream();
    void _ntripErrorActionRetries();
    void _ntripConnectionActionIsIdempotent_data();
    void _ntripConnectionActionIsIdempotent();
    void _ntripMountpointLockedWhileActive();
    void _ntripLongStatusWrapsWithinPanel();
    void _ntripMountpointButtonsAligned();
};
