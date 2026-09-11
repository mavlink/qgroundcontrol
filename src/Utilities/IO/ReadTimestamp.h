#pragma once

#include <QtCore/QIODevice>

#include <chrono>

/// Byte streams preserve the producer receipt time across buffering and decoding.
class ReadTimestamp
{
public:
    static quint64 nowUs()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }

    virtual ~ReadTimestamp() = default;
    virtual quint64 lastReadTimestampUs() const = 0;

    static quint64 from(const QIODevice* device)
    {
        const auto* timestamped = dynamic_cast<const ReadTimestamp*>(device);
        return timestamped ? timestamped->lastReadTimestampUs() : nowUs();
    }
};
