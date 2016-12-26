/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

/// @file
///     @brief TCP link type for SITL support
///
///     @author Don Gagne <don@thegagnes.com>

TCPLink::TCPLink(SharedLinkConfigurationPointer& config)
    : LinkInterface(config)
    , _tcpConfig(qobject_cast<TCPConfiguration*>(config.data()))
    , _socket(NULL)
    , _socketIsConnected(false)
{
    Q_ASSERT(_tcpConfig);
    moveToThread(this);
}

TCPLink::~TCPLink()
{
    _disconnect();
    // Tell the thread to exit
    quit();
    // Wait for it to exit
    wait();
}

void TCPLink::run()
{
    _hardwareConnect();
    exec();
}

#ifdef TCPLINK_READWRITE_DEBUG
void TCPLink::_writeDebugBytes(const QByteArray data)
{
    QString bytes;
    QString ascii;
    for (int i=0, size = data.size(); i<size; i++)
    {
        unsigned char v = data[i];
        bytes.append(QString().sprintf("%02x ", v));
        if (data[i] > 31 && data[i] < 127)
        {
            ascii.append(data[i]);
        }
        else
        {
            ascii.append(219);
        }
    }
    qDebug() << "Sent" << size << "bytes to" << _tcpConfig->address().toString() << ":" << _tcpConfig->port() << "data:";
    qDebug() << bytes;
    qDebug() << "ASCII:" << ascii;
}
#endif

void TCPLink::_writeBytes(const QByteArray data)
{
#ifdef TCPLINK_READWRITE_DEBUG
    _writeDebugBytes(data);
#endif
    if (!_socket)
        return;

    _socket->write(data);
    _logOutputDataRate(data.size(), QDateTime::currentMSecsSinceEpoch());
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void TCPLink::readBytes()
{
    qint64 byteCount = _socket->bytesAvailable();
    if (byteCount)
    {
        QByteArray buffer;
        buffer.resize(byteCount);
        _socket->read(buffer.data(), buffer.size());
        emit bytesReceived(this, buffer);
        _logInputDataRate(byteCount, QDateTime::currentMSecsSinceEpoch());
#ifdef TCPLINK_READWRITE_DEBUG
        writeDebugBytes(buffer.data(), buffer.size());
#endif
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
void TCPLink::_disconnect(void)
{
    quit();
    wait();
    if (_socket) {
        _socketIsConnected = false;
        _socket->deleteLater(); // Make sure delete happens on correct thread
        _socket = NULL;
        emit disconnected();
    }
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool TCPLink::_connect(void)
{
    if (isRunning())
    {
        quit();
        wait();
    }
    start(HighPriority);
    return true;
}

bool TCPLink::_hardwareConnect()
{
    Q_ASSERT(_socket == NULL);
    _socket = new QTcpSocket();

    QSignalSpy errorSpy(_socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error));
    _socket->connectToHost(_tcpConfig->address(), _tcpConfig->port());
    QObject::connect(_socket, &QTcpSocket::readyRead, this, &TCPLink::readBytes);

    QObject::connect(_socket,static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                     this, &TCPLink::_socketError);

    // Give the socket a second to connect to the other side otherwise error out
    if (!_socket->waitForConnected(1000))
    {
        // Whether a failed connection emits an error signal or not is platform specific.
        // So in cases where it is not emitted, we emit one ourselves.
        if (errorSpy.count() == 0) {
            emit communicationError(tr("Link Error"), QString("Error on link %1. Connection failed").arg(getName()));
        }
        delete _socket;
        _socket = NULL;
        return false;
    }
    _socketIsConnected = true;
    emit connected();
    return true;
}

void TCPLink::_socketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit communicationError(tr("Link Error"), QString("Error on link %1. Error on socket: %2.").arg(getName()).arg(_socket->errorString()));
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

QString TCPLink::getName() const
{
    return _tcpConfig->name();
}

qint64 TCPLink::getConnectionSpeed() const
{
    return 54000000; // 54 Mbit
}

qint64 TCPLink::getCurrentInDataRate() const
{
    return 0;
}

qint64 TCPLink::getCurrentOutDataRate() const
{
    return 0;
}

void TCPLink::waitForBytesWritten(int msecs)
{
    Q_ASSERT(_socket);
    _socket->waitForBytesWritten(msecs);
}

void TCPLink::waitForReadyRead(int msecs)
{
    Q_ASSERT(_socket);
    _socket->waitForReadyRead(msecs);
}

void TCPLink::_restartConnection()
{
    if(this->isConnected())
    {
        _disconnect();
        _connect();
    }
}

//--------------------------------------------------------------------------
//-- TCPConfiguration

static bool is_ip(const QString& address)
{
    int a,b,c,d;
    if (sscanf(address.toStdString().c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4
            && strcmp("::1", address.toStdString().c_str())) {
        return false;
    } else {
        return true;
    }
}

static QString get_ip_address(const QString& address)
{
    if(is_ip(address))
        return address;
    // Need to look it up
    QHostInfo info = QHostInfo::fromName(address);
    if (info.error() == QHostInfo::NoError)
    {
        QList<QHostAddress> hostAddresses = info.addresses();
        QHostAddress address;
        for (int i = 0; i < hostAddresses.size(); i++)
        {
            // Exclude all IPv6 addresses
            if (!hostAddresses.at(i).toString().contains(":"))
            {
                return hostAddresses.at(i).toString();
            }
        }
    }
    return QString("");
}

TCPConfiguration::TCPConfiguration(const QString& name) : LinkConfiguration(name)
{
    _port    = QGC_TCP_PORT;
    _address = QHostAddress::Any;
}

TCPConfiguration::TCPConfiguration(TCPConfiguration* source) : LinkConfiguration(source)
{
    _port    = source->port();
    _address = source->address();
}

void TCPConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    TCPConfiguration* usource = dynamic_cast<TCPConfiguration*>(source);
    Q_ASSERT(usource != NULL);
    _port    = usource->port();
    _address = usource->address();
}

void TCPConfiguration::setPort(quint16 port)
{
    _port = port;
}

void TCPConfiguration::setAddress(const QHostAddress& address)
{
    _address = address;
}

void TCPConfiguration::setHost(const QString host)
{
    QString ipAdd = get_ip_address(host);
    if(ipAdd.isEmpty()) {
        qWarning() << "TCP:" << "Could not resolve host:" << host;
    } else {
        _address = ipAdd;
    }
}

void TCPConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue("port", (int)_port);
    settings.setValue("host", address().toString());
    settings.endGroup();
}

void TCPConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    _port = (quint16)settings.value("port", QGC_TCP_PORT).toUInt();
    QString address = settings.value("host", _address.toString()).toString();
    _address = address;
    settings.endGroup();
}

void TCPConfiguration::updateSettings()
{
    if(_link) {
        TCPLink* ulink = dynamic_cast<TCPLink*>(_link);
        if(ulink) {
            ulink->_restartConnection();
        }
    }
}
