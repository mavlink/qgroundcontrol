#pragma once
#include "UnitTest.h"

class DataRateTrackerTest : public UnitTest
{
    Q_OBJECT
private slots:
    void testInitialState();
    void testRecordBytesAccumulates();
    void testReset();
    void testKBpsConversion();
};
