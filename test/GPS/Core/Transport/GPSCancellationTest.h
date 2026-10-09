#pragma once

#include "UnitTest.h"

class GPSCancellationTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _cancelRunsCallbacksOnce();
    void _destructionWaitsForRunningCallback();
};
