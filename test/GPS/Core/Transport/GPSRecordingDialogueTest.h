#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// A session recording replays as the dialogue it was. The driver, given the receiver's bytes at their recorded
/// times and the refusals of the recorded link, writes what the recording holds, and a recording that holds something
/// else fails at the record that differs. The recordings are made here, from the session of every golden scenario
/// against its receiver model, on a link that refuses nothing and on links that refuse a baud rate or every write.
class GPSRecordingDialogueTest : public GPSProtocolTestBase
{
    Q_OBJECT

protected slots:
    void init() override;

private slots:
    void _modelSessionReplays_data();
    void _modelSessionReplays();
    void _refusedSessionReplays_data();
    void _refusedSessionReplays();
    void _changedRecordingFails_data();
    void _changedRecordingFails();
    void _receivedBytesArriveAtRecordedTimes();
    void _writesAreComparedAsOneStream();
    void _refusalsGetTheRecordedAnswer();
    void _departureNamesTheRecord_data();
    void _departureNamesTheRecord();
    void _unusableRecordingIsAnError_data();
    void _unusableRecordingIsAnError();
    void _interruptionEndsTheReplay();
    void _recordedDialoguesReplay();
};
