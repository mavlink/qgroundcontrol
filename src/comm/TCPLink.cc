/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <iostream>
#include "TCPLink.h"
#include "LinkManager.h"
#include "QGC.h"
#include <QHostInfo>
#include <QSignalSpy>

TCPLink::TCPLink(SharedLinkConfigurationPtr& config)
    : LinkInterface(config)
    , _tcpConfig(qobject_cast<TCPConfiguration*>(config.get()))
    , _socket(nullptr)
    , _socketIsConnected(false)
{
    Q_ASSERT(_tcpConfig);
}

TCPLink::~TCPLink()
{
    disconnect();
}

#ifdef TCPLINK_READWRITE_DEBUG
void TCPLink::_writeDebugBytes(const QByteArray data)
{
    QString bytes;
    QString ascii;
    for (int i=0, size = data.size(); i<size; i++)
    {
        unsigned char v = data[i];
        bytes.append(QString::asprintf("%02x ", v));
        if (data[i] > 31 && data[i] < 127)
        {
            ascii.append(data[i]);
        }
        else
        {
            ascii.append(219);
        }
    }
    qDebug() << "Sent" << size << "bytes to" << _tcpConfig->host() << ":" << _tcpConfig->port() << "data:";
    qDebug() << bytes;
    qDebug() << "ASCII:" << ascii;
}
#endif

void TCPLink::_writeBytes(const QByteArray data)
{
#ifdef TCPLINK_READWRITE_DEBUG
    _writeDebugBytes(data);
#endif

    if (_socket) {
        _socket->write(data);
        emit bytesSent(this, data);
    }
}

void TCPLink::_readBytes()
{
    if (_socket) {
        qint64 byteCount = _socket->bytesAvailable();
        if (byteCount)
        {
            QByteArray buffer;
            buffer.resize(byteCount);
            _socket->read(buffer.data(), buffer.size());
            emit bytesReceived(this, buffer);
#ifdef TCPLINK_READWRITE_DEBUG
            writeDebugBytes(buffer.data(), buffer.size());
#endif
        }
    }
}

void TCPLink::disconnect(void)
{
    if (_socket) {
        // This prevents stale signal from calling the link after it has been deleted
        QObject::disconnect(_socket, &QIODevice::readyRead, this, &TCPLink::_readBytes);
        _socketIsConnected = false;
        _socket->disconnectFromHost(); // Disconnect tcp
        _socket->deleteLater(); // Make sure delete happens on correct thread
        _socket = nullptr;
        emit disconnected();
    }
}

bool TCPLink::_connect(void)
{
    if (_socket) {
        qWarning() << "connect called while already connected";
        return true;
    }

    return _hardwareConnect();
}

bool TCPLink::_hardwareConnect()
{
    Q_ASSERT(_socket == nullptr);
    _socket = new QTcpSocket();
    QObject::connect(_socket, &QIODevice::readyRead, this, &TCPLink::_readBytes);

    QSignalSpy errorSpy(_socket, &QAbstractSocket::errorOccurred);
    QObject::connect(_socket, &QAbstractSocket::errorOccurred, this, &TCPLink::_socketError);

    _socket->connectToHost(_tcpConfig->host(), _tcpConfig->port());

    // Give the socket a second to connect to the other side otherwise error out
    if (!_socket->waitForConnected(1000))
    {
        // Whether a failed connection emits an error signal or not is platform specific.
        // So in cases where it is not emitted, we emit one ourselves.
        if (errorSpy.count() == 0) {
            emit communicationError(tr("Link Error"), tr("Error on link %1. Connection failed").arg(_config->name()));
        }
        delete _socket;
        _socket = nullptr;
        return false;
    }
    _socketIsConnected = true;
    emit connected();
    return true;
}

void TCPLink::_socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit communicationError(tr("Link Error"), tr("Error on link %1. Error on socket: %2.").arg(_config->name()).arg(_socket->errorString()));
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool TCPLink::isConnected() const
{
    return _socketIsConnected;
}

//--------------------------------------------------------------------------
//-- TCPConfiguration

TCPConfiguration::TCPConfiguration(const QString& name) : LinkConfiguration(name)
{
    _port    = QGC_TCP_PORT;
    _host    = QLatin1String("0.0.0.0");
}

TCPConfiguration::TCPConfiguration(TCPConfiguration* source) : LinkConfiguration(source)
{
    _port    = source->port();
    _host    = source->host();
}

void TCPConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    auto* usource = qobject_cast<TCPConfiguration*>(source);
    Q_ASSERT(usource != nullptr);
    _port    = usource->port();
    _host = usource->host();
}

void TCPConfiguration::setPort(quint16 port)
{
    _port = port;
}

void TCPConfiguration::setHost(const QString host)
{
    _host = host;
}

void TCPConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue("port", (int)_port);
    settings.setValue("host", _host);
    settings.endGroup();
}

void TCPConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    _port = (quint16)settings.value("port", QGC_TCP_PORT).toUInt();
    _host = settings.value("host", _host).toString();
    settings.endGroup();
}
