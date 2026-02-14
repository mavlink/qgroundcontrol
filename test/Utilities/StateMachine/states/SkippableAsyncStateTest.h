#pragma once

#include "UnitTest.h"

class SkippableAsyncStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testSkippableAsyncStateExecute();
    void _testSkippableAsyncStateSkip();
    void _testSkippableAsyncStateTimeout();
    void _testSkippableAsyncStateWithSkipAction();
};
