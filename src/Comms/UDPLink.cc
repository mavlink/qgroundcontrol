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
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"

#include <QtCore/QMutexLocker>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QUdpSocket>
#include <QtTest/QSignalSpy>

QGC_LOGGING_CATEGORY(UDPLinkLog, "test.comms.udplink")

const QHostAddress UDPLink::_multicastGroup = QHostAddress(QStringLiteral("224.0.0.1"));

static bool containsTarget(const QList<std::shared_ptr<UDPClient>> &list, const QHostAddress &address, quint16 port)
{
    for (const std::shared_ptr<UDPClient> &target : list) {
        if ((target->address == address) && (target->port == port)) {
            return true;
        }
    }

    return false;
}

UDPLink::UDPLink(SharedLinkConfigurationPtr &config, QObject *parent)
    : LinkInterface(config, parent)
    , _localAddresses(QNetworkInterface::allAddresses())
    , _udpConfig(qobject_cast<const UDPConfiguration*>(config.get()))
    , _socket(new QUdpSocket(this))
{
    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(_udpConfig);
    if (!_udpConfig) {
        qCWarning(UDPLinkLog) << "Invalid UDPConfiguration provided.";
        emit communicationError(
            tr("Configuration Error"),
            tr("Link %1: Invalid UDP configuration.").arg(config->name())
        );
        return;
    }

    _setupSocket();
}

UDPLink::~UDPLink()
{
#ifdef QGC_ZEROCONF_ENABLED
    _deregisterZeroconf();
#endif

    UDPLink::disconnect();

    _sessionTargets.clear();

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;
}

void UDPLink::_setupSocket()
{
    (void) QObject::connect(_socket, &QUdpSocket::connected, this, [this]() {
        _isConnected = true;
        _reconnectionAttempts = 0;
        qCDebug(UDPLinkLog) << "UDP connected to" << _udpConfig->localPort();
        emit connected();
    }, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QUdpSocket::disconnected, this, [this]() {
        _isConnected = false;
        qCDebug(UDPLinkLog) << "UDP disconnected from" << _udpConfig->localPort();
        emit disconnected();
    }, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QUdpSocket::readyRead, this, &UDPLink::_readBytes, Qt::QueuedConnection);

    (void) connect(_socket, &QUdpSocket::errorOccurred, this, [this](QUdpSocket::SocketError error) {
        qCWarning(UDPLinkLog) << "UDP Link error:" << error << _socket->errorString();
        emit communicationError(
            tr("UDP Link Error"),
            tr("Link %1: %2.").arg(_udpConfig->name(), _socket->errorString())
        );
    }, Qt::AutoConnection);

#ifdef QT_DEBUG
    (void) connect(_socket, &QUdpSocket::stateChanged, this, [](QUdpSocket::SocketState state) {
        qCDebug(UDPLinkLog) << "UDP State Changed:" << state;
    }, Qt::AutoConnection);

    (void) QObject::connect(_socket, &QUdpSocket::hostFound, this, []() {
        qCDebug(UDPLinkLog) << "UDP Host Found";
    }, Qt::AutoConnection);
#endif
}

bool UDPLink::isConnected() const
{
    return (_isConnected && _socket && _socket->isValid());
}

bool UDPLink::isSecureConnection()
{
    return QGCDeviceInfo::isNetworkWired();
}

void UDPLink::disconnect()
{
    if (isConnected()) {
        (void) _socket->leaveMulticastGroup(_multicastGroup);
        (void) QObject::disconnect(_socket, &QUdpSocket::readyRead, this, &UDPLink::_readBytes);
        _socket->close();
    } else {
        emit disconnected();
    }
}

bool UDPLink::_connect()
{
    if (isConnected()) {
        qCWarning(UDPLinkLog) << "Already connected to" << _udpConfig->localPort();
        return true;
    }

    QSignalSpy errorSpy(_socket, &QUdpSocket::errorOccurred);

    qCDebug(UDPLinkLog) << "Attempting to bind to port:" << _udpConfig->localPort();
    const bool bindSuccess = _socket->bind(QHostAddress::AnyIPv4, _udpConfig->localPort(), QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress);

    if (!bindSuccess) {
        // If binding failed and no error signal was emitted, emit communicationError manually
        if (errorSpy.count() == 0) {
            emit communicationError(
                tr("UDP Link Connect Error"),
                tr("Link %1: %2.").arg(_udpConfig->name(), tr("Failed to bind to port %1").arg(_udpConfig->localPort()))
            );
        }
        qCWarning(UDPLinkLog) << "Failed to bind UDP socket to port" << _udpConfig->localPort() << "with no error signal.";
        return false;
    }

    const bool joinSuccess = _socket->joinMulticastGroup(_multicastGroup);
    if (!joinSuccess) {
        emit communicationError(
            tr("UDP Link Connect Error"),
            tr("Link %1: %2.").arg(_udpConfig->name(), tr("Failed to join multicast group"))
        );
        qCWarning(UDPLinkLog) << "Failed to join multicast group" << _multicastGroup.toString();
        return false;
    }

#ifdef QGC_ZEROCONF_ENABLED
    _registerZeroconf(_udpConfig->localPort());
#endif

    qCDebug(UDPLinkLog) << "Successfully bound to port" << _udpConfig->localPort() << "and joined multicast group.";
    return true;
}

