#pragma once

#include "UnitTest.h"

class SensorsComponentControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _largeVehicleCalibrationResult_data();
    void _largeVehicleCalibrationResult();
    void _largeVehicleCommandFailureText_data();
    void _largeVehicleCommandFailureText();
};
