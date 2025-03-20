/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UDPLink.h"
#include "AutoConnectSettings.h"
#include "DeviceInfo.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QUdpSocket>

QGC_LOGGING_CATEGORY(UDPLinkLog, "qgc.comms.udplink")

namespace {
    constexpr int BUFFER_TRIGGER_SIZE = 10 * 1024;
    constexpr int RECEIVE_TIME_LIMIT_MS = 50;

    bool containsTarget(const QList<std::shared_ptr<UDPClient>> &list, const QHostAddress &address, quint16 port)
    {
        for (const std::shared_ptr<UDPClient> &target : list) {
            if ((target->address == address) && (target->port == port)) {
                return true;
            }
        }

        return false;
    }
}

/*===========================================================================*/

UDPConfiguration::UDPConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
}

UDPConfiguration::UDPConfiguration(const UDPConfiguration *source, QObject *parent)
    : LinkConfiguration(source, parent)
{
    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;

    UDPConfiguration::copyFrom(source);
}

UDPConfiguration::~UDPConfiguration()
{
    _targetHosts.clear();

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;
}

void UDPConfiguration::setAutoConnect(bool autoc)
{
    if (isAutoConnect() != autoc) {
        AutoConnectSettings *const settings = SettingsManager::instance()->autoConnectSettings();
        const QString targetHostIP = settings->udpTargetHostIP()->rawValue().toString();
        const quint16 targetHostPort = settings->udpTargetHostPort()->rawValue().toUInt();
        if (autoc) {
            setLocalPort(settings->udpListenPort()->rawValue().toInt());    
            if (!targetHostIP.isEmpty()) {
                addHost(targetHostIP, targetHostPort);
            }
        }
        else {
            setLocalPort(0);
            if (!targetHostIP.isEmpty()) {
                removeHost(targetHostIP, targetHostPort);
            }
        }
        LinkConfiguration::setAutoConnect(autoc);
    }
}

void UDPConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_ASSERT(source);
    LinkConfiguration::copyFrom(source);

    const UDPConfiguration *const udpSource = qobject_cast<const UDPConfiguration*>(source);
    Q_ASSERT(udpSource);

    setLocalPort(udpSource->localPort());
    _targetHosts.clear();

    for (const std::shared_ptr<UDPClient> &target : udpSource->targetHosts()) {
        if (!_targetHosts.contains(target)) {
            _targetHosts.append(std::make_shared<UDPClient>(target.get()));
            _updateHostList();
        }
    }
}

void UDPConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setLocalPort(static_cast<quint16>(settings.value("port", SettingsManager::instance()->autoConnectSettings()->udpListenPort()->rawValue().toUInt()).toUInt()));

    _targetHosts.clear();
    const qsizetype hostCount = settings.value("hostCount", 0).toUInt();
    for (qsizetype i = 0; i < hostCount; i++) {
        const QString hkey = QStringLiteral("host%1").arg(i);
        const QString pkey = QStringLiteral("port%1").arg(i);
        if (settings.contains(hkey) && settings.contains(pkey)) {
            addHost(settings.value(hkey).toString(), settings.value(pkey).toUInt());
        }
    }

    _updateHostList();

    settings.endGroup();
}

void UDPConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue(QStringLiteral("hostCount"), _targetHosts.size());
    settings.setValue(QStringLiteral("port"), _localPort);

    for (qsizetype i = 0; i < _targetHosts.size(); i++) {
        const std::shared_ptr<UDPClient> target = _targetHosts.at(i);
        const QString hkey = QStringLiteral("host%1").arg(i);
        settings.setValue(hkey, target->address.toString());
        const QString pkey = QStringLiteral("port%1").arg(i);
        settings.setValue(pkey, target->port);
    }

    settings.endGroup();
}

void UDPConfiguration::addHost(const QString &host)
{
    if (host.contains(":")) {
        const QStringList hostInfo = host.split(":");
        if (hostInfo.size() != 2) {
            qCWarning(UDPLinkLog) << "Invalid host format:" << host;
            return;
        }

        const QString address = hostInfo.constFirst();
        const quint16 port = hostInfo.constLast().toUInt();

        addHost(address, port);
    } else {
        addHost(host, _localPort);
    }
}

