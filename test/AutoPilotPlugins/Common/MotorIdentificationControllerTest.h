#pragma once

#include "BaseClasses/VehicleTest.h"

/// Motors page identification routine against a quadrotor MockLink
class MotorIdentificationControllerTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit MotorIdentificationControllerTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _layoutAngles_data();
    void _layoutAngles();
    void _quadLayout();
    void _identify();
    void _ignoresUnavailablePositions();
    void _cancelStopsPulsingMotor();
    void _startRejectsInvalidThrottle();

private:
    /// @return true: the last command MockLink received is a motor test with these values
    bool _lastMotorTestIs(int motor, int throttlePercent, int timeoutSecs) const;
};
