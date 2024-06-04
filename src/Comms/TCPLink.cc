/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TCPLink.h"
#include "DeviceInfo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>

QGC_LOGGING_CATEGORY(TCPLinkLog, "qgc.comms.tcplink")

namespace {
    constexpr int SEND_BUFFER_SIZE = 1024; // >= MAVLINK_MAX_PACKET_LEN
    constexpr int RECEIVE_BUFFER_SIZE = 1024; // >= MAVLINK_MAX_PACKET_LEN
    constexpr int READ_BUFFER_SIZE = 1024; // >= MAVLINK_MAX_PACKET_LEN
    constexpr int CONNECT_TIMEOUT_MS = 1000;
    constexpr int TYPE_OF_SERVICE = 32; // Optional: Set ToS for low delay
    constexpr int MAX_RECONNECTION_ATTEMPTS = 5;
}

TCPLink::TCPLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _tcpConfig(qobject_cast<const TCPConfiguration*>(config.get()))
    , _socket(new QTcpSocket())
{
    // qCDebug(TCPLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(_tcpConfig);
    if (!_tcpConfig) {
        qCWarning(TCPLinkLog) << "Invalid TCPConfiguration provided.";
        emit communicationError(
            tr("Configuration Error"),
            tr("Link %1: Invalid TCP configuration.").arg(config->name())
        );
        return;
    }

    _socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, SEND_BUFFER_SIZE);
    _socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, RECEIVE_BUFFER_SIZE);
    _socket->setReadBufferSize(READ_BUFFER_SIZE);
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    // _socket->setSocketOption(QAbstractSocket::TypeOfServiceOption, TYPE_OF_SERVICE);

    (void) QObject::connect(_socket, &QTcpSocket::connected, this, [this]() {
        _isConnected = true;
        _reconnectionAttempts = 0;
        qCDebug(TCPLinkLog) << "TCP connected to" << _tcpConfig->host() << ":" << _tcpConfig->port();
        emit connected();
    }, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QTcpSocket::disconnected, this, [this]() {
        _isConnected = false;
        qCDebug(TCPLinkLog) << "TCP disconnected from" << _tcpConfig->host() << ":" << _tcpConfig->port();
        emit disconnected();
        // TODO: Uncomment after threading changes
        // _attemptReconnection();
    }, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QTcpSocket::readyRead, this, &TCPLink::_readBytes, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QTcpSocket::errorOccurred, this, [this](QTcpSocket::SocketError error) {
        qCWarning(TCPLinkLog) << "TCP Link Error:" << error << _socket->errorString();
        emit communicationError(
            tr("TCP Link Error"),
            tr("Link %1: %2.").arg(_tcpConfig->name(), _socket->errorString())
        );
    }, Qt::AutoConnection);

#ifdef QT_DEBUG
    (void) QObject::connect(_socket, &QTcpSocket::stateChanged, this, [](QTcpSocket::SocketState state) {
        qCDebug(TCPLinkLog) << "TCP State Changed:" << state;
    }, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QTcpSocket::hostFound, this, []() {
        qCDebug(TCPLinkLog) << "TCP Host Found";
    }, Qt::AutoConnection);
#endif
}

TCPLink::~TCPLink()
{
    if (_socket->isOpen()) {
        _socket->disconnectFromHost();
        if (_socket->state() != QAbstractSocket::UnconnectedState) {
            _socket->waitForDisconnected(CONNECT_TIMEOUT_MS);
        }
    }

    _socket->deleteLater();

    // qCDebug(TCPLinkLog) << Q_FUNC_INFO << this;
}

void TCPLink::disconnect()
{
    if (isConnected()) {
        _socket->disconnectFromHost();
    } else {
        emit disconnected();
    }
}

bool TCPLink::_connect()
{
    if (isConnected()) {
        qCWarning(TCPLinkLog) << "Already connected to" << _tcpConfig->host() << ":" << _tcpConfig->port();
        return true;
    }

    QSignalSpy errorSpy(_socket, &QTcpSocket::errorOccurred);

    qCDebug(TCPLinkLog) << "Attempting to connect to host:" << _tcpConfig->host() << "port:" << _tcpConfig->port();
    _socket->connectToHost(_tcpConfig->host(), _tcpConfig->port());

    // TODO: Switch Blocking to Signals after Threading Changes
    if (!_socket->waitForConnected(CONNECT_TIMEOUT_MS)) {
        qCWarning(TCPLinkLog) << "Connection to" << _tcpConfig->host() << ":" << _tcpConfig->port() << "failed:" << _socket->errorString();
        if (errorSpy.count() == 0) {
            emit communicationError(
                tr("TCP Link Connect Error"),
                tr("Link %1: %2.").arg(_tcpConfig->name(), tr("Connection Failed: %1").arg(_socket->errorString()))
            );
        }
        return false;
    }

    qCDebug(TCPLinkLog) << "Successfully connected to" << _tcpConfig->host() << ":" << _tcpConfig->port();
    return true;
}

