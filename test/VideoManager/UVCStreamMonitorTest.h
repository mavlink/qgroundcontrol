#pragma once

#include "UnitTest.h"

class QVideoSink;
class UVCStreamMonitor;

class UVCStreamMonitorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testFramesBecomeStats();
    void _testDetachClearsStats();
    void _testDestroyedSinkClearsStats();
    void _testStatsGroupShowsCamera();

private:
    /// Feeds @p monitor a second of 30 fps frames from @p sink after the baseline sample, which makes its stats valid.
    static void _measureSecond(UVCStreamMonitor& monitor, QVideoSink& sink, qint64& nowMs);
};
