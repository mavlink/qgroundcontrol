#pragma once

#include "UnitTest.h"

class ScheduledTaskTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _replacementAndReentrantScheduling();
    void _repeatingRunsUntilCancelled();
    UT_PARAMETERIZED_TEST(_dependencyLifetime);
    UT_PARAMETERIZED_TEST(_cancellationAndReplacement);
    UT_PARAMETERIZED_TEST(_invalidRequests);
    UT_PARAMETERIZED_TEST(_callbackDestroysDependency);
};
