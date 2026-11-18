#pragma once

#include "UnitTest.h"

/// Tests for SignalHandler (graceful shutdown on SIGINT/SIGTERM)
class SignalHandlerTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Basic lifecycle tests
    void _testConstruction();
    void _testSetupReturnsSuccess();

    // Multiple handlers
    void _testCurrentUpdatesWithNewHandler();

    // Signal delivery (POSIX)
    void _testSignalInChild_data();
    void _testSignalInChild();
};
