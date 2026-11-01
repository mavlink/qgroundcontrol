#pragma once

#include "UnitTest.h"

class UVCFrameRelayTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testTransformedLeavesFrameAlone();
    void _testTransformedShowsTheOutputTransform();
    void _testRelaysLatestFrameOnItsThread();
};
