#pragma once

#include "UnitTest.h"

class QtMultimediaVideoReceiver;

/// QtMultimediaVideoReceiver without a stream: what it plays for a URI, and what it does with a recording that has
/// no picture yet. QtMultimediaVideoReceiverStreamTest and VideoReceiverContractTest play streams into it.
class QtMultimediaVideoReceiverTest : public UnitTest
{
    Q_OBJECT

public:
    /// Has @p receiver sample its stream five times a second rather than once: the tests wait for its statistics,
    /// which take two samples, and for the timeout the samples check.
    static void sampleOften(QtMultimediaVideoReceiver& receiver);

private:
    void _ignorePlayerLogs();
    void _ignoreEncoderFallbacks();

private slots:
    void _testUnplayableUri();
    void _testRecordingNeedsActiveStream();
    void _testStartWhileRecordingIsRefused();
    void _testClosingEndsRecordingNotStarted();
    void _testRecordingUsesEncodingSet();
    void _testRecordingTimes_data();
    void _testRecordingTimes();
};
