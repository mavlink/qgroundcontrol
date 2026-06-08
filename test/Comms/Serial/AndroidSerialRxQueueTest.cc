// SPDX-License-Identifier: Apache-2.0 OR GPL-3.0-only

#include "AndroidSerialRxQueueTest.h"

#include "AndroidSerialRxQueue.h"

#include <QtTest/QTest>

UT_REGISTER_TEST(AndroidSerialRxQueueTest, TestLabel::Unit, TestLabel::Comms)

void AndroidSerialRxQueueTest::_reserveWithinCap_acceptsAndTracksPending()
{
    AndroidSerialRxQueue queue(100);

    const AndroidSerialRxQueue::Reservation reservation = queue.reserve(40);
    QVERIFY(reservation.accepted);
    QVERIFY(!reservation.shouldWarn);
    QCOMPARE(reservation.generation, 0u);
    QCOMPARE(queue.pendingBytes(), 40);
}

void AndroidSerialRxQueueTest::_reserveOverCap_dropsAndWarnsOnce()
{
    AndroidSerialRxQueue queue(100);

    QVERIFY(queue.reserve(80).accepted);

    const AndroidSerialRxQueue::Reservation first = queue.reserve(80);
    QVERIFY(!first.accepted);
    QVERIFY(first.shouldWarn);

    const AndroidSerialRxQueue::Reservation second = queue.reserve(80);
    QVERIFY(!second.accepted);
    QVERIFY(!second.shouldWarn);

    QCOMPARE(queue.pendingBytes(), 80);
}

void AndroidSerialRxQueueTest::_releaseReservation_balancesPendingAndResetsWarn()
{
    AndroidSerialRxQueue queue(100);

    QVERIFY(queue.reserve(80).accepted);
    QVERIFY(queue.reserve(80).shouldWarn);  // latches the queue-cap warning

    queue.releaseReservation(80);
    QCOMPARE(queue.pendingBytes(), 0);

    // Draining to zero re-arms the warn-once latch, so the next overflow logs again.
    QVERIFY(queue.reserve(80).accepted);
    QVERIFY(queue.reserve(80).shouldWarn);
}

void AndroidSerialRxQueueTest::_isStale_afterFlush_dropsOldGeneration()
{
    AndroidSerialRxQueue queue;

    const AndroidSerialRxQueue::Reservation before = queue.reserve(10);
    QVERIFY(before.accepted);
    QVERIFY(!queue.isStale(before.generation));

    queue.flush();
    QVERIFY(queue.isStale(before.generation));

    const AndroidSerialRxQueue::Reservation after = queue.reserve(10);
    QVERIFY(!queue.isStale(after.generation));
}

void AndroidSerialRxQueueTest::_checkBufferCap_warnsOnceThenResetsOnAccept()
{
    AndroidSerialRxQueue queue(100);

    const AndroidSerialRxQueue::BufferCapDecision over1 = queue.checkBufferCap(90, 20);
    QVERIFY(over1.overCap);
    QVERIFY(over1.shouldWarn);

    const AndroidSerialRxQueue::BufferCapDecision over2 = queue.checkBufferCap(90, 20);
    QVERIFY(over2.overCap);
    QVERIFY(!over2.shouldWarn);

    const AndroidSerialRxQueue::BufferCapDecision within = queue.checkBufferCap(10, 20);
    QVERIFY(!within.overCap);

    // Accepting (within cap) re-arms the buffer-cap warning.
    const AndroidSerialRxQueue::BufferCapDecision over3 = queue.checkBufferCap(90, 20);
    QVERIFY(over3.overCap);
    QVERIFY(over3.shouldWarn);
}

void AndroidSerialRxQueueTest::_flush_bumpsGenerationAndZeroesPending()
{
    AndroidSerialRxQueue queue;

    QVERIFY(queue.reserve(50).accepted);
    const quint32 generation = queue.generation();

    queue.flush();

    QCOMPARE(queue.pendingBytes(), 0);
    QCOMPARE(queue.generation(), generation + 1);
}
