#pragma once

#include <chrono>
#include <functional>

#include <QtCore/QByteArray>
#include <QtCore/QVector>

#include "MonotonicClock.h"
#include "NTRIPError.h"
#include "NTRIPTransport.h"
#include "RTCMFrame.h"

class MockNTRIPTransport : public NTRIPTransport
{
    Q_OBJECT

public:
    using NTRIPTransport::NTRIPTransport;

    void start() override
    {
        _started = true;
        _stopped = false;
        startCount++;

        if (autoConnect) {
            emit connected();
        }
    }

    void stop() override
    {
        _started = false;
        _stopped = true;
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
        const bool valid = RTCM::isValidFrame(data);
        emit correctionFrameReceived(
            {.data = data,
             .messageId = messageId,
             .receivedAtMs = receivedAtMs,
             .valid = valid,
             .filtered = valid && !lastWhitelist.isEmpty() && !lastWhitelist.contains(messageId)});
    }

    void simulatePlaintextWarning() { emit plaintextCredentialsWarning(); }

    // --- Test inspection ---

    bool isStarted() const { return _started; }

    bool isStopped() const { return _stopped; }

    bool autoConnect = true;
    int startCount = 0;
    int stopCount = 0;
    std::function<void()> onStop;
    QVector<QByteArray> sentNmea;
    QVector<int> lastWhitelist;

private:
    bool _started = false;
    bool _stopped = false;
};
