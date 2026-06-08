#include "AndroidSerialRxQueue.h"

AndroidSerialRxQueue::Reservation AndroidSerialRxQueue::reserve(qint64 size)
{
    // Stamp the epoch before committing the reservation: a flush() between here and the matching append()
    // then tags this payload with the old generation, so isStale() drops it instead of re-appending
    // pre-flush bytes. Sampled before the CAS so a racing flush can't be missed.
    const quint32 generation = _generation.load(std::memory_order_acquire);

    // CAS the cap check and the reserve into one atomic step so concurrent producers can't both pass the
    // check and over-reserve past the cap (the documented invariant is a single JNI reader thread; this
    // keeps the accounting correct even if that ever changes).
    qint64 pending = _pendingRxBytes.load(std::memory_order_acquire);
    do {
        if ((pending + size) > _cap) {
            Reservation result;
            result.accepted = false;
            result.shouldWarn = !_queueCapWarned.exchange(true, std::memory_order_relaxed);
            return result;
        }
    } while (!_pendingRxBytes.compare_exchange_weak(pending, pending + size, std::memory_order_acq_rel));

    Reservation result;
    result.accepted = true;
    result.generation = generation;
    return result;
}

void AndroidSerialRxQueue::releaseReservation(qint64 size)
{
    const qint64 prev = _pendingRxBytes.fetch_sub(size, std::memory_order_acq_rel);
    // flush() may have zeroed the counter between reserve and here; CAS-clamp to 0 so a concurrent
    // new-session fetch_add isn't erased by a blind store.
    qint64 cur = prev - size;
    while ((cur < 0) && !_pendingRxBytes.compare_exchange_weak(cur, 0, std::memory_order_acq_rel)) { }
    if ((prev - size) <= 0) {
        _queueCapWarned.store(false, std::memory_order_relaxed);
    }
}

AndroidSerialRxQueue::BufferCapDecision AndroidSerialRxQueue::checkBufferCap(qint64 currentBuffered, qint64 size)
{
    BufferCapDecision decision;
    if ((currentBuffered + size) > _cap) {
        decision.overCap = true;
        decision.shouldWarn = !_bufferCapWarned;
        _bufferCapWarned = true;
        return decision;
    }
    _bufferCapWarned = false;
    return decision;
}

void AndroidSerialRxQueue::flush()
{
    _generation.fetch_add(1, std::memory_order_acq_rel);
    _pendingRxBytes.store(0, std::memory_order_release);
    _queueCapWarned.store(false, std::memory_order_relaxed);
}
