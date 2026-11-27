#pragma once

#include "UnitTest.h"

class TileRegionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDefaultConstruction();
    void _testClear();
    void _testPlusEqualsAccumulates();
};
