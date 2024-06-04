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

QGC_LOGGING_CATEGORY(UDPLinkLog, "READY.comms.udplink")

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

    (void) QObject::connect(_socket, &QUdpSocket::connected, this, &UDPLink::connected, Qt::AutoConnection);
    (void) QObject::connect(_socket, &QUdpSocket::disconnected, this, &UDPLink::disconnected, Qt::AutoConnection);

    (void) connect(_socket, &QUdpSocket::errorOccurred, this, [this](QUdpSocket::SocketError error) {
        qCWarning(UDPLinkLog) << "UDP Link error:" << error << _socket->errorString();
        emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link: %1, %2.").arg(_udpConfig->name(), _socket->errorString()));
    }, Qt::AutoConnection);

#ifdef QT_DEBUG
    (void) connect(_socket, &QUdpSocket::stateChanged, this, [](QUdpSocket::SocketState state) {
        qCDebug(UDPLinkLog) << "UDP State Changed:" << state;
    }, Qt::AutoConnection);
#endif
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

bool UDPLink::isSecureConnection()
{
    return QGCDeviceInfo::isNetworkWired();
}

bool UDPLink::isConnected() const
{
    return (_socket->isValid() && ((_socket->state() == QAbstractSocket::SocketState::ConnectedState) || (_socket->state() == QAbstractSocket::SocketState::ConnectingState) || (_socket->state() == QAbstractSocket::SocketState::BoundState)));
}

void UDPLink::disconnect()
{
    if (isConnected()) {
        (void) _socket->leaveMulticastGroup(_multicastGroup);
        (void) QObject::disconnect(_socket, &QUdpSocket::readyRead, this, &UDPLink::_readBytes);
        _socket->close();
    }
}

bool UDPLink::_connect()
{
    if (isConnected()) {
        return true;
    }

    if (_socket->bind(QHostAddress(QHostAddress::AnyIPv4), _udpConfig->localPort(), QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress)) {
        (void) QObject::connect(_socket, &QUdpSocket::readyRead, this, &UDPLink::_readBytes, Qt::AutoConnection);
        (void) _socket->joinMulticastGroup(_multicastGroup);

#ifdef QGC_ZEROCONF_ENABLED
        _registerZeroconf(_udpConfig->localPort());
#endif

        return true;
    }

    emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link: %1, %2.").arg(_udpConfig->name(), "Failed to Connect."));
    return false;
}

bool UDPLink::_isIpLocal(const QHostAddress &address) const
{
    return (address.isLoopback() || _localAddresses.contains(address));
}

void UDPLink::_writeBytes(const QByteArray &data)
{
    if (!_socket->isValid()) {
        return;
    }

    static const QString title = QStringLiteral("UDP Link Write Error");
    static const QString error = QStringLiteral("Link %1: %2.");

    if (!isConnected()) {
        emit communicationError(title, error.arg(_udpConfig->name(), "Could Not Send Data - Link is Disconnected!"));
        return;
    }

    QMutexLocker locker(&_sessionTargetsMutex);

    // Send to all manually targeted systems
    for (const std::shared_ptr<UDPClient> &target : _udpConfig->targetHosts()) {
        if (!_sessionTargets.contains(target)) {
            if (_socket->writeDatagram(data, target->address, target->port) < 0) {
                emit communicationError(title, error.arg(_udpConfig->name(), "Could Not Send Data - Write Failed!"));
            }
        }
    }

    // Send to all connected systems
    for (const std::shared_ptr<UDPClient> &target: _sessionTargets) {
        if (_socket->writeDatagram(data, target->address, target->port) < 0) {
            emit communicationError(title, error.arg(_udpConfig->name(), "Could Not Send Data - Write Failed!"));
        }
    }

    locker.unlock();

    emit bytesSent(this, data);
}

