#pragma once
#include "PortableTest.h"

class DataRateTrackerTest : public PortableTest
{
    Q_OBJECT
private slots:
    void testInitialState();
    void testRecordBytesAccumulates();
    UT_PARAMETERIZED_TEST(testReset);
    void testRefreshDuringSilence();
};