void TCPLink::_writeBytes(const QByteArray &bytes)
{
    if (!_socket->isValid()) {
        return;
    }

    static const QString title = tr("TCP Link Write Error");

    if (!isConnected()) {
        emit communicationError(
            title,
            tr("Link %1: Could Not Send Data - Link is Disconnected!").arg(_tcpConfig->name())
        );
        return;
    }

    const qint64 bytesWritten = _socket->write(bytes);
    if (bytesWritten < 0) {
        emit communicationError(
            title,
            tr("Link %1: Could Not Send Data - Write Failed: %2").arg(_tcpConfig->name(), _socket->errorString())
        );
        return;
    }

    if (bytesWritten < bytes.size()) {
        qCWarning(TCPLinkLog) << "Wrote" << bytesWritten << "Out of" << bytes.size() << "total bytes";
        const QByteArray remainingBytes = bytes.mid(bytesWritten);
        writeBytesThreadSafe(remainingBytes.constData(), remainingBytes.size());
    }

    emit bytesSent(this, bytes);
}

void TCPLink::_readBytes()
{
    if (!_socket->isValid()) {
        return;
    }

    if (!isConnected()) {
        emit communicationError(
            tr("TCP Link Read Error"),
            tr("Link %1: Could Not Read Data - Link is Disconnected!").arg(_tcpConfig->name())
        );
        return;
    }

    const QByteArray buffer = _socket->readAll();

    if (buffer.isEmpty()) {
        emit communicationError(
            tr("TCP Link Read Error"),
            tr("Link %1: Could Not Read Data - No Data Available!").arg(_tcpConfig->name())
        );
        return;
    }

    emit bytesReceived(this, buffer);
}

void TCPLink::_attemptReconnection()
{
    if (_reconnectionAttempts >= MAX_RECONNECTION_ATTEMPTS) {
        qCWarning(TCPLinkLog) << "Max reconnection attempts reached for" << _tcpConfig->host() << ":" << _tcpConfig->port();
        emit communicationError(tr("TCP Link Reconnect Error"), tr("Link %1: Maximum reconnection attempts reached.").arg(_tcpConfig->name()));
        return;
    }

    _reconnectionAttempts++;
    const int delay = qPow(2, _reconnectionAttempts) * 1000; // Exponential backoff
    qCDebug(TCPLinkLog) << "Attempting reconnection #" << _reconnectionAttempts << "in" << delay << "ms";
    QTimer::singleShot(delay, this, [this]() {
        _connect();
    });
}

bool TCPLink::isSecureConnection()
{
    return QGCDeviceInfo::isNetworkWired();
}

/*===========================================================================*/

TCPConfiguration::TCPConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    // qCDebug(TCPLinkLog) << Q_FUNC_INFO << this;
}

TCPConfiguration::TCPConfiguration(const TCPConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
    , _host(copy->host())
    , _port(copy->port())
{
    // qCDebug(TCPLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(copy);

    LinkConfiguration::copyFrom(copy);
}

TCPConfiguration::~TCPConfiguration()
{
    // qCDebug(TCPLinkLog) << Q_FUNC_INFO << this;
}

void TCPConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_CHECK_PTR(source);
    LinkConfiguration::copyFrom(source);

    const TCPConfiguration* const tcpSource = qobject_cast<const TCPConfiguration*>(source);
    Q_CHECK_PTR(tcpSource);

    setHost(tcpSource->host());
    setPort(tcpSource->port());
}

void TCPConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setHost(settings.value(QStringLiteral("host"), host()).toString());
    setPort(static_cast<quint16>(settings.value(QStringLiteral("port"), port()).toUInt()));

    settings.endGroup();
}

void TCPConfiguration::saveSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    settings.setValue(QStringLiteral("host"), host());
    settings.setValue(QStringLiteral("port"), port());

    settings.endGroup();
}
