#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies APMFirmwarePlugin rewrites ArduPilot PARAM_VALUE messages, which carry every value as a cast
/// float, into MAVLink's bytewise mavlink_param_union_t encoding with no indeterminate bytes.
class APMParamValueTest : public VehicleTestAPM
{
    Q_OBJECT

public:
    explicit APMParamValueTest(QObject* parent = nullptr)
        : VehicleTestAPM(parent)
    {}

private slots:
    void _supportedTypeReencodedBytewise_data();
    void _supportedTypeReencodedBytewise();
    void _unsupportedTypeReencodedAsZero();
};
