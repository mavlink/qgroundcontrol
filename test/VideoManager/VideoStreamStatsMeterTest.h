#pragma once

#include "UnitTest.h"

class VideoStreamStatsMeterTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFirstSampleIsBaseline();
    void _testRatesOverInterval_data();
    void _testRatesOverInterval();
    void _testRtpLoss();
    void _testCounterRestartKeepsTotalsGrowing();
    void _testResetStartsNewStream();
    void _testRatesAverageOverWindow();
};
