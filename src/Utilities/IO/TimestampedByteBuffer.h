#pragma once
#include <QtCore/QByteArray>
#include <QtCore/QMutex>

#include <deque>

#include "ReadTimestamp.h"

/// Thread-safe byte queue retaining receipt timestamps and reporting discarded input.
class TimestampedByteBuffer
{
public:
    /// Returns whether the consumer needs a notification, not whether input was accepted.
    /// Nonempty input is always appended (retaining its newest bytes if oversized).
    /// Only the transition from empty/no-gap to readable needs a notification;
    /// the consumer must drain reads, including a pending gap, before waiting again.
    bool append(const QByteArray& bytes, quint64 receivedAtUs = ReadTimestamp::nowUs());

    struct ReadResult
    {
        qint64 bytes = 0;
        quint64 receivedAtUs = 0;
        bool gap = false;
    };

    /// Reads at most one chunk, retaining its receipt time across partial reads.
    /// Overflow, future timestamps, or age >= maximumAgeUs cause one zero-byte gap result
    /// before retained data. Times must use the same monotonic clock. Empty/invalid reads return {}.
    /// Callers supply any protocol-specific resynchronization; this queue never manufactures bytes.
    ReadResult read(char* data, qint64 length, quint64 nowUs, quint64 maximumAgeUs);
    bool gapPending() const;
    qint64 size() const;

private:
    mutable QMutex _mutex;

    struct Chunk
    {
        QByteArray bytes;
        quint64 receivedAtUs = 0;
        qsizetype offset = 0;

        qsizetype remaining() const { return bytes.size() - offset; }
    };

    std::deque<Chunk> _chunks;
    qsizetype _size = 0;
    bool _gap = false;
    static constexpr qsizetype kCapacity = 64 * 1024 - 1;
    static constexpr size_t kMaxChunks = 1024;
};
