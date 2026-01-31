#pragma once

#include "UnitTest.h"

class InitialConnectTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _performTestCases();
    void _boardVendorProductId();
    void _progressTracking();
    void _highLatencySkipsPlans();
    void _arducopterConnect();
    void _stateMachineActiveStatus();
    void _stateTransitionOrder();
    void _multipleReconnects();
};
