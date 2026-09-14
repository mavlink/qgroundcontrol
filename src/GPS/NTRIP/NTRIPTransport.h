#pragma once

#include <QtCore/QObject>
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
        connect(this, &NTRIPTransport::correctionFrameReceived, this, [this](const RTCMFrameDecoder::Result& frame) {
            if (frame.valid && !frame.filtered) {
                emit RTCMDataUpdate(frame.data, frame.messageId);
            }
        });
    }

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void sendNMEA(const QByteArray& nmea) = 0;

    /// Live-apply the RTCM whitelist without tearing down the connection.
    /// Default no-op for transports that don't filter.
    virtual void setRtcmWhitelist(const QVector<int>& /*messageIds*/) {}

signals:
    void connected();
    void error(NTRIPError code, const QString& detail);
    /// Compatibility projection of valid, unfiltered decoded frames.
    void RTCMDataUpdate(const QByteArray& message, int messageId);
    /// Includes invalid and filtered candidates.
    void correctionFrameReceived(const RTCMFrameDecoder::Result& frame);
    void finished();

    /// Emitted when the transport sent authentication credentials over a cleartext
    /// channel (e.g. Basic auth over HTTP, no TLS). Subclasses that never transmit
    /// credentials simply never emit this. Exposed on the base so NTRIPManager does
    /// not need to know the concrete transport type to wire the warning logger.
    void plaintextCredentialsWarning();
};
