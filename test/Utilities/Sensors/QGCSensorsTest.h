#pragma once

#include "UnitTest.h"

/// Tests for the ground station's own sensors: the filters that keep implausible readings out of QGC, and how
/// each sensor starts, reports and stops. A fake Qt Sensors backend stands in for the platform's.
class QGCSensorsTest : public UnitTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testAmbientTemperatureFilter);
    UT_PARAMETERIZED_TEST(_testPressureFilter);
    UT_PARAMETERIZED_TEST(_testCompassFilter);

    UT_PARAMETERIZED_TEST(_testSensorAbsent);
    UT_PARAMETERIZED_TEST(_testSensorFailsToStart);
    UT_PARAMETERIZED_TEST(_testSensorReportsReadings);
    UT_PARAMETERIZED_TEST(_testRepeatedInit);
    UT_PARAMETERIZED_TEST(_testSensorLogsBackend);
    void _testCompassReportsDirection();

private:
    template <typename Sensor>
    void _checkSensorAbsent();
    template <typename Sensor>
    void _checkSensorFailsToStart();
    template <typename Sensor>
    void _checkSensorReportsReadings();
    template <typename Sensor>
    void _checkRepeatedInit();
    template <typename Sensor>
    void _checkSensorLogsBackend();
};
