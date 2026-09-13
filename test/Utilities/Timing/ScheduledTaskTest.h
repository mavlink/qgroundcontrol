#pragma once

#include "UnitTest.h"

class ScheduledTaskTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _replacementAndReentrantScheduling();
    void _dependencyLifetime_data();
    void _dependencyLifetime();
    void _cancellationAndReplacement_data();
    void _cancellationAndReplacement();
    void _invalidRequests_data();
    void _invalidRequests();
    void _callbackDestroysDependency_data();
    void _callbackDestroysDependency();
};
