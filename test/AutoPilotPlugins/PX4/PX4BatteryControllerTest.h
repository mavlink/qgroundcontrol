#pragma once

#include "BaseClasses/VehicleTest.h"

/// PX4 Battery page controller against a PX4 MockLink, which streams only the parameters PX4 would have looked up
/// and answers the rest by name.
class PX4BatteryControllerTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4BatteryControllerTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {
        setWaitForParameters(true);
    }

private slots:
    void _requestedParamPendingUntilReadEnds_data();
    void _requestedParamPendingUntilReadEnds();
    void _unansweredParamNotRequestedAgain();
    void _unrequestedParamCounted();
};
