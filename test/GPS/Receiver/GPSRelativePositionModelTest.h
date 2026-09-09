#pragma once

#include "UnitTest.h"

class GPSRelativePositionModelTest : public UnitTest
{
    Q_OBJECT
private slots:
    void _validityAndZeroBaseline();
    void _freshnessAndSessionIsolation();
    void _reentrantReplacement();
};
