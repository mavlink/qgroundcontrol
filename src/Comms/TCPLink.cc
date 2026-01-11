#include "TCPLink.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QTcpSocket>

QGC_LOGGING_CATEGORY(TCPLinkLog, "Comms.TCPLink")

namespace {
    constexpr int CONNECT_TIMEOUT_MS = 3000;
    constexpr int DISCONNECT_TIMEOUT_MS = 3000;
    constexpr int TYPE_OF_SERVICE = 32; // Set ToS to priority for low delay
}

/*===========================================================================*/

TCPConfiguration::TCPConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    qCDebug(TCPLinkLog) << this;
}

TCPConfiguration::TCPConfiguration(const TCPConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
    , _host(copy->host())
    , _port(copy->port())
{
    qCDebug(TCPLinkLog) << this;
}

TCPConfiguration::~TCPConfiguration()
{
    qCDebug(TCPLinkLog) << this;
}

void TCPConfiguration::setHost(const QString &host)
{
    const QString cleanHost = host.trimmed();
    if (cleanHost != _host) {
        _host = cleanHost;
        emit hostChanged();
    }
}

void TCPConfiguration::setPort(quint16 port)
{
    if (port != _port) {
        _port = port;
        emit portChanged();
    }
}

void TCPConfiguration::copyFrom(const LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);

    const TCPConfiguration* tcpSource = qobject_cast<const TCPConfiguration*>(source);

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

void TCPConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue(QStringLiteral("host"), host());
    settings.setValue(QStringLiteral("port"), port());

    settings.endGroup();
}

/*===========================================================================*/

TCPWorker::TCPWorker(const TCPConfiguration *config, QObject *parent)
    : QObject(parent)
    , _config(config)
{
    qCDebug(TCPLinkLog) << this;
}

TCPWorker::~TCPWorker()
{
    disconnectFromHost();

    qCDebug(TCPLinkLog) << this;
}

bool TCPWorker::isConnected() const
{
    return (_socket && _socket->isOpen() && (_socket->state() == QAbstractSocket::ConnectedState));
}

void TCPWorker::setupSocket()
{
    if (!_socket) {
        _socket = new QTcpSocket(this);
    }

    _socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    _socket->setSocketOption(QAbstractSocket::TypeOfServiceOption, TYPE_OF_SERVICE);

    (void) connect(_socket, &QTcpSocket::connected, this, &TCPWorker::_onSocketConnected);
    (void) connect(_socket, &QTcpSocket::disconnected, this, &TCPWorker::_onSocketDisconnected);
    (void) connect(_socket, &QTcpSocket::readyRead, this, &TCPWorker::_onSocketReadyRead);
    (void) connect(_socket, &QTcpSocket::errorOccurred, this, &TCPWorker::_onSocketErrorOccurred);

    if (TCPLinkLog().isDebugEnabled()) {
        // (void) connect(_socket, &QTcpSocket::bytesWritten, this, &TCPWorker::_onSocketBytesWritten);

        (void) connect(_socket, &QTcpSocket::stateChanged, this, [](QTcpSocket::SocketState state) {
            qCDebug(TCPLinkLog) << "TCP State Changed:" << state;
        });

        (void) connect(_socket, &QTcpSocket::hostFound, this, [this]() {
            qCDebug(TCPLinkLog) << "TCP Host Found" << _socket->peerName() << _socket->peerAddress() << _socket->peerPort();
        });
    }
}

void TCPWorker::connectToHost()
{
    if (isConnected()) {
        qCWarning(TCPLinkLog) << "Already connected to" << _config->host() << ":" << _config->port();
        return;
    }

    if (_config->host().isEmpty()) {
        emit errorOccurred(tr("Connection Failed: Host address is empty"));
        return;
    }

    _errorEmitted = false;

    qCDebug(TCPLinkLog) << "Attempting to connect to host:" << _config->host() << "port:" << _config->port();
    _socket->connectToHost(_config->host(), _config->port());

    if (!_socket->waitForConnected(CONNECT_TIMEOUT_MS)) {
        qCWarning(TCPLinkLog) << "Connection to" << _config->host() << ":" << _config->port() << "failed:" << _socket->errorString();

        if (!_errorEmitted.exchange(true)) {
            emit errorOccurred(tr("Connection Failed: %1").arg(_socket->errorString()));
        }

        _onSocketDisconnected();
    } else {
        qCDebug(TCPLinkLog) << "Successfully connected to host:" << _config->host() << "port:" << _config->port();
    }
}

void TCPWorker::disconnectFromHost()
{
    if (!isConnected()) {
        qCDebug(TCPLinkLog) << "Already disconnected from host:" << _config->host() << "port:" << _config->port();
        return;
    }

    qCDebug(TCPLinkLog) << "Attempting to disconnect from host:" << _config->host() << "port:" << _config->port();

    _socket->disconnectFromHost();

    if (_socket->state() != QAbstractSocket::UnconnectedState) {
        _socket->waitForDisconnected(1000);
    }
}

