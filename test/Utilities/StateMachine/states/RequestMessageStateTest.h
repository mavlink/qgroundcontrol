#pragma once

#include "StateMachineTest.h"

/// Tests for RequestMessageState
/// Note: Most tests require a MockLink/Vehicle connection
class RequestMessageStateTest : public StateMachineTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testDefaultParameters();
    void _testTimeoutConfiguration();
};
