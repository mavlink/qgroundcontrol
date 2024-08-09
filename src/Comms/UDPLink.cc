/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UDPLink.h"

#include <QGCApplication.h>
#include <SettingsManager.h>
#include <AutoConnectSettings.h>
#include <QGCLoggingCategory.h>

#include <QtCore/QList>
#include <QtCore/QMutexLocker>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QUdpSocket>

QGC_LOGGING_CATEGORY(UDPLinkLog, "qgc.comms.udplink")

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
    (void) QObject::connect(_socket, &QUdpSocket::readyRead, this, &UDPLink::_readBytes, Qt::AutoConnection);

    (void) connect(_socket, &QUdpSocket::errorOccurred, this, [this](QUdpSocket::SocketError error) {
        qCWarning(UDPLinkLog) << "UDP Link error:" << error << _socket->errorString();
        emit communicationError(QStringLiteral("UDP Link Error"), _socket->errorString());
    }, Qt::AutoConnection);

    (void) connect(_socket, &QUdpSocket::stateChanged, this, [](QUdpSocket::SocketState state) {
        qCDebug(UDPLinkLog) << "UDP State Changed:" << state;
    }, Qt::AutoConnection);
}

UDPLink::~UDPLink()
{
    UDPLink::disconnect();

#ifdef QGC_ZEROCONF_ENABLED
    _deregisterZeroconf();
#endif

    _sessionTargets.clear();

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;
}

bool UDPLink::isConnected() const
{
    return (_socket->isValid() && ((_socket->state() == QAbstractSocket::SocketState::ConnectedState) || (_socket->state() == QAbstractSocket::SocketState::ConnectingState)));
}

void UDPLink::disconnect()
{
    (void) QObject::disconnect(_socket, &QUdpSocket::readyRead, this, &UDPLink::_readBytes);
    _socket->close();

    emit disconnected();
}

bool UDPLink::_connect()
{
    if (isConnected()) {
        return true;
    }

    // TODO: Support IPv6
    if (_socket->bind(QHostAddress(QHostAddress::AnyIPv4), _udpConfig->localPort(), QAbstractSocket::ReuseAddressHint | QAbstractSocket::ShareAddress)) {
        (void) QObject::connect(_socket, &QUdpSocket::readyRead, this, &UDPLink::_readBytes, Qt::AutoConnection);

        (void) _socket->joinMulticastGroup(QHostAddress(QStringLiteral("224.0.0.1")));

#ifdef __mobile__
        _socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 64 * 1024);
        _socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 128 * 1024);
#else
        _socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 256 * 1024);
        _socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 512 * 1024);
#endif

#ifdef QGC_ZEROCONF_ENABLED
        _registerZeroconf(_udpConfig->localPort(), "_qgroundcontrol._udp");
#endif

        emit connected();
        return true;
    }

    emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link: %1, %2.").arg(_udpConfig->name(), "Failed to Connect."));
    return false;
}

bool UDPLink::_isIpLocal(const QHostAddress &address) const
{
    return (address.isLoopback() || _localAddresses.contains(address));
}

#ifdef QGC_ZEROCONF_ENABLED
void UDPLink::_registerZeroconf(uint16_t port, const char *regType)
{
    const DNSServiceErrorType result = DNSServiceRegister(
        &_dnssServiceRef,
        0,
        0,
        0,
        regType,
        NULL,
        NULL,
        htons(port),
        0,
        NULL,
        NULL,
        NULL
    );

    if (result != kDNSServiceErr_NoError) {
        _dnssServiceRef = NULL;
        emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link: %1, Error Registering Zeroconf").arg(_udpConfig->name()));
    }
}

void UDPLink::_deregisterZeroconf()
{
    if (_dnssServiceRef) {
        DNSServiceRefDeallocate(_dnssServiceRef);
        _dnssServiceRef = NULL;
    }
}
#endif // QGC_ZEROCONF_ENABLED

void UDPLink::_writeBytes(const QByteArray &data)
{
    if (!isConnected()) {
        emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link %1: %2.").arg(_udpConfig->name(), "Could Not Send Data - Link is Disconnected!"));
        return;
    }

    QMutexLocker locker(&_sessionTargetsMutex);

    // Send to all manually targeted systems
    for (const std::shared_ptr<UDPClient> &target : _udpConfig->targetHosts()) {
        if (!_sessionTargets.contains(target)) {
            if (_socket->writeDatagram(data, target->address, target->port) < 0) {
                emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link %1: %2.").arg(_udpConfig->name(), "Could Not Send Data - Write Failed!"));
            }
        }
    }

    // Send to all connected systems
    for (const std::shared_ptr<UDPClient> &target: _sessionTargets) {
        if (_socket->writeDatagram(data, target->address, target->port) < 0) {
            emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link %1: %2.").arg(_udpConfig->name(), "Could Not Send Data - Write Failed!"));
        }
    }

    locker.unlock();

    emit bytesSent(this, data);
}

void UDPLink::_readBytes()
{
    if (!isConnected()) {
        emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link %1: %2.").arg(_udpConfig->name(), QStringLiteral("Could Not Read Data - link is Disconnected!")));
        return;
    }

    const qint64 byteCount = _socket->pendingDatagramSize();
    if (byteCount <= 0) {
        emit communicationError(QStringLiteral("Link Error"), QStringLiteral("Link %1: %2.").arg(_udpConfig->name(), QStringLiteral("Could Not Read Data - No Data Available!")));
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

        // qCInfo(UDPLinkLog) << Q_FUNC_INFO << datagramIn.senderAddress() << datagramIn.senderPort() << datagramIn.data().size() << "bytes on interface" << datagramIn.interfaceIndex();

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
        emit communicationError(QStringLiteral("UDP Link Error"), QStringLiteral("Link %1: %2.").arg(_udpConfig->name(), "Could Not Read Data - Read Failed!"));
        return;
    }

    emit bytesReceived(this, buffer);
}

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

UDPConfiguration::UDPConfiguration(UDPConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
{
    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;

    Q_CHECK_PTR(copy);

    UDPConfiguration::copyFrom(copy);
}

UDPConfiguration::~UDPConfiguration()
{
    _targetHosts.clear();

    // qCDebug(UDPLinkLog) << Q_FUNC_INFO << this;
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

void UDPConfiguration::setLocalPort(quint16 port)
{
    if (port != _localPort) {
        _localPort = port;
        emit localPortChanged();
    }
}

void UDPConfiguration::copyFrom(LinkConfiguration *source)
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
