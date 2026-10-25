#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies that a "[cal]" status text from ArduPilot, which does not use that prefix for calibration, still shows up
/// in the vehicle messages like any other text. Only PX4 calibration texts are kept out of them.
class APMCalibrationStatusTextTest : public VehicleTestAPM
{
    Q_OBJECT

public:
    explicit APMCalibrationStatusTextTest(QObject* parent = nullptr)
        : VehicleTestAPM(parent)
    {}

private slots:
    void _calibrationPrefixedTextShownAsVehicleMessage();
};
