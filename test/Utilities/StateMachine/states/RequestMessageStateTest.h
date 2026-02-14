#pragma once

#include "UnitTest.h"

/// Tests for RequestMessageState
/// Note: Most tests require a MockLink/Vehicle connection
class RequestMessageStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testStateCreation();
    void _testDefaultParameters();
    void _testTimeoutConfiguration();
};
