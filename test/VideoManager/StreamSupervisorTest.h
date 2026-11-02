#pragma once

#include "UnitTest.h"

/// The reconnect policy both video receivers share, without a stream.
class StreamSupervisorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testBackoffSequence();
    void _testBackoffRestartsAfterRecovery();
    void _testStartOverResetsBackoffAndReconnects();
    void _testHaltsWithoutAutoReconnect_data();
    void _testHaltsWithoutAutoReconnect();
    void _testAutoReconnectChange();
    void _testStates();
    void _testReconnectDue();
};
