#pragma once
#include "PortableTest.h"

class DataRateTrackerTest : public PortableTest
{
    Q_OBJECT
private slots:
    void testInitialState();
    void testRecordBytesAccumulates();
    void testReset();
    void testKBpsConversion();
};