void TCPWorker::writeData(const QByteArray &data)
{
    if (data.isEmpty()) {
        emit errorOccurred(tr("Data to Send is Empty"));
        return;
    }

    if (!isConnected()) {
        emit errorOccurred(tr("Socket is not connected"));
        return;
    }

    qint64 totalBytesWritten = 0;
    while (totalBytesWritten < data.size()) {
        const qint64 bytesWritten = _socket->write(data.constData() + totalBytesWritten, data.size() - totalBytesWritten);
        if (bytesWritten == -1) {
            emit errorOccurred(tr("Could Not Send Data - Write Failed: %1").arg(_socket->errorString()));
            return;
        } else if (bytesWritten == 0) {
            emit errorOccurred(tr("Could Not Send Data - Write Returned 0 Bytes"));
            return;
        }
        totalBytesWritten += bytesWritten;
    }

    emit dataSent(data.first(totalBytesWritten));
}

void TCPWorker::_onSocketConnected()
{
    qCDebug(TCPLinkLog) << "Socket connected:" << _config->host() << _config->port();
    _errorEmitted = false;
    emit connected();
}

void TCPWorker::_onSocketDisconnected()
{
    qCDebug(TCPLinkLog) << "Socket disconnected:" << _config->host() << _config->port();
    _errorEmitted = false;
    emit disconnected();
}

void TCPWorker::_onSocketReadyRead()
{
    const QByteArray data = _socket->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}

void TCPWorker::_onSocketBytesWritten(qint64 bytes)
{
    qCDebug(TCPLinkLog) << _config->host() << "Wrote" << bytes << "bytes";
}

void TCPWorker::_onSocketErrorOccurred(QAbstractSocket::SocketError socketError)
{
    const QString errorString = _socket->errorString();
    qCWarning(TCPLinkLog) << "Socket error:" << socketError << errorString;

    if (!_errorEmitted.exchange(true)) {
        emit errorOccurred(errorString);
    }
}

/*===========================================================================*/

TCPLink::TCPLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _tcpConfig(qobject_cast<const TCPConfiguration*>(config.get()))
    , _worker(new TCPWorker(_tcpConfig))
    , _workerThread(new QThread(this))
{
    qCDebug(TCPLinkLog) << this;

    _workerThread->setObjectName(QStringLiteral("TCP_%1").arg(_tcpConfig->name()));

    _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::started, _worker, &TCPWorker::setupSocket);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);

    (void) connect(_worker, &TCPWorker::connected, this, &TCPLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker, &TCPWorker::disconnected, this, &TCPLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker, &TCPWorker::errorOccurred, this, &TCPLink::_onErrorOccurred, Qt::QueuedConnection);
    (void) connect(_worker, &TCPWorker::dataReceived, this, &TCPLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker, &TCPWorker::dataSent, this, &TCPLink::_onDataSent, Qt::QueuedConnection);

    _workerThread->start();
}

TCPLink::~TCPLink()
{
    if (isConnected()) {
        (void) QMetaObject::invokeMethod(_worker, "disconnectFromHost", Qt::BlockingQueuedConnection);
        _onDisconnected();
    }

    _workerThread->quit();
    if (!_workerThread->wait(DISCONNECT_TIMEOUT_MS)) {
        qCWarning(TCPLinkLog) << "Failed to wait for TCP Thread to close";
    }

    qCDebug(TCPLinkLog) << this;
}

bool TCPLink::isConnected() const
{
    return _worker && _worker->isConnected();
}

bool TCPLink::_connect()
{
    return QMetaObject::invokeMethod(_worker, "connectToHost", Qt::QueuedConnection);
}

void TCPLink::disconnect()
{
    if (isConnected()) {
        (void) QMetaObject::invokeMethod(_worker, "disconnectFromHost", Qt::QueuedConnection);
    }
}

void TCPLink::_onConnected()
{
    _disconnectedEmitted = false;
    emit connected();
}

void TCPLink::_onDisconnected()
{
    if (!_disconnectedEmitted.exchange(true)) {
        emit disconnected();
    }
}

void TCPLink::_onErrorOccurred(const QString &errorString)
{
    qCWarning(TCPLinkLog) << "Communication error:" << errorString;
    emit communicationError(tr("TCP Link Error"), tr("Link %1: (Host: %2 Port: %3) %4").arg(_tcpConfig->name(), _tcpConfig->host()).arg(_tcpConfig->port()).arg(errorString));
}

void TCPLink::_onDataReceived(const QByteArray &data)
{
    emit bytesReceived(this, data);
}

void TCPLink::_onDataSent(const QByteArray &data)
{
    emit bytesSent(this, data);
}

void TCPLink::_writeBytes(const QByteArray& bytes)
{
    (void) QMetaObject::invokeMethod(_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, bytes));
}

bool TCPLink::isSecureConnection() const
{
    return QGCNetworkHelper::isNetworkEthernet();
}
