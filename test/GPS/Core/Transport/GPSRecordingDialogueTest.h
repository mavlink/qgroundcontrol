#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// A session recording replays as the dialogue it was. The driver, given the receiver's bytes at their recorded
/// times, writes what the recording holds, and a recording that holds something else fails at the record that
/// differs. The recordings are made here, from the session of every golden scenario against its receiver model.
class GPSRecordingDialogueTest : public GPSProtocolTestBase
{
    Q_OBJECT

protected slots:
    void init() override;

private slots:
    void _modelSessionReplays_data();
    void _modelSessionReplays();
    void _changedRecordingFails_data();
    void _changedRecordingFails();
    void _receivedBytesArriveAtRecordedTimes();
    void _writesAreComparedAsOneStream();
    void _departureNamesTheRecord_data();
    void _departureNamesTheRecord();
    void _unusableRecordingIsAnError_data();
    void _unusableRecordingIsAnError();
    void _interruptionEndsTheReplay();
    void _recordedDialoguesReplay();
};
