#pragma once

#include "UnitTest.h"

class QtVideoRecorderTest : public UnitTest
{
    Q_OBJECT

private:
    /// Qt reports each format this platform can't encode, which the recorder moves past.
    void _ignoreEncoderFallbacks();

private slots:
    void _testRecordingFormat_data();
    void _testRecordingFormat();
    void _testRecordingFormatTriesCodecFirst();
    void _testRecordingCodecs();
    void _testContainers();
    void _testRecordsFrameInput();
    void _testQueuedRetryHonorsStop();
    void _testQueuedRetryYieldsToNewStart();
};
