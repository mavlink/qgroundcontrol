#pragma once

#include "UnitTest.h"

/// The session recorder writes what crossed the link and what the link refused in order, in a file that replays to
/// the same decoded events, and keeps the link working when the file fails or reaches its size cap. The reader of that
/// file returns both directions and the refusals with their times and stops at damage, as the replay of the file
/// does. A recorder that was given its session writes the session's settings first, on the session's clock, and the
/// reader returns them as they were.
class GPSRecordingTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _recordsLinkTrafficInOrder();
    void _recordsWhatLinkRefused_data();
    void _recordsWhatLinkRefused();
    void _recordsWriteGivenUpAtItsDeadline();
    void _recordsSessionSettings_data();
    void _recordsSessionSettings();
    void _recordingReplaysToSameEvents();
    void _readerReturnsBothDirectionsWithTimes();
    void _readerStopsAtDamage_data();
    void _readerStopsAtDamage();
    void _replayEndsAtDamage();
    void _replayKeepsRecordedTimes();
    void _fileErrorKeepsLink_data();
    void _fileErrorKeepsLink();
    void _fileStopsAtSizeCap();
};
