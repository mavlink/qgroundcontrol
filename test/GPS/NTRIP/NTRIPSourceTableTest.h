#pragma once

#include "UnitTest.h"

class NTRIPSourceTableTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testParseSTRLine();
    void testParseShortLine();
    void testParseNonSTRLine();
    void testParseFullTable();
    void testDistanceCalculation();
    void testUpdateDistancesAll();
    void testEmptyTable();
};
