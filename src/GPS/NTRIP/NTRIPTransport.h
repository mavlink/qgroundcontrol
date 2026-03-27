#pragma once

#include "NTRIPError.h"

#include <QtCore/QObject>

class NTRIPTransport : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void sendNMEA(const QByteArray& nmea) = 0;

signals:
    void connected();
    void error(NTRIPError code, const QString& detail);
    void RTCMDataUpdate(const QByteArray& message);
    void finished();

    /// Emitted when the transport sent authentication credentials over a cleartext
    /// channel (e.g. Basic auth over HTTP, no TLS). Subclasses that never transmit
    /// credentials simply never emit this. Exposed on the base so NTRIPManager does
    /// not need to know the concrete transport type to wire the warning logger.
    void plaintextCredentialsWarning();
};
