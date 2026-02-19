#pragma once

#include "UnitTest.h"

/// Tests for UnitTest async wait helpers/macros.
class UnitTestAsyncHelpersTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testWaitForSignalSuccess();
    void _testWaitForSignalTimeout();
    void _testWaitForSignalCountSuccess();
    void _testWaitForSignalCountAlreadySatisfied();
    void _testWaitForConditionSuccess();
    void _testWaitForConditionTimeout();
    void _testWaitMacros();
};
