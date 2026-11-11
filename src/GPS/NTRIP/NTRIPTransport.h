#pragma once

#include <QtCore/QObject>

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

signals:
    void connected();
    void error(const NTRIPFailure& failure);
    /// A valid frame of the stream.
    void correctionFrameReceived(const RTCMDecodedFrame& frame);

    /// A self-signed caster certificate was trusted on first use; the owner persists the pin.
    void certificatePinned(const QString& pin);
};
