#pragma once

#include "UnitTest.h"

/// Tests for the filters that keep implausible readings of the ground station's own sensors out of QGC.
/// The sensors themselves need a platform backend and are not covered.
class QGCSensorsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testAmbientTemperatureFilter_data();
    void _testAmbientTemperatureFilter();
    void _testPressureFilter_data();
    void _testPressureFilter();
    void _testCompassFilter_data();
    void _testCompassFilter();
};
