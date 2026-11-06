#pragma once

#include "BaseClasses/VehicleTest.h"

/// ArduPilot motor identification layouts and SERVOn_FUNCTION remapping against an ArduCopter MockLink
class APMMotorIdentificationControllerTest : public VehicleTestAPM
{
    Q_OBJECT

public:
    explicit APMMotorIdentificationControllerTest(QObject* parent = nullptr)
        : VehicleTestAPM(parent)
    {
        setWaitForParameters(true);
    }

private slots:
    void _loadLayout_data();
    void _loadLayout();
    void _motorFunction();
    void _remapServoFunctions_data();
    void _remapServoFunctions();
    void _xConfigFollowsFrameType_data();
    void _xConfigFollowsFrameType();
    void _noLayoutWithoutFrame();
    void _triSkipsTailServo();
    void _remapMotors();

private:
    void _setFrame(int frameClass, int frameType);
    void _setParam(const QString& name, int value);
    int _intParam(const QString& name) const;
};
