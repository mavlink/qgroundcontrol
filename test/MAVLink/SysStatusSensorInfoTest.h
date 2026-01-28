#pragma once

#include "TestFixtures.h"

/// Unit tests for SysStatusSensorInfo MAVLink sensor parsing.
class SysStatusSensorInfoTest : public OfflineTest
{
    Q_OBJECT

public:
    SysStatusSensorInfoTest() = default;

private slots:
    // Basic functionality tests
    void _initialStateTest();
    void _singleSensorUpdateTest();
    void _multipleSensorUpdateTest();

    // Sensor status tests
    void _sensorEnabledHealthyTest();
    void _sensorEnabledUnhealthyTest();
    void _sensorDisabledTest();

    // State change tests
    void _sensorAddedRemovedTest();
    void _sensorStatusChangeTest();
    void _noChangeNoSignalTest();

    // Ordering tests
    void _sensorOrderingTest();
};
