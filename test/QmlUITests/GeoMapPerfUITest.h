#pragma once

#include "QmlUITestBase.h"

/// Manual GeoMap performance benchmark (not registered with CTest, never runs
/// in CI or standard test sweeps). Drives a fixed, repeatable gesture script
/// against the live FlyViewGeo view while the SurfacePatchModel perf capture
/// records per-second counters, then prints the CSV path for offline analysis.
///
/// Run explicitly (--onscreen: offscreen/software rendering makes the timings meaningless):
///   ./QGroundControl.app/Contents/MacOS/QGroundControl --unittest:GeoMapPerfUITest --onscreen
class GeoMapPerfUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _benchmark();
};
