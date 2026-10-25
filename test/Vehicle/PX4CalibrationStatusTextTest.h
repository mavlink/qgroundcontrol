#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies PX4 "[cal]" status texts reach calibration listeners but stay out of the vehicle messages and alerts.
class PX4CalibrationStatusTextTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4CalibrationStatusTextTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _calibrationTextNotShownAsVehicleMessage();
};
