// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#pragma once

#include "UnitTest.h"

// Host test for AndroidSerialRxQueue — the pure RX flow-control accounting (backlog/epoch/warn latches).
class AndroidSerialRxQueueTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _reserveWithinCap_acceptsAndTracksPending();
    void _reserveOverCap_dropsAndWarnsOnce();
    void _releaseReservation_balancesPendingAndResetsWarn();
    void _isStale_afterFlush_dropsOldGeneration();
    void _checkBufferCap_warnsOnceThenResetsOnAccept();
    void _flush_bumpsGenerationAndZeroesPending();
};
