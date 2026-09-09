#pragma once

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "NTRIPError.h"

class NTRIPStream : public QObject
{
    Q_OBJECT

public:
    explicit NTRIPStream(QObject* parent = nullptr);
    ~NTRIPStream() override;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void sendNMEA(const QByteArray& nmea) = 0;

    /// Live-apply the RTCM whitelist without tearing down the connection.
    /// Default no-op for transports that don't filter.
    virtual void setRtcmWhitelist(const QVector<int>& /*messageIds*/) {}

signals:
    void connected();
    void correctionReceivedAt(const QByteArray& data, int messageId, bool filtered, qint64 receivedAtMs);
    void correctionRejectedAt(const QByteArray& data, int messageId, qint64 receivedAtMs);
    void error(NTRIPError code, const QString& detail);
    void failed(const NTRIPFailure& failure);
    void bytesReceived(qint64 bytes);
    void finished();

    /// Emitted when the transport sent authentication credentials over a cleartext
    /// channel (e.g. Basic auth over HTTP, no TLS). Subclasses that never transmit
    /// credentials simply never emit this. Exposed on the base so NTRIPManager does
    /// not need to know the concrete transport type to wire the warning logger.
    void plaintextCredentialsWarning();
};
