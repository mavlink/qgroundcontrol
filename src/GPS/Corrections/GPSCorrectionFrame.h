#pragma once

#include <limits>

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QString>

#include "MonotonicClock.h"

// Unknown identifies legacy unclassified input; routing policy is selected separately.
enum class GPSCorrectionSource
{
    Unknown,
    LocalReceiver,
    Ntrip,
    Udp
};
Q_DECLARE_METATYPE(GPSCorrectionSource)

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
    quint64 deliveryId = 0;

    static qint64 monotonicNowMs() { return static_cast<qint64>(MonotonicClock::nowUs() / 1000); }

    static qint64 ageMs(qint64 receivedAtMs, qint64 nowMs)
    {
        if (receivedAtMs <= 0 || nowMs < receivedAtMs ||
            quint64(nowMs) > (std::numeric_limits<quint64>::max)() / 1000) {
            return -1;
        }
        return MonotonicClock::ageMilliseconds(quint64(receivedAtMs) * 1000, quint64(nowMs) * 1000);
    }
};
Q_DECLARE_METATYPE(GPSCorrectionFrame)
