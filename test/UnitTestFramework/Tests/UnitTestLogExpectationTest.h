#pragma once

#include "UnitTest.h"

/// Tests for UnitTest log expectation helpers.
class UnitTestLogExpectationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testExpectVerifySingleMessage();
    void _testExpectVerifyCategoryScopedMessage();
    void _testExpectVerifyMultipleMessagesInOrder();
    void _testExpectVerifyDuplicatePatternMessages();
    void _testIgnoreLogMessagePersistentSuppression();
};