void UDPLink::_readBytes()
{
    if (!_socket->isValid()) {
        return;
    }

    static const QString title = QStringLiteral("UDP Link Read Error");
    static const QString error = QStringLiteral("Link %1: %2.");

    if (!isConnected()) {
        emit communicationError(title, error.arg(_udpConfig->name(), QStringLiteral("Could Not Read Data - link is Disconnected!")));
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
        emit communicationError(title, error.arg(_udpConfig->name(), "Could Not Read Data - Read Failed!"));
        return;
    }

    emit bytesReceived(this, buffer);
}


#ifdef QGC_ZEROCONF_ENABLED
void UDPLink::_zeroconfRegisterCallback(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context)
{
    Q_UNUSED(sdRef)
    Q_UNUSED(flags)
    Q_UNUSED(name)
    Q_UNUSED(regtype)
    Q_UNUSED(domain)
    qCDebug(UDPLinkLog) << Q_FUNC_INFO;

    static const QString title = QStringLiteral("UDP Link Zeroconf Error");
    static const QString error = QStringLiteral("Link %1: %2.");

    UDPLink* const udplink = static_cast<UDPLink*>(context);
    if (errorCode != kDNSServiceErr_NoError) {
        emit udplink->communicationError(title, error.arg(udplink->linkConfiguration->name(), QStringLiteral("Zeroconf Register Error!")));
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

    static const QString title = QStringLiteral("UDP Link Zeroconf Error");
    static const QString error = QStringLiteral("Link %1: %2.");

    if (result != kDNSServiceErr_NoError) {
        _dnssServiceRef = NULL;
        emit communicationError(title, error.arg(_udpConfig->name(), QStringLiteral("Error Registering Zeroconf")));
        return;
    }

    const int sockfd = DNSServiceRefSockFD(_dnssServiceRef);
    if (sockfd == -1) {
        emit communicationError(title, error.arg(_udpConfig->name(), QStringLiteral("Invalid sockfd")));
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

//------------------------------------------------------------------------------

UDPConfiguration::UDPConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    AutoConnectSettings* const settings = qgcApp()->toolbox()->settingsManager()->autoConnectSettings();
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

    const UDPConfiguration* const udpSource = qobject_cast<const UDPConfiguration*>(source);
    Q_CHECK_PTR(udpSource);

    setLocalPort(udpSource->localPort());
    _targetHosts.clear();

    for (const std::shared_ptr<UDPClient> &target : udpSource->targetHosts()) {
        if (!_targetHosts.contains(target)) {
            (void) _targetHosts.append(std::make_shared<UDPClient>(target.get()));
            _updateHostList();
        }
    }
}

void UDPConfiguration::addHost(const QString &host)
{
    if (host.contains(":")) {
        const QStringList hostInfo = host.split(":");
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
        (void) _targetHosts.append(std::make_shared<UDPClient>(address, port));
        _updateHostList();
    }
}

void UDPConfiguration::removeHost(const QString &host)
{
    if (host.contains(":")) {
        const QHostAddress address = QHostAddress(_getIpAddress(host.split(":").constFirst()));
        const quint16 port = host.split(":").constLast().toUInt();

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

    AutoConnectSettings* const autoConnectSettings = qgcApp()->toolbox()->settingsManager()->autoConnectSettings();
    setLocalPort(static_cast<quint16>(settings.value("port", autoConnectSettings->udpListenPort()->rawValue().toUInt()).toUInt()));

    _targetHosts.clear();
    const qsizetype hostCount = settings.value("hostCount", 0).toUInt();
    for (qsizetype i = 0; i < hostCount; i++) {
        const QString hkey = QStringLiteral("host%1").arg(i);
        const QString pkey = QStringLiteral("port%1").arg(i);
        if(settings.contains(hkey) && settings.contains(pkey)) {
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
    if (info.error() == QHostInfo::NoError) {
        const QList<QHostAddress> hostAddresses = info.addresses();
        for (const QHostAddress &hostAddress : hostAddresses) {
            if (hostAddress.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                return hostAddress.toString();
            }
        }
    }

    return QString();
}
