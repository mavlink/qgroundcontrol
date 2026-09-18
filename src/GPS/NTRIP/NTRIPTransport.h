#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include "NTRIPError.h"
#include "RTCMFrameDecoder.h"

class NTRIPTransport : public QObject
{
    Q_OBJECT

public:
    explicit NTRIPTransport(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void sendNMEA(const QByteArray& nmea) = 0;

    /// Live-apply the RTCM whitelist without tearing down the connection.
    /// Default no-op for transports that don't filter.
    virtual void setRtcmWhitelist(const QVector<int>& /*messageIds*/) {}

signals:
    void connected();
    void error(const NTRIPFailure& failure);
    /// Includes invalid and filtered candidates.
    void correctionFrameReceived(const RTCMFrameDecoder::Result& frame);

    /// Warns before admitting a plaintext credential write; observers may cancel.
    void plaintextCredentialsWarning();
};
