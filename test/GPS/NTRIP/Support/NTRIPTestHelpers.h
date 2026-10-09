#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtTest/QSignalSpy>

#include "NTRIPConfiguration.h"
#include "NTRIPError.h"
#include "RTCMFramer.h"

/// The NTRIP suites' loopback caster configuration, HTTP response builders, readers for the NTRIPTransport signal
/// payloads, and exact log expectations.
namespace GPSTest {

/// Plain-TCP configuration for a caster on 127.0.0.1 serving @a mountpoint.
inline NTRIPConnectionConfig connectionConfig(int port = 2101, const QString& mountpoint = QStringLiteral("TEST"))
{
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("127.0.0.1");
    config.port = port;
    config.mountpoint = mountpoint;
    return config;
}

/// A 200 response carrying @a body, delimited by Content-Length.
inline QByteArray okResponse(const QByteArray& body)
{
    return "HTTP/1.1 200 OK\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
}

/// A 200 response carrying @a body as one chunk of the HTTP/1.1 chunked transfer coding, followed by the last chunk
/// when @a complete.
inline QByteArray okChunkedResponse(const QByteArray& body, bool complete = true)
{
    return "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n" + QByteArray::number(body.size(), 16) + "\r\n" +
           body + "\r\n" + (complete ? QByteArray("0\r\n\r\n") : QByteArray());
}

/// The failure of the @a emission-th NTRIPTransport::error() recorded by @a errors.
inline NTRIPFailure failureAt(const QSignalSpy& errors, qsizetype emission = 0)
{
    return qvariant_cast<NTRIPFailure>(errors.at(emission).at(0));
}

/// The frame of the @a emission-th NTRIPTransport::correctionFrameReceived() recorded by @a frames.
inline RTCMDecodedFrame frameAt(const QSignalSpy& frames, qsizetype emission = 0)
{
    return qvariant_cast<RTCMDecodedFrame>(frames.at(emission).at(0));
}

/// A pattern matching exactly @a message, for expectLogMessage().
inline QRegularExpression exactMessage(const QString& message)
{
    return QRegularExpression(QRegularExpression::anchoredPattern(QRegularExpression::escape(message)));
}

}  // namespace GPSTest
