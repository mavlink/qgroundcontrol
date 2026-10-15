#pragma once

#include "UnitTest.h"

/// GstVideoReceiver's components on their own, without a running pipeline.
class GstVideoReceiverComponentsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testReconnectBackoffSequence();
    void _testReconnectBackoffRestartsAfterRecovery();
    void _testStallDetection_data();
    void _testStallDetection();
    void _testSourceTimeoutStartsAtFirstCheck();
    void _testFinalizerSettlesOnlyFromArmedRecording_data();
    void _testFinalizerSettlesOnlyFromArmedRecording();
    void _testFinalizerKeepsFirstOutcome();
};
