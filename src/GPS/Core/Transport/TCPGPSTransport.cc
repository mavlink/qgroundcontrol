#include "TCPGPSTransport.h"

#include <utility>

#include <QtCore/QCoreApplication>
#include <QtNetwork/QTcpSocket>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(TCPGPSTransportLog, "GPS.Transport.TCPGPSTransport")

namespace {
GPSOpenResult socketOpenFailure(const GPSTransport& transport, QAbstractSocket& socket, QDeadlineTimer deadline)
{
    const bool timedOut = deadline.hasExpired();
    const GPSOpenResult result{
        transport.isCancelled() ? GPSOpenStatus::Cancelled
        : timedOut              ? GPSOpenStatus::TimedOut
                                : GPSOpenStatus::Error,
        timedOut ? QCoreApplication::translate("GPSTransport", "Receiver connection timed out") : socket.errorString()};
    socket.abort();
    return result;
}
}  // namespace

TCPGPSTransport::TCPGPSTransport(QString host, quint16 port, GPSCancelToken cancelToken)
    : GPSDeviceTransport(std::move(cancelToken))
    , _host(std::move(host))
    , _port(port)
{}

TCPGPSTransport::~TCPGPSTransport() = default;

GPSOpenResult TCPGPSTransport::open()
{
    if (isCancelled()) {
        return {GPSOpenStatus::Cancelled};
    }
    const QDeadlineTimer connectDeadline(CONNECT_TIMEOUT);
    _socket = std::make_unique<QTcpSocket>();
    _socket->setReadBufferSize(READ_BUFFER_BYTES);
    _socket->connectToHost(_host, _port);
    if (waitForDevice([this]() { return _socket->state() == QAbstractSocket::ConnectedState; }, connectDeadline)) {
        return {GPSOpenStatus::Opened};
    }
    if (!isCancelled()) {
        qCWarning(TCPGPSTransportLog) << "Failed to connect to GPS receiver" << _host << _port
                                      << _socket->errorString();
    }
    return socketOpenFailure(*this, *_socket, connectDeadline);
}

bool TCPGPSTransport::fatalError() const
{
    return !_socket || _socket->state() == QAbstractSocket::UnconnectedState;
}

QIODevice* TCPGPSTransport::device() const
{
    return _socket.get();
}

std::chrono::milliseconds TCPGPSTransport::configurationWriteTimeout() const
{
    return WRITE_TIMEOUT;
}

void TCPGPSTransport::retire()
{
    _socket->abort();
}
