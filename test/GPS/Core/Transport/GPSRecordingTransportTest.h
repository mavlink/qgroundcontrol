#pragma once

#include "UnitTest.h"

/// The session recorder writes what crossed the link in order, in a file that replays to the same decoded events, and
/// keeps the link working when the file fails or reaches its size cap. The reader of that file returns both
/// directions with their times and stops at damage, as the replay of the file does.
class GPSRecordingTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _recordsLinkTrafficInOrder();
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
