#pragma once

#include "UnitTest.h"

/// Tests for the ground station's own sensors: the filters that keep implausible readings out of QGC, and how
/// each sensor starts, reports and stops. A fake Qt Sensors backend stands in for the platform's.
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

    void _testSensorAbsent_data();
    void _testSensorAbsent();
    void _testSensorFailsToStart_data();
    void _testSensorFailsToStart();
    void _testSensorReportsReadings_data();
    void _testSensorReportsReadings();
    void _testSensorLogsBackend_data();
    void _testSensorLogsBackend();
    void _testCompassReportsDirection();

private:
    template <typename Sensor>
    void _checkSensorAbsent();
    template <typename Sensor>
    void _checkSensorFailsToStart();
    template <typename Sensor>
    void _checkSensorReportsReadings();
    template <typename Sensor>
    void _checkSensorLogsBackend();
};
