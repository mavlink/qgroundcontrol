#pragma once

#include "UnitTest.h"

class TileFetchMetricsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _testCacheHitMissCounters();
    void _testNetworkSuccessBytesAndLatency();
    void _testNotModifiedCounter();
    void _testNetworkErrorCounter();
    void _testReset();
};
