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

#include <QtNetwork/QTcpSocket>

QGC_LOGGING_CATEGORY(TCPLinkLog, "TEST.comms.tcplink")

// MAVLINK_COMM_NUM_BUFFERS

TCPLink::TCPLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _tcpConfig(qobject_cast<const TCPConfiguration*>(config.get()))
    , _socket(new QTcpSocket())
{
    // qCDebug(TCPLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(_tcpConfig);

    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    // _socket->setSocketOption(QAbstractSocket::TypeOfServiceOption, 32);
    // MAVLINK_MAX_PACKET_LEN
    _socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024);
    _socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024);
    // _socket->setReadBufferSize(1024);

    (void) QObject::connect(_socket, &QTcpSocket::connected, this, &TCPLink::connected, Qt::AutoConnection);
    (void) QObject::connect(_socket, &QTcpSocket::disconnected, this, &TCPLink::disconnected, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QTcpSocket::errorOccurred, this, [this](QTcpSocket::SocketError error) {
        qCWarning(TCPLinkLog) << "TCP Link Error:" << error << _socket->errorString();
        emit communicationError(QStringLiteral("TCP Link Error"), QStringLiteral("Link: %1, %2.").arg(_tcpConfig->name(), _socket->errorString()));
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
    _socket->deleteLater();

    // qCDebug(TCPLinkLog) << Q_FUNC_INFO << this;
}

bool TCPLink::isConnected() const
{
    return ((_socket->state() == QAbstractSocket::ConnectedState) || (_socket->state() == QAbstractSocket::ConnectingState));
}

void TCPLink::disconnect()
{
    (void) QObject::disconnect(_socket, &QTcpSocket::readyRead, this, &TCPLink::_readBytes);

    _socket->disconnectFromHost();
}

bool TCPLink::_connect()
{
    if (isConnected()) {
        return true;
    }

    static const QString title = tr("TCP Link Connect Error");
    static const QString error = tr("Link %1: %2.");

    (void) QObject::connect(_socket, &QTcpSocket::readyRead, this, &TCPLink::_readBytes, Qt::AutoConnection);
    // (void) connect(_socket, &QTcpSocket::bytesWritten, this, [this](qint64 written) {});

    _socket->connectToHost(_tcpConfig->host(), _tcpConfig->port());

    if (!_socket->isValid()) {
        return false;
    }

    if (!_socket->waitForConnected(1000)) {
        emit communicationError(title, error.arg(_tcpConfig->name(), tr("Connection failed")));
    }

    return true;
}

void TCPLink::_writeBytes(const QByteArray &bytes)
{
    if (!_socket->isValid()) {
        return;
    }

    static const QString title = tr("TCP Link Write Error");
    static const QString error = tr("Link %1: %2.");

    if (!isConnected()) {
        emit communicationError(title, error.arg(_tcpConfig->name(), tr("Could Not Send Data - Link is Disconnected!")));
        return;
    }

    const qint64 bytesWritten = _socket->write(bytes);
    if (bytesWritten < 0) {
        emit communicationError(title, error.arg(_tcpConfig->name(), tr("Could Not Send Data - Write Failed!")));
        return;
    }

    if (bytesWritten < bytes.size()) {
        qCWarning(TCPLinkLog) << "Wrote" << bytesWritten << "Out of" << bytes.size() << "total bytes";
        // const QByteArray remainingBytes = bytes.sliced(bytesWritten - 1, bytes.size() - bytesWritten);
        // writeBytesThreadSafe(remainingBytes.constData(), remainingBytes.size());
    }

    emit bytesSent(this, bytes);
}

void TCPLink::_readBytes()
{
    if (!_socket->isValid()) {
        return;
    }

    static const QString title = tr("TCP Link Read Error");
    static const QString error = tr("Link %1: %2.");

    if (!isConnected()) {
        emit communicationError(title, error.arg(_tcpConfig->name(), tr("Could Not Read Data - link is Disconnected!")));
        return;
    }

    const qint64 byteCount = _socket->bytesAvailable();
    // MAVLINK_NUM_NON_PAYLOAD_BYTES
    if (byteCount <= 0) {
        emit communicationError(title, error.arg(_tcpConfig->name(), tr("Could Not Read Data - No Data Available!")));
        return;
    }

    QByteArray buffer(byteCount, Qt::Initialization::Uninitialized);
    // _socket->readAll()
    if (_socket->read(buffer.data(), buffer.size()) < 0) {
        emit communicationError(title, error.arg(_tcpConfig->name(), tr("Could Not Read Data - Read Failed!")));
        return;
    }

    emit bytesReceived(this, buffer);
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
