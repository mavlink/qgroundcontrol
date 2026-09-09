#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QString>

#include <chrono>

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

    static qint64 monotonicNowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }
};
Q_DECLARE_METATYPE(GPSCorrectionFrame)
