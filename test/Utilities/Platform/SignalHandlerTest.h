#pragma once

#include "UnitTest.h"

/// Tests for SignalHandler (graceful shutdown on SIGINT/SIGTERM)
class SignalHandlerTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Basic lifecycle tests
    void _testConstruction();
    void _testCurrentReturnsInstance();
    void _testSetupReturnsSuccess();
    void _testDestructorClearsCurrent();

    // Multiple handlers
    void _testCurrentUpdatesWithNewHandler();
};
