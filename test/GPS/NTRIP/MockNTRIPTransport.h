#pragma once

#include "NTRIPTransport.h"
#include "NTRIPError.h"

#include <QtCore/QByteArray>
#include <QtCore/QVector>

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
        emit finished();
    }

    void sendNMEA(const QByteArray& nmea) override
    {
        sentNmea.append(nmea);
    }

    // --- Test control ---

    void simulateConnect()
    {
        emit connected();
    }

    void simulateError(NTRIPError code, const QString& detail)
    {
        emit error(code, detail);
    }

    void simulateRtcmData(const QByteArray& data)
    {
        emit RTCMDataUpdate(data);
    }

    void simulateDisconnect()
    {
        emit finished();
    }

    // --- Test inspection ---

    bool isStarted() const { return _started; }
    bool isStopped() const { return _stopped; }

    bool autoConnect = true;
    int startCount = 0;
    int stopCount = 0;
    QVector<QByteArray> sentNmea;

private:
    bool _started = false;
    bool _stopped = false;
};
