#pragma once

#include <chrono>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QString>

#include "MonotonicClock.h"

// Unknown identifies legacy unclassified input; routing policy is selected separately.
namespace GPSCorrectionSources {
Q_NAMESPACE

enum class GPSCorrectionSource
{
    Unknown,
    LocalReceiver,
    NTRIP,
    Udp
};
Q_ENUM_NS(GPSCorrectionSource)

}  // namespace GPSCorrectionSources

using GPSCorrectionSource = GPSCorrectionSources::GPSCorrectionSource;

struct GPSCorrectionFrame
{
    GPSCorrectionSource source = GPSCorrectionSource::Unknown;
    quint64 session = 0;
    qint64 receivedAtMs = 0;
    QByteArray data;
    int messageId = 0;
    bool validated = false;
    bool filtered = false;
    QString sourceInstance = {};

    static qint64 monotonicNowMs() { return static_cast<qint64>(MonotonicClock::nowUs() / 1000); }

    /// Empty for a missing or future receipt.
    static std::optional<std::chrono::milliseconds> age(qint64 receivedAtMs, qint64 nowMs)
    {
        if (receivedAtMs <= 0 || nowMs < receivedAtMs) {
            return std::nullopt;
        }
        return std::chrono::milliseconds(nowMs - receivedAtMs);
    }
};
