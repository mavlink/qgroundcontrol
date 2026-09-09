#pragma once

#include "UnitTest.h"

class NTRIPSourceTableTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testParseSTRLine();
    void _testParseShortLine();
    void _testParseNonSTRLine();
    void _testParseFullTable();
    void _testDistanceCalculation();
    void _testUpdateDistancesAll();
    void _testEmptyTable();
};
