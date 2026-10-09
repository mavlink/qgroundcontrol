#pragma once

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "NTRIPError.h"
#include "RTCMFramer.h"

class NTRIPTransport : public QObject
{
    Q_OBJECT

public:
    explicit NTRIPTransport(QObject* parent = nullptr)
        : QObject(parent)
    {}

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void sendNMEA(const QByteArray& nmea) = 0;

    /// Live-apply the RTCM whitelist without tearing down the connection.
    virtual void setRtcmWhitelist(const QVector<int>& messageIds) = 0;

signals:
    void connected();
    void error(const NTRIPFailure& failure);
    /// A valid frame of a whitelisted message.
    void correctionFrameReceived(const RTCMDecodedFrame& frame);

    /// A self-signed caster certificate was trusted on first use; the owner persists the pin.
    void certificatePinned(const QString& pin);
};