bool UDPLink::_isIpLocal(const QHostAddress &address) const
{
    return (address.isLoopback() || _localAddresses.contains(address));
}

void UDPLink::_writeBytes(const QByteArray &data)
{
    if (!_socket->isValid()) {
        emit communicationError(
            tr("UDP Link Write Error"),
            tr("Link %1: Could Not Send Data - Socket is Invalid!").arg(_udpConfig->name())
        );
        return;
    }

    if (!isConnected()) {
        emit communicationError(
            tr("UDP Link Write Error"),
            tr("Link %1: Could Not Send Data - Link is Disconnected!").arg(_udpConfig->name())
        );
        return;
    }

    QMutexLocker locker(&_sessionTargetsMutex);

    // Send to all manually targeted systems
    for (const std::shared_ptr<UDPClient> &target : _udpConfig->targetHosts()) {
        if (!_sessionTargets.contains(target)) {
            if (_socket->writeDatagram(data, target->address, target->port) < 0) {
                emit communicationError(
                    tr("UDP Link Write Error"),
                    tr("Link %1: Could Not Send Data - Write Failed!").arg(_udpConfig->name())
                );
            }
        }
    }

    // Send to all connected systems
    for (const std::shared_ptr<UDPClient> &target: _sessionTargets) {
        if (_socket->writeDatagram(data, target->address, target->port) < 0) {
            emit communicationError(
                tr("UDP Link Write Error"),
                tr("Link %1: Could Not Send Data - Write Failed!").arg(_udpConfig->name())
            );
        }
    }

    locker.unlock();

    emit bytesSent(this, data);
}

void UDPLink::_readBytes()
{
    if (!_socket->isValid()) {
        emit communicationError(
            tr("UDP Link Read Error"),
            tr("Link %1: Socket is Invalid!").arg(_udpConfig->name())
        );
        return;
    }

    static const QString title = QStringLiteral("UDP Link Read Error");
    static const QString error = QStringLiteral("Link %1: %2.");

    if (!isConnected()) {
        emit communicationError(
            tr("UDP Link Read Error"),
            tr("Link %1: Could Not Read Data - Link is Disconnected!").arg(_udpConfig->name())
        );
        return;
    }

    const qint64 byteCount = _socket->pendingDatagramSize();
    if (byteCount <= 0) {
        emit communicationError(title, error.arg(_udpConfig->name(), QStringLiteral("Could Not Read Data - No Data Available!")));
        return;
    }

    QByteArray buffer;
    QElapsedTimer timer;
    timer.start();
    while (_socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagramIn = _socket->receiveDatagram();
        if (datagramIn.isNull() || datagramIn.data().isEmpty()) {
            continue;
        }

        (void) buffer.append(datagramIn.data());

        if (buffer.size() > (10 * 1024) || (timer.elapsed() > 50)) {
            emit bytesReceived(this, buffer);
            buffer.clear();
            (void) timer.restart();
        }

        const QHostAddress senderAddress = _isIpLocal(datagramIn.senderAddress()) ? QHostAddress(QHostAddress::SpecialAddress::LocalHost) : datagramIn.senderAddress();

        QMutexLocker locker(&_sessionTargetsMutex);
        if (!containsTarget(_sessionTargets, senderAddress, datagramIn.senderPort())) {
            qCDebug(UDPLinkLog) << Q_FUNC_INFO << "Adding target:" << senderAddress << datagramIn.senderPort();
            (void) _sessionTargets.append(std::make_shared<UDPClient>(senderAddress, datagramIn.senderPort()));
        }
        locker.unlock();
    }

    if (buffer.isEmpty()) {
        emit communicationError(
            title,
            error.arg(_udpConfig->name(), tr("No Data Available to Read!"))
        );
        return;
    }

    emit bytesReceived(this, buffer);
}

#ifdef QGC_ZEROCONF_ENABLED
void UDPLink::_zeroconfRegisterCallback(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context)
{
    Q_UNUSED(sdRef); Q_UNUSED(flags); Q_UNUSED(name); Q_UNUSED(regtype); Q_UNUSED(domain);

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO;

    UDPLink *const udplink = static_cast<UDPLink*>(context);
    if (errorCode != kDNSServiceErr_NoError) {
        emit udplink->communicationError(
            tr("UDP Link Zeroconf Error"),
            tr("Link %1: %2.").arg(udplink->linkConfiguration->name(), tr("Zeroconf Register Error!"))
        );
    }
}

