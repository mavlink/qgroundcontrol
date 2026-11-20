#pragma once

#include <memory>

#include <QtCore/QTemporaryDir>

#include "GstVideoReceiverTestBase.h"

/// GstVideoReceiver's recordings: finalized files across errors, reconnects, slow or hung storage and teardown.
class GstVideoReceiverRecordingTest : public GstVideoReceiverTestBase
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testDecoderFailureKeepsRecording_data();
    void _testDecoderFailureKeepsRecording();
    void _testUndecodableStreamIsRecorded();
    void _testCodecMp4CannotHoldIsRecordedAsMatroska();
    void _testReconnectResumesRecordingInNextPart();
    void _testLongRecordingContinuesInNextPart();
    void _testFileSwitchRequestsOneKeyframe();
    void _testFramesArrivingTogetherStartNoNewPart();
    void _testRecordingLeavesFileBeforeHeaderSpaceRunsOut();
    void _testRecordingStartsAtKeyframeWithParameterSets();
    void _testRecordingFailsWithoutKeyframeWithParameterSets();
    void _testFormatChangeContinuesRecordingInNextPart_data();
    void _testFormatChangeContinuesRecordingInNextPart();
    void _testSlowRecordingDoesNotStallDisplay();
    void _testStopRecordsFramesStillQueued();
    void _testDeactivateFinishesRecording();
    void _testRecordingPlayableWhileInProgress_data();
    void _testRecordingPlayableWhileInProgress();
    void _testSecondRecordingIsNotTruncated();
    void _testRecordingSwitchWithoutDisplay_data();
    void _testRecordingSwitchWithoutDisplay();
    void _testStreamEndClosesRecordingAtOnce();
    void _testRecordingErrorKeepsStreaming();
    void _testFullStorageNamedInRecordingError_data();
    void _testFullStorageNamedInRecordingError();
    void _testFailedSyncAtEndFailsRecording();
    void _testFailedRecordingStartLeavesBranchUsable();
    void _testRecordingStartKeepsPipelinePlaying();
    void _testDestroyWhileRecordingFinalizesFile();
    void _testDestroyWithHungRecorderIsBounded();

private:
    std::unique_ptr<QTemporaryDir> _tempDir;
};
