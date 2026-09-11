#pragma once
#include <QtCore/QByteArray>
#include <QtCore/QMutex>

#include <deque>

#include "ReadTimestamp.h"

/// Bounded mailbox shared by the blocking receiver worker and its Qt decoder stream.
class TimestampedByteBuffer
{
public:
    bool append(const QByteArray& bytes, quint64 receivedAtUs = ReadTimestamp::nowUs());

    struct ReadResult
    {
        qint64 bytes = 0;
        quint64 receivedAtUs = 0;
        bool gap = false;
    };

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
