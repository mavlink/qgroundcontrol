#pragma once

#include <QtCore/QIODevice>

#include "GPSObservation.h"

/// Byte streams preserve the producer receipt time across buffering and decoding.
class GPSReadTimestamp
{
public:
    virtual ~GPSReadTimestamp() = default;
    virtual quint64 lastReadTimestampUs() const = 0;

    static quint64 from(const QIODevice* device)
    {
        const auto* timestamped = dynamic_cast<const GPSReadTimestamp*>(device);
        return timestamped ? timestamped->lastReadTimestampUs() : GPSObservation::monotonicNowUs();
    }
};