void UDPConfiguration::addHost(const QString &host, quint16 port)
{
    const QString ipAdd = _getIpAddress(host);
    if (ipAdd.isEmpty()) {
        qCWarning(UDPLinkLog) << "Could not resolve host:" << host << "port:" << port;
        return;
    }

    const QHostAddress address(ipAdd);
    if (!containsTarget(_targetHosts, address, port)) {
        _targetHosts.append(std::make_shared<UDPClient>(address, port));
        _updateHostList();
    }
}

void UDPConfiguration::removeHost(const QString &host)
{
    if (host.contains(":")) {
        const QStringList hostInfo = host.split(":");
        if (hostInfo.size() != 2) {
            qCWarning(UDPLinkLog) << "Invalid host format:" << host;
            return;
        }

        const QHostAddress address = QHostAddress(_getIpAddress(hostInfo.constFirst()));
        const quint16 port = hostInfo.constLast().toUInt();

        if (!containsTarget(_targetHosts, address, port)) {
            qCWarning(UDPLinkLog) << "Could not remove unknown host:" << host << "port:" << port;
            return;
        }

        QMutableListIterator<std::shared_ptr<UDPClient>> it(_targetHosts);
        while (it.hasNext()) {
            std::shared_ptr<UDPClient> target = it.next();
            if ((target->address == address) && (target->port == port)) {
                target.reset();
                it.remove();
                _updateHostList();
                return;
            }
        }
    } else {
        removeHost(host, _localPort);
    }
}

void UDPConfiguration::removeHost(const QString &host, quint16 port)
{
    const QString ipAdd = _getIpAddress(host);
    if (ipAdd.isEmpty()) {
        qCWarning(UDPLinkLog) << "Could not resolve host:" << host << "port:" << port;
        return;
    }

    const QHostAddress address(ipAdd);
    if (!containsTarget(_targetHosts, address, port)) {
        qCWarning(UDPLinkLog) << "Could not remove unknown host:" << host << "port:" << port;
        return;
    }

    QMutableListIterator<std::shared_ptr<UDPClient>> it(_targetHosts);
    while (it.hasNext()) {
        std::shared_ptr<UDPClient> target = it.next();
        if ((target->address == address) && (target->port == port)) {
            target.reset();
            it.remove();
            _updateHostList();
            return;
        }
    }
}

void UDPConfiguration::_updateHostList()
{
    _hostList.clear();
    for (const std::shared_ptr<UDPClient> &target : _targetHosts) {
        const QString host = target->address.toString() + ":" + QString::number(target->port);
        _hostList.append(host);
    }

    emit hostListChanged();
}

QString UDPConfiguration::_getIpAddress(const QString &address)
{
    const QHostAddress host(address);
    if (!host.isNull()) {
        return address;
    }

    const QHostInfo info = QHostInfo::fromName(address);
    if (info.error() != QHostInfo::NoError) {
        return QString();
    }

    const QList<QHostAddress> hostAddresses = info.addresses();
    for (const QHostAddress &hostAddress : hostAddresses) {
        if (hostAddress.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
            return hostAddress.toString();
        }
    }

    return QString();
}

/*===========================================================================*/

const QHostAddress UDPWorker::_multicastGroup = QHostAddress(QStringLiteral("224.0.0.1"));

UDPWorker::UDPWorker(const UDPConfiguration *config, QObject *parent)
    : QObject(parent)
    , _udpConfig(config)
{
    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;
}

UDPWorker::~UDPWorker()
{
    disconnectLink();

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;
}

bool UDPWorker::isConnected() const
{
    return (_socket && _socket->isValid() && _isConnected);
}

