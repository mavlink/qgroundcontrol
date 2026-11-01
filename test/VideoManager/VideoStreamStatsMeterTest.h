#pragma once

#include "UnitTest.h"

/// VideoStreamStatsMeter's rates and totals, and the VideoStreamStatsSampler that adds the session's fields.
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
    void _testBitrateNeedsByteCount();
    void _testSamplerAddsSessionFields();
};
