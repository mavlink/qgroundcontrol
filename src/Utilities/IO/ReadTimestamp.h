#pragma once

#include <QtCore/QIODevice>

#include "MonotonicClock.h"

/// Byte streams preserve the producer receipt time across buffering and decoding.
class ReadTimestamp
{
public:
    static quint64 nowUs() { return MonotonicClock::nowUs(); }

    virtual ~ReadTimestamp() = default;
    virtual quint64 lastReadTimestampUs() const = 0;

    static quint64 from(const QIODevice* device)
    {
        const auto* timestamped = dynamic_cast<const ReadTimestamp*>(device);
        return timestamped ? timestamped->lastReadTimestampUs() : nowUs();
    }
};
