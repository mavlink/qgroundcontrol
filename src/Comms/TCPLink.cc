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
    constexpr int CONNECT_TIMEOUT_MS = 1000;
    constexpr int TYPE_OF_SERVICE = 32; // Set ToS for low delay
    constexpr int MAX_RECONNECTION_ATTEMPTS = 3;
}

TCPWorker::TCPWorker(const QString &host, quint16 port, QObject *parent)
    : QObject(parent)
    , _socket(new QTcpSocket(this))
    , _host(host)
    , _port(port)
{
    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    _socket->setSocketOption(QAbstractSocket::TypeOfServiceOption, TYPE_OF_SERVICE);

    (void) connect(_socket, &QTcpSocket::connected, this, &TCPWorker::_onSocketConnected);
    (void) connect(_socket, &QTcpSocket::disconnected, this, &TCPWorker::_onSocketDisconnected);
    (void) connect(_socket, &QTcpSocket::readyRead, this, &TCPWorker::_onSocketReadyRead);
    (void) connect(_socket, &QTcpSocket::errorOccurred, this, &TCPWorker::_onSocketErrorOccurred);

#ifdef QT_DEBUG
    (void) QObject::connect(_socket, &QTcpSocket::stateChanged, this, [](QTcpSocket::SocketState state) {
        qCDebug(TCPLinkLog) << "TCP State Changed:" << state;
    }, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QTcpSocket::hostFound, this, []() {
        qCDebug(TCPLinkLog) << "TCP Host Found";
    }, Qt::AutoConnection);
#endif
}

TCPWorker::~TCPWorker()
{
    disconnectFromHost();
}

bool TCPWorker::isConnected() const
{
    return (_socket->isOpen() && (_socket->state() == QAbstractSocket::ConnectedState));
}

void TCPWorker::connectToHost()
{
    if (isConnected()) {
        qCWarning(TCPLinkLog) << "Already connected to" << _host << ":" << _port;
        return;
    }

    QSignalSpy errorSpy(_socket, &QTcpSocket::errorOccurred);

    qCDebug(TCPLinkLog) << "Attempting to connect to host:" << _host << "port:" << _port;
    _socket->connectToHost(_host, _port);

    if (!_socket->waitForConnected(CONNECT_TIMEOUT_MS)) {
        qCWarning(TCPLinkLog) << "Connection to" << _host << ":" << _port << "failed:" << _socket->errorString();
        if (errorSpy.count() == 0) {
            emit errorOccurred(tr("Connection Failed: %1").arg(_socket->errorString()));
        }
        _onSocketDisconnected();
    }

    qCDebug(TCPLinkLog) << "Successfully connected to" << _host.toString() << ":" << _port;
}

void TCPWorker::disconnectFromHost()
{
    if (isConnected()) {
        _socket->disconnectFromHost();
    }
}

void TCPWorker::writeData(const QByteArray &data)
{
    if (isConnected()) {
        qint64 totalBytesWritten = 0;
        while (totalBytesWritten < data.size()) {
            const qint64 bytesWritten = _socket->write(data.constData() + totalBytesWritten, data.size() - totalBytesWritten);
            if (bytesWritten <= 0) {
                break;
            }

            totalBytesWritten += bytesWritten;
        }

        if (totalBytesWritten < 0) {
            emit errorOccurred(tr("Could Not Send Data - Write Failed: %1").arg(_socket->errorString()));
        }
    } else {
        emit errorOccurred(tr("Socket is not connected"));
    }
}

void TCPWorker::_onSocketConnected()
{
    emit connected();
}

void TCPWorker::_onSocketDisconnected()
{
    emit disconnected();
}

void TCPWorker::_onSocketReadyRead()
{
    const QByteArray data = _socket->readAll();
    emit dataReceived(data);
}

void TCPWorker::_onSocketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    emit errorOccurred(_socket->errorString());
}

/*===========================================================================*/

TCPLink::TCPLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _tcpConfig(qobject_cast<const TCPConfiguration*>(config.get()))
    , _worker(nullptr)
    , _workerThread(new QThread(this))
{
    Q_CHECK_PTR(_tcpConfig);
    if (!_tcpConfig) {
        qCWarning(TCPLinkLog) << "Invalid TCPConfiguration provided.";
        emit communicationError(
            tr("Configuration Error"),
            tr("Link %1: Invalid TCP configuration.").arg(config->name())
        );
        return;
    }

    _worker = new TCPWorker(_tcpConfig->host(), _tcpConfig->port());

    _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);

    (void) connect(_worker, &TCPWorker::connected, this, &TCPLink::_onConnected);
    (void) connect(_worker, &TCPWorker::disconnected, this, &TCPLink::_onDisconnected);
    (void) connect(_worker, &TCPWorker::errorOccurred, this, &TCPLink::_onErrorOccurred);
    (void) connect(_worker, &TCPWorker::dataReceived, this, &TCPLink::_onDataReceived);

#ifdef QT_DEBUG
    _workerThread->setObjectName(QString("TCP_%1").arg(config->name()));
#endif

    _workerThread->start();
}

TCPLink::~TCPLink()
{
    disconnect();

    _workerThread->quit();
    _workerThread->wait();
}

bool TCPLink::isConnected() const
{
    return (_worker && _worker->isConnected());
}

bool TCPLink::_connect()
{
    return QMetaObject::invokeMethod(_worker, "connectToHost", Qt::QueuedConnection);
}

void TCPLink::disconnect()
{
    (void) QMetaObject::invokeMethod(_worker, "disconnectFromHost", Qt::QueuedConnection);
}

void TCPLink::_onConnected()
{
    emit connected();
}

void TCPLink::_onDisconnected()
{
    emit disconnected();
}

void TCPLink::_onErrorOccurred(const QString &errorString)
{
    emit communicationError(tr("TCP Link Error"), tr("Link %1: %2").arg(_tcpConfig->name(), errorString));
}

void TCPLink::_onDataReceived(const QByteArray &data)
{
    // Process data or emit signal
    emit bytesReceived(this, data);
}

void TCPLink::_writeBytes(const QByteArray& bytes)
{
    (void) QMetaObject::invokeMethod(_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, bytes));
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
