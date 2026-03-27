#pragma once

#include "UnitTest.h"

/// State-transition smoke tests for the NTRIP manager singleton.
///
/// The detailed GGA formatting suite that used to live here has been moved to
/// NTRIPGgaProviderTest — that is where the implementation of makeGGA lives and
/// where future sentence-format cases should go. This file focuses on the
/// NTRIPManager's observable state machine (connectionStatus, casterStatus)
/// rather than downstream sentence encoding.
class NTRIPManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialStateIsDisconnected();
    void testStopFromIdleIsNoop();
};