void UDPWorker::setupSocket()
{
    Q_ASSERT(!_socket);
    _socket = new QUdpSocket(this);

    const QList<QHostAddress> localAddresses = QNetworkInterface::allAddresses();
    _localAddresses = QSet(localAddresses.constBegin(), localAddresses.constEnd());

    _socket->setProxy(QNetworkProxy::NoProxy);

    (void) connect(_socket, &QUdpSocket::connected, this, &UDPWorker::_onSocketConnected);
    (void) connect(_socket, &QUdpSocket::disconnected, this, &UDPWorker::_onSocketDisconnected);
    (void) connect(_socket, &QUdpSocket::readyRead, this, &UDPWorker::_onSocketReadyRead);
    (void) connect(_socket, &QUdpSocket::errorOccurred, this, &UDPWorker::_onSocketErrorOccurred);
    (void) connect(_socket, &QUdpSocket::stateChanged, this, [this](QUdpSocket::SocketState state) {
        qCDebug(UDPLinkLog) << "UDP State Changed:" << state;
        switch (state) {
        case QAbstractSocket::BoundState:
            _onSocketConnected();
            break;
        case QAbstractSocket::ClosingState:
        case QAbstractSocket::UnconnectedState:
            _onSocketDisconnected();
            break;
        default:
            break;
        }
    });

    if (UDPLinkLog().isDebugEnabled()) {
        // (void) connect(_socket, &QUdpSocket::bytesWritten, this, &UDPWorker::_onSocketBytesWritten);

        (void) QObject::connect(_socket, &QUdpSocket::hostFound, this, []() {
            qCDebug(UDPLinkLog) << "UDP Host Found";
        });
    }
}

void UDPWorker::connectLink()
{
    if (isConnected()) {
        qCWarning(UDPLinkLog) << "Already connected to" << _udpConfig->localPort();
        return;
    }

    _errorEmitted = false;

    qCDebug(UDPLinkLog) << "Attempting to bind to port:" << _udpConfig->localPort();
    const bool bindSuccess = _socket->bind(QHostAddress::AnyIPv4, _udpConfig->localPort(), QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress);
    if (!bindSuccess) {
        qCWarning(UDPLinkLog) << "Failed to bind UDP socket to port" << _udpConfig->localPort();

        if (!_errorEmitted) {
            emit errorOccurred(tr("Failed to bind UDP socket to port"));
            _errorEmitted = true;
        }

        // Disconnecting here on autoconnect will cause continuous error popups
        /*if (!_udpConfig->isAutoConnect()) {
            _onSocketDisconnected();
        }*/

        return;
    }

    qCDebug(UDPLinkLog) << "Attempting to join multicast group:" << _multicastGroup.toString();
    const bool joinSuccess = _socket->joinMulticastGroup(_multicastGroup);
    if (!joinSuccess) {
        qCWarning(UDPLinkLog) << "Failed to join multicast group" << _multicastGroup.toString();
    }

#ifdef QGC_ZEROCONF_ENABLED
    _registerZeroconf(_udpConfig->localPort());
#endif
}

void UDPWorker::disconnectLink()
{
#ifdef QGC_ZEROCONF_ENABLED
    _deregisterZeroconf();
#endif

    if (isConnected()) {
        (void) _socket->leaveMulticastGroup(_multicastGroup);
        _socket->close();
    }

    _sessionTargets.clear();
}

void UDPWorker::writeData(const QByteArray &data)
{
    if (!isConnected()) {
        emit errorOccurred(tr("Could Not Send Data - Link is Disconnected!"));
        return;
    }

    QMutexLocker locker(&_sessionTargetsMutex);

    // Send to all manually targeted systems
    for (const std::shared_ptr<UDPClient> &target : _udpConfig->targetHosts()) {
        if (!_sessionTargets.contains(target)) {
            if (_socket->writeDatagram(data, target->address, target->port) < 0) {
                qCWarning(UDPLinkLog) << "Could Not Send Data - Write Failed!";
            }
        }
    }

    // Send to all connected systems
    for (const std::shared_ptr<UDPClient> &target: _sessionTargets) {
        if (_socket->writeDatagram(data, target->address, target->port) < 0) {
            qCWarning(UDPLinkLog) << "Could Not Send Data - Write Failed!";
        }
    }

    locker.unlock();

    emit dataSent(data);
}

