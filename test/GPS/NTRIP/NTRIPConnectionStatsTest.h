#pragma once

#include "UnitTest.h"

class NTRIPConnectionStatsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testRecordMessage();
    void testReset();
    void testDataRate();
};