void UDPLink::_registerZeroconf(uint16_t port)
{
    static constexpr const char *regType = "_qgroundcontrol._udp";

    if (_dnssServiceRef) {
        qCWarning(UDPLinkLog) << "Already registered zeroconf";
        return;
    }

#if (Q_BYTE_ORDER == Q_LITTLE_ENDIAN)
    port = htons(port); // qToBigEndian
#endif

    const DNSServiceErrorType result = DNSServiceRegister(
        &_dnssServiceRef,
        0,
        0,
        0,
        regType,
        NULL,
        NULL,
        port,
        0,
        NULL,
        &UDPLink::_zeroconfRegisterCallback,
        this
    );

    if (result != kDNSServiceErr_NoError) {
        _dnssServiceRef = NULL;
        emit communicationError(
            tr("UDP Link Zeroconf Error"),
            tr("Link %1: %2.").arg(_udpConfig->name(), tr("Error Registering Zeroconf"))
        );
        return;
    }

    const int sockfd = DNSServiceRefSockFD(_dnssServiceRef);
    if (sockfd == -1) {
        emit communicationError(
            tr("UDP Link Zeroconf Error"),
            tr("Link %1: %2.").arg(_udpConfig->name(), tr("Invalid sockfd"))
        );
        return;
    }

    _socketNotifier = new QSocketNotifier(sockfd, QSocketNotifier::Read, this);
    (void) connect(_socketNotifier, &QSocketNotifier::activated, this, [this]() {
        const DNSServiceErrorType error = DNSServiceProcessResult(_dnssServiceRef);
        if (err != kDNSServiceErr_NoError) {
            emit communicationError(title, error.arg(_udpConfig->name(), error));
        }
        _socketNotifier->deleteLater();
    });
}

void UDPLink::_deregisterZeroconf()
{
    if (_dnssServiceRef) {
        DNSServiceRefDeallocate(_dnssServiceRef);
        _dnssServiceRef = NULL;
    }
}
#endif // QGC_ZEROCONF_ENABLED

/*===========================================================================*/

UDPConfiguration::UDPConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    AutoConnectSettings *const settings = SettingsManager::instance()->autoConnectSettings();
    _localPort = settings->udpListenPort()->rawValue().toInt();

    const QString targetHostIP = settings->udpTargetHostIP()->rawValue().toString();
    if (!targetHostIP.isEmpty()) {
        const quint16 targetHostPort = settings->udpTargetHostPort()->rawValue().toUInt();
        addHost(targetHostIP, targetHostPort);
    }
}

UDPConfiguration::UDPConfiguration(const UDPConfiguration *source, QObject *parent)
    : LinkConfiguration(source, parent)
{
    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(source);

    UDPConfiguration::copyFrom(source);
}

UDPConfiguration::~UDPConfiguration()
{
    _targetHosts.clear();

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;
}

void UDPConfiguration::copyFrom(const LinkConfiguration *source)
{
    Q_CHECK_PTR(source);
    LinkConfiguration::copyFrom(source);

    const UDPConfiguration *const udpSource = qobject_cast<const UDPConfiguration*>(source);
    Q_CHECK_PTR(udpSource);

    setLocalPort(udpSource->localPort());
    _targetHosts.clear();

    for (const std::shared_ptr<UDPClient> &target : udpSource->targetHosts()) {
        if (!_targetHosts.contains(target)) {
            _targetHosts.append(std::make_shared<UDPClient>(target.get()));
            _updateHostList();
        }
    }
}

void UDPConfiguration::addHost(const QString &host)
{
    if (host.contains(":")) {
        const QStringList hostInfo = host.split(":");
        if (hostInfo.size() != 2) {
            qCWarning(UDPLinkLog) << "Invalid host format:" << host;
            return;
        }
        addHost(hostInfo.constFirst(), hostInfo.constLast().toUInt());
    } else {
        addHost(host, _localPort);
    }
}

void UDPConfiguration::addHost(const QString &host, quint16 port)
{
    const QString ipAdd = _getIpAddress(host);
    if (ipAdd.isEmpty()) {
        qCWarning(UDPLinkLog) << Q_FUNC_INFO << "Could not resolve host:" << host << "port:" << port;
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
        const QHostAddress address = QHostAddress(_getIpAddress(hostInfo.at(0)));
        const quint16 port = hostInfo.at(1).toUInt();

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

    _updateHostList();

    qCWarning(UDPLinkLog) << Q_FUNC_INFO << "Could not remove unknown host:" << host;
}

void UDPConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    AutoConnectSettings* const autoConnectSettings = SettingsManager::instance()->autoConnectSettings();
    setLocalPort(static_cast<quint16>(settings.value("port", autoConnectSettings->udpListenPort()->rawValue().toUInt()).toUInt()));

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

void UDPConfiguration::saveSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    settings.setValue("hostCount", _targetHosts.size());
    settings.setValue("port", _localPort);
    for (qsizetype i = 0; i < _targetHosts.size(); i++) {
        const std::shared_ptr<UDPClient> target = _targetHosts.at(i);
        const QString hkey = QString("host%1").arg(i);
        settings.setValue(hkey, target->address.toString());
        const QString pkey = QString("port%1").arg(i);
        settings.setValue(pkey, target->port);
    }

    settings.endGroup();
}

void UDPConfiguration::_updateHostList()
{
    _hostList.clear();
    for (const std::shared_ptr<UDPClient> &target : _targetHosts) {
        const QString host = target->address.toString() + ":" + QString::number(target->port);
        (void) _hostList.append(host);
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