void UDPWorker::_onSocketConnected()
{
    qCDebug(UDPLinkLog) << "UDP connected to" << _udpConfig->localPort();
    _isConnected = true;
    _errorEmitted = false;
    emit connected();
}

void UDPWorker::_onSocketDisconnected()
{
    qCDebug(UDPLinkLog) << "UDP disconnected from" << _udpConfig->localPort();
    _isConnected = false;
    _errorEmitted = false;
    emit disconnected();
}

void UDPWorker::_onSocketReadyRead()
{
    if (!isConnected()) {
        emit errorOccurred(tr("Could Not Read Data - Link is Disconnected!"));
        return;
    }

    const qint64 byteCount = _socket->pendingDatagramSize();
    if (byteCount <= 0) {
        emit errorOccurred(tr("Could Not Read Data - No Data Available!"));
        return;
    }

    QByteArray buffer;
    buffer.reserve(BUFFER_TRIGGER_SIZE);
    QElapsedTimer timer;
    timer.start();
    bool received = false;
    while (_socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagramIn = _socket->receiveDatagram();
        if (datagramIn.isNull() || datagramIn.data().isEmpty()) {
            continue;
        }

        (void) buffer.append(datagramIn.data());

        if ((buffer.size() > BUFFER_TRIGGER_SIZE) || (timer.elapsed() > RECEIVE_TIME_LIMIT_MS)) {
            received = true;
            emit dataReceived(buffer);
            buffer.clear();
            (void) timer.restart();
        }

        const bool ipLocal = datagramIn.senderAddress().isLoopback() || _localAddresses.contains(datagramIn.senderAddress());
        const QHostAddress senderAddress = ipLocal ? QHostAddress(QHostAddress::SpecialAddress::LocalHost) : datagramIn.senderAddress();

        QMutexLocker locker(&_sessionTargetsMutex);
        if (!containsTarget(_sessionTargets, senderAddress, datagramIn.senderPort())) {
            qCDebug(UDPLinkLog) << "UDP Adding target:" << senderAddress << datagramIn.senderPort();
            _sessionTargets.append(std::make_shared<UDPClient>(senderAddress, datagramIn.senderPort()));
        }
        locker.unlock();
    }

    if (!received && buffer.isEmpty()) {
        qCWarning(UDPLinkLog) << "No Data Available to Read!";
        return;
    }

    emit dataReceived(buffer);
}

void UDPWorker::_onSocketBytesWritten(qint64 bytes)
{
    qCDebug(UDPLinkLog) << "Wrote" << bytes << "bytes";
}

void UDPWorker::_onSocketErrorOccurred(QUdpSocket::SocketError error)
{
    const QString errorString = _socket->errorString();
    qCWarning(UDPLinkLog) << "UDP Link error:" << error << _socket->errorString();

    if (!_errorEmitted) {
        emit errorOccurred(errorString);
        _errorEmitted = true;
    }
}

#ifdef QGC_ZEROCONF_ENABLED
void UDPWorker::_zeroconfRegisterCallback(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context)
{
    Q_UNUSED(sdRef); Q_UNUSED(flags); Q_UNUSED(name); Q_UNUSED(regtype); Q_UNUSED(domain);

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO;

    UDPWorker *const worker = static_cast<UDPWorker*>(context);
    if (errorCode != kDNSServiceErr_NoError) {
        emit worker->errorOccurred(tr("Zeroconf Register Error: %1").arg(errorCode));
    }
}

