#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

#include <chrono>

// Unknown retains the legacy unclassified input and selects all sources in the router.
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

    static qint64 monotonicNowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }
};
Q_DECLARE_METATYPE(GPSCorrectionFrame)
