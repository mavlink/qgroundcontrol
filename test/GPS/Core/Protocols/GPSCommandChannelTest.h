#pragma once

#include "Protocols/Support/GPSProtocolTestBase.h"

/// The command channel: scripted command/reply runs, sequence attempts, write results and invalid reads.
class GPSCommandChannelTest : public GPSProtocolTestBase
{
    Q_OBJECT

private slots:
    void _channel_data();
    void _channel();
    void _sequenceAttempts_data();
    void _sequenceAttempts();
    void _sequenceEndsOnWriteFailure();
    void _writeResults_data();
    void _writeResults();
    void _invalidReadResults_data();
    void _invalidReadResults();
};
