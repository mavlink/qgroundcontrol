#pragma once

#include "UnitTest.h"

class RetryTransitionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testRetryActionCalled();
    void _testTransitionAfterMaxRetries();
    void _testRetryCountResets();
    void _testMultipleRetries();
    void _testZeroRetries();
};
