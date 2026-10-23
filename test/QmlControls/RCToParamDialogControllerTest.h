#pragma once

#include "BaseClasses/VehicleTest.h"

class RCToParamDialogControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _setTuningFactWithoutVehicle();
};

class RCToParamDialogControllerVehicleTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit RCToParamDialogControllerVehicleTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _setTuningFactRefreshesParameter();
};
