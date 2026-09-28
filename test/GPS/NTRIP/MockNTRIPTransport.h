#pragma once

#include <chrono>
#include <functional>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QPointer>
#include <QtCore/QVector>

#include "MonotonicClock.h"
#include "NTRIPError.h"
#include "NTRIPManager.h"
#include "NTRIPTransport.h"
#include "RTCMFramer.h"

class MockNTRIPTransport : public NTRIPTransport
{
    Q_OBJECT

public:
    using NTRIPTransport::NTRIPTransport;

    void start() override
    {
        startCount++;

        if (autoConnect) {
            emit connected();
        }
    }

    void stop() override
    {
        stopCount++;
        const auto callback = onStop;
        if (callback) {
            callback();
        }
    }

    void sendNMEA(const QByteArray& nmea) override { sentNmea.append(nmea); }

    void setRtcmWhitelist(const QVector<int>& messageIds) override { lastWhitelist = messageIds; }

    // --- Test control ---

    void simulateConnect() { emit connected(); }

    void simulateError(NTRIPError code, const QString& detail, std::chrono::milliseconds retryAfter = {})
    {
        emit error(NTRIPFailure{code, detail, retryAfter});
    }

    void simulateRtcmData(const QByteArray& data, int messageId = 0,
                          qint64 receivedAtMs = static_cast<qint64>(MonotonicClock::nowUs() / 1000))
    {
        const bool valid = RTCMFramer::isValidFrame(data);
        emit correctionFrameReceived(
            {.data = data,
             .messageId = messageId,
             .receivedAtMs = receivedAtMs,
             .valid = valid,
             .filtered = valid && !lastWhitelist.isEmpty() && !lastWhitelist.contains(messageId)});
    }

    void simulatePlaintextWarning() { emit plaintextCredentialsWarning(); }

    bool autoConnect = true;
    int startCount = 0;
    int stopCount = 0;
    std::function<void()> onStop;
    QVector<QByteArray> sentNmea;
    QVector<int> lastWhitelist;
};

/// Makes @a manager open @a transport on its next connection attempt, then NTRIPHttpTransport again.
inline void injectNextTransport(NTRIPManager& manager, NTRIPTransport* transport)
{
    manager.setTransportFactory(
        [next = QPointer<NTRIPTransport>(transport)](const NTRIPManager::Configuration&, QObject*) mutable {
            return std::exchange(next, {}).data();
        });
}
