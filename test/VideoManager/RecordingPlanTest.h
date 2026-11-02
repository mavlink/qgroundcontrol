#pragma once

#include "UnitTest.h"

/// The recording file contract across reconnects, without a stream.
class RecordingPlanTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testPartFile_data();
    void _testPartFile();
    void _testPartsFollowOpenedSessions();
    void _testRecordingTimeAndSize();
};
