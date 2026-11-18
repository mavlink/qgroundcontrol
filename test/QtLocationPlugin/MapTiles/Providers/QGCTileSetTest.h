#pragma once

#include "UnitTest.h"

class QGCTileSetTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDefaultConstruction();
    void _testClear();
    void _testPlusEqualsAccumulates();
    void _testPlusEqualsWithZero();
    void _testPlusEqualsChaining();
};