void UDPWorker::_registerZeroconf(uint16_t port)
{
    static constexpr const char *regType = "_qgroundcontrol._udp";

    if (_dnssServiceRef) {
        qCWarning(UDPLinkLog) << "Already registered zeroconf";
        return;
    }

    const DNSServiceErrorType result = DNSServiceRegister(
        &_dnssServiceRef,
        0,
        0,
        0,
        regType,
        NULL,
        NULL,
        qToBigEndian(port),
        0,
        NULL,
        &UDPWorker::_zeroconfRegisterCallback,
        this
    );

    if (result != kDNSServiceErr_NoError) {
        _dnssServiceRef = NULL;
        emit errorOccurred(tr("Error Registering Zeroconf: %1").arg(result));
        return;
    }

    const int sockfd = DNSServiceRefSockFD(_dnssServiceRef);
    if (sockfd == -1) {
        emit errorOccurred(tr("Invalid sockfd"));
        return;
    }

    QSocketNotifier *const socketNotifier = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
    (void) connect(socketNotifier, &QSocketNotifier::activated, this, [this, socketNotifier]() {
        const DNSServiceErrorType error = DNSServiceProcessResult(_dnssServiceRef);
        if (error != kDNSServiceErr_NoError) {
            emit errorOccurred(tr("DNSServiceProcessResult Error: %1").arg(error));
        }
        socketNotifier->deleteLater();
    });
}

void UDPWorker::_deregisterZeroconf()
{
    if (_dnssServiceRef) {
        DNSServiceRefDeallocate(_dnssServiceRef);
        _dnssServiceRef = NULL;
    }
}
#endif // QGC_ZEROCONF_ENABLED

/*===========================================================================*/

UDPLink::UDPLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _udpConfig(qobject_cast<const UDPConfiguration*>(config.get()))
    , _worker(new UDPWorker(_udpConfig))
    , _workerThread(new QThread(this))
{
    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;

    _workerThread->setObjectName(QStringLiteral("UDP_%1").arg(_udpConfig->name()));

    _worker->moveToThread(_workerThread);

    (void) connect(_workerThread, &QThread::started, _worker, &UDPWorker::setupSocket);
    (void) connect(_workerThread, &QThread::finished, _worker, &QObject::deleteLater);

    (void) connect(_worker, &UDPWorker::connected, this, &UDPLink::_onConnected, Qt::QueuedConnection);
    (void) connect(_worker, &UDPWorker::disconnected, this, &UDPLink::_onDisconnected, Qt::QueuedConnection);
    (void) connect(_worker, &UDPWorker::errorOccurred, this, &UDPLink::_onErrorOccurred, Qt::QueuedConnection);
    (void) connect(_worker, &UDPWorker::dataReceived, this, &UDPLink::_onDataReceived, Qt::QueuedConnection);
    (void) connect(_worker, &UDPWorker::dataSent, this, &UDPLink::_onDataSent, Qt::QueuedConnection);

    _workerThread->start();
}

UDPLink::~UDPLink()
{
    UDPLink::disconnect();

    _workerThread->quit();
    if (!_workerThread->wait()) {
        qCWarning(UDPLinkLog) << "Failed to wait for UDP Thread to close";
    }

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;
}

bool UDPLink::isConnected() const
{
    return _worker->isConnected();
}

bool UDPLink::_connect()
{
    return QMetaObject::invokeMethod(_worker, "connectLink", Qt::QueuedConnection);
}

void UDPLink::disconnect()
{
    (void) QMetaObject::invokeMethod(_worker, "disconnectLink", Qt::QueuedConnection);
}

void UDPLink::_onConnected()
{
    emit connected();
}

void UDPLink::_onDisconnected()
{
    emit disconnected();
}

void UDPLink::_onErrorOccurred(const QString &errorString)
{
    qCWarning(UDPLinkLog) << "Communication error:" << errorString;
    emit communicationError(tr("UDP Link Error"), tr("Link %1: %2").arg(_udpConfig->name(), errorString));
}

void UDPLink::_onDataReceived(const QByteArray &data)
{
    emit bytesReceived(this, data);
}

void UDPLink::_onDataSent(const QByteArray &data)
{
    emit bytesSent(this, data);
}

void UDPLink::_writeBytes(const QByteArray& bytes)
{
    (void) QMetaObject::invokeMethod(_worker, "writeData", Qt::QueuedConnection, Q_ARG(QByteArray, bytes));
}

bool UDPLink::isSecureConnection() const
{
    return QGCDeviceInfo::isNetworkWired();
}
