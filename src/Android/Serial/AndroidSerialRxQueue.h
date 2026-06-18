#pragma once

// Pure RX flow-control accounting for AndroidSerialPort's lossy receive path. Holds no QObject/JNI/
// QIODevice state, so it builds and unit-tests on the host. The Android port keeps the actual byte
// storage (QIODevicePrivate::buffer) and emits readyRead(); this class only owns the policy:
//   * a backlog reservation bounding queued-but-not-yet-drained JNI payloads against the RX cap,
//   * an RX generation/epoch so a clear(Input)/close() drops already-posted payloads as stale,
//   * warn-once latches for the queue-backlog and buffer overflow drop messages.
//
// Threading: reserve()/isStale()/releaseReservation()/flush() touch only atomics and are safe from the
// JNI thread; checkBufferCap() mutates the owner-thread-only buffer-cap latch and must run on the owner.

#include <QtCore/QtTypes>

#include <atomic>

#include "QGCSerialPortTypes.h"

class AndroidSerialRxQueue
{
public:
    explicit AndroidSerialRxQueue(qint64 capBytes = kSerialRxBufferCapBytes) : _cap(capBytes) {}

    struct Reservation
    {
        bool accepted = false;     // false: over the backlog cap, caller drops the payload
        bool shouldWarn = false;   // first drop of an overflow episode — caller logs once
        quint32 generation = 0;    // epoch to stamp on the payload (only meaningful when accepted)
    };

    // JNI thread: reserve backlog for `size` bytes. Stamps the current generation so a flush() between
    // here and the matching append() invalidates the payload.
    Reservation reserve(qint64 size);

    // Owner thread: did `generation` predate a flush()?
    bool isStale(quint32 generation) const { return generation != _generation.load(std::memory_order_acquire); }

    // Owner thread: release a reservation taken by reserve(), whether or not the payload was kept.
    void releaseReservation(qint64 size);

    struct BufferCapDecision
    {
        bool overCap = false;
        bool shouldWarn = false;   // first over-cap drop of an episode
    };

    // Owner thread: may `size` bytes append to a buffer currently holding `currentBuffered`? A within-cap
    // result clears the warn-once latch so the next overflow logs again.
    BufferCapDecision checkBufferCap(qint64 currentBuffered, qint64 size);

    // clear(Input)/close()/open(): bump the epoch and reset the backlog so already-posted payloads drop.
    void flush();

    qint64 cap() const { return _cap; }
    qint64 pendingBytes() const { return _pendingRxBytes.load(std::memory_order_acquire); }
    quint32 generation() const { return _generation.load(std::memory_order_acquire); }

private:
    const qint64 _cap;
    std::atomic<qint64> _pendingRxBytes{0};
    std::atomic<bool> _queueCapWarned{false};
    std::atomic<quint32> _generation{0};
    bool _bufferCapWarned = false;  // owner-thread only
};
