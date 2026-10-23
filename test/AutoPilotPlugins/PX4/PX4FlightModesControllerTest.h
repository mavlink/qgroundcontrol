#pragma once

#include "BaseClasses/VehicleTest.h"

/// PX4 Flight Modes page controller against a PX4 MockLink
class PX4FlightModesControllerTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4FlightModesControllerTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {
        setWaitForParameters(true);
    }

private slots:
    void _rcValuesFollowMonitoring_data();
    void _rcValuesFollowMonitoring();
    void _rcValuesExpireWithoutInput();
};
