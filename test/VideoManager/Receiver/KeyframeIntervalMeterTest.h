#pragma once

#include "UnitTest.h"

/// The keyframe interval the receivers' statistics give, from the keyframes' timestamps alone.
class KeyframeIntervalMeterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testAveragesOverWindow();
    void _testTimeGoingBackStartsOver();
};
