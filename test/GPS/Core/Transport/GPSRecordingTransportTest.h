#pragma once

#include "UnitTest.h"

/// The session recorder writes what crossed the link in order, in a file that replays to the same decoded events, and
/// keeps the link working when the file fails or reaches its size cap.
class GPSRecordingTransportTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _recordsLinkTrafficInOrder();
    void _recordingReplaysToSameEvents();
    void _fileErrorKeepsLink_data();
    void _fileErrorKeepsLink();
    void _fileStopsAtSizeCap();
};
