#pragma once

#include "GstVideoReceiverTestBase.h"

/// GstVideoReceiver's recordings: finalized files across errors, reconnects, slow or hung storage and teardown.
class GstVideoReceiverRecordingTest : public GstVideoReceiverTestBase
{
    Q_OBJECT

private slots:
#ifdef QGC_GST_STREAMING
    void _testDecoderFailureKeepsRecording();
    void _testReconnectResumesRecordingInNextPart();
    void _testSlowRecordingDoesNotStallDisplay();
    void _testDeactivateFinishesRecording();
    void _testRecordingPlayableWhileInProgress_data();
    void _testRecordingPlayableWhileInProgress();
    void _testSecondRecordingIsNotTruncated();
    void _testRecordingSwitchWithoutDisplay_data();
    void _testRecordingSwitchWithoutDisplay();
    void _testStreamEndClosesRecordingAtOnce();
    void _testRecordingErrorKeepsStreaming();
    void _testFailedRecordingStartLeavesBranchUsable();
    void _testRecordingStartKeepsPipelinePlaying();
    void _testDestroyWhileRecordingFinalizesFile();
    void _testDestroyWithHungRecorderIsBounded();
#endif
};
