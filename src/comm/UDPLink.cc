/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Definition of UDP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QtGlobal>
#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <QNetworkProxy>
#include <QNetworkInterface>
#include <iostream>
#include <QHostInfo>

#include "UDPLink.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "AutoConnectSettings.h"

#define REMOVE_GONE_HOSTS 0

static const char* kZeroconfRegistration = "_qgroundcontrol._udp";

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
        for (int i = 0; i < hostAddresses.size(); i++)
        {
            // Exclude all IPv6 addresses
            if (!hostAddresses.at(i).toString().contains(":"))
            {
                return hostAddresses.at(i).toString();
            }
        }
    }
    return {};
}

static bool contains_target(const QList<UDPCLient*> list, const QHostAddress& address, quint16 port)
{
    for(UDPCLient* target: list) {
        if(target->address == address && target->port == port) {
            return true;
        }
    }
    return false;
}

UDPLink::UDPLink(SharedLinkConfigurationPointer& config)
    : LinkInterface(config)
    #if defined(QGC_ZEROCONF_ENABLED)
    , _dnssServiceRef(nullptr)
    #endif
    , _running(false)
    , _socket(nullptr)
    , _udpConfig(qobject_cast<UDPConfiguration*>(config.data()))
    , _connectState(false)
{
    if (!_udpConfig) {
        qWarning() << "Internal error";
    }
    for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
        _localAddress.append(QHostAddress(address));
    }
    moveToThread(this);
}

UDPLink::~UDPLink()
{
    _disconnect();
    // Tell the thread to exit
    _running = false;
    // Clear client list
    qDeleteAll(_sessionTargets);
    _sessionTargets.clear();
    quit();
    // Wait for it to exit
    wait();
    this->deleteLater();
}

/**
 * @brief Runs the thread
 *
 **/
void UDPLink::run()
{
    if(_hardwareConnect()) {
        exec();
    }
    if (_socket) {
        _deregisterZeroconf();
        _socket->close();
    }
}

void UDPLink::_restartConnection()
{
    if(this->isConnected())
    {
        _disconnect();
        _connect();
    }
}

QString UDPLink::getName() const
{
    return _udpConfig->name();
}

bool UDPLink::_isIpLocal(const QHostAddress& add)
{
    // In simulation and testing setups the vehicle and the GCS can be
    // running on the same host. This leads to packets arriving through
    // the local network or the loopback adapter, which makes it look
    // like the vehicle is connected through two different links,
    // complicating routing.
    //
    // We detect this case and force all traffic to a simulated instance
    // onto the local loopback interface.
    // Run through all IPv4 interfaces and check if their canonical
    // IP address in string representation matches the source IP address
    //
    // On Windows, this is a very expensive call only Redmond would know
    // why. As such, we make it once and keep the list locally. If a new
    // interface shows up after we start, it won't be on this list.
    for (const QHostAddress &address: _localAddress) {
        if (address == add) {
            // This is a local address of the same host
            return true;
        }
    }
    return false;
}

void UDPLink::_writeBytes(const QByteArray data)
{
    if (!_socket) {
        return;
    }
    emit bytesSent(this, data);
    // Send to all manually targeted systems
    for(UDPCLient* target: _udpConfig->targetHosts()) {
        // Skip it if it's part of the session clients below
        if(!contains_target(_sessionTargets, target->address, target->port)) {
            _writeDataGram(data, target);
        }
    }
    // Send to all connected systems
    for(UDPCLient* target: _sessionTargets) {
        _writeDataGram(data, target);
    }
}

void UDPLink::_writeDataGram(const QByteArray data, const UDPCLient* target)
{
    //qDebug() << "UDP Out" << target->address << target->port;
    if(_socket->writeDatagram(data, target->address, target->port) < 0) {
        qWarning() << "Error writing to" << target->address << target->port;
    } else {
        // Only log rate if data actually got sent. Not sure about this as
        // "host not there" takes time too regardless of size of data. In fact,
        // 1 byte or "UDP frame size" bytes are the same as that's the data
        // unit sent by UDP.
        _logOutputDataRate(data.size(), QDateTime::currentMSecsSinceEpoch());
    }
}

/**
 * @brief Read a number of bytes from the interface.
 **/
void UDPLink::readBytes()
{
    if (!_socket) {
        return;
    }
    QByteArray databuffer;
    while (_socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        //-- Note: This call is broken in Qt 5.9.3 on Windows. It always returns a blank sender and 0 for the port.
        _socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        databuffer.append(datagram);
        //-- Wait a bit before sending it over
        if(databuffer.size() > 10 * 1024) {
            emit bytesReceived(this, databuffer);
            databuffer.clear();
        }
        _logInputDataRate(datagram.length(), QDateTime::currentMSecsSinceEpoch());
        // TODO: This doesn't validade the sender. Anything sending UDP packets to this port gets
        // added to the list and will start receiving datagrams from here. Even a port scanner
        // would trigger this.
        // Add host to broadcast list if not yet present, or update its port
        QHostAddress asender = sender;
        if(_isIpLocal(sender)) {
            asender = QHostAddress(QString("127.0.0.1"));
        }
        if(!contains_target(_sessionTargets, asender, senderPort)) {
            qDebug() << "Adding target" << asender << senderPort;
            UDPCLient* target = new UDPCLient(asender, senderPort);
            _sessionTargets.append(target);
        }
    }
    //-- Send whatever is left
    if(databuffer.size()) {
        emit bytesReceived(this, databuffer);
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
void UDPLink::_disconnect(void)
{
    _running = false;
    quit();
    wait();
    if (_socket) {
        // Make sure delete happen on correct thread
        _socket->deleteLater();
        _socket = nullptr;
        emit disconnected();
    }
    _connectState = false;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool UDPLink::_connect(void)
{
    if(this->isRunning() || _running)
    {
        _running = false;
        quit();
        wait();
    }
    _running = true;
    start(NormalPriority);
    return true;
}

bool UDPLink::_hardwareConnect()
{
    if (_socket) {
        delete _socket;
        _socket = nullptr;
    }
    QHostAddress host = QHostAddress::AnyIPv4;
    _socket = new QUdpSocket(this);
    _socket->setProxy(QNetworkProxy::NoProxy);
    _connectState = _socket->bind(host, _udpConfig->localPort(), QAbstractSocket::ReuseAddressHint | QUdpSocket::ShareAddress);
    if (_connectState) {
        _socket->joinMulticastGroup(QHostAddress("224.0.0.1"));
        //-- Make sure we have a large enough IO buffers
#ifdef __mobile__
        _socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,     64 * 1024);
        _socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 128 * 1024);
#else
        _socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,    256 * 1024);
        _socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 512 * 1024);
#endif
        _registerZeroconf(_udpConfig->localPort(), kZeroconfRegistration);
        QObject::connect(_socket, &QUdpSocket::readyRead, this, &UDPLink::readBytes);
        emit connected();
    } else {
        emit communicationError(tr("UDP Link Error"), tr("Error binding UDP port: %1").arg(_socket->errorString()));
    }
    return _connectState;
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool UDPLink::isConnected() const
{
    return _connectState;
}

qint64 UDPLink::getConnectionSpeed() const
{
    return 54000000; // 54 Mbit
}

qint64 UDPLink::getCurrentInDataRate() const
{
    return 0;
}

qint64 UDPLink::getCurrentOutDataRate() const
{
    return 0;
}

void UDPLink::_registerZeroconf(uint16_t port, const std::string &regType)
{
#if defined(QGC_ZEROCONF_ENABLED)
    DNSServiceErrorType result = DNSServiceRegister(&_dnssServiceRef, 0, 0, 0,
                                                    regType.c_str(),
                                                    NULL,
                                                    NULL,
                                                    htons(port),
                                                    0,
                                                    NULL,
                                                    NULL,
                                                    NULL);
    if (result != kDNSServiceErr_NoError)
    {
        emit communicationError(tr("UDP Link Error"), tr("Error registering Zeroconf"));
        _dnssServiceRef = NULL;
    }
#else
    Q_UNUSED(port);
    Q_UNUSED(regType);
#endif
}

void UDPLink::_deregisterZeroconf()
{
#if defined(QGC_ZEROCONF_ENABLED)
    if (_dnssServiceRef)
    {
        DNSServiceRefDeallocate(_dnssServiceRef);
        _dnssServiceRef = NULL;
    }
#endif
}

//--------------------------------------------------------------------------
//-- UDPConfiguration

UDPConfiguration::UDPConfiguration(const QString& name) : LinkConfiguration(name)
{
    AutoConnectSettings* settings = qgcApp()->toolbox()->settingsManager()->autoConnectSettings();
    _localPort = settings->udpListenPort()->rawValue().toInt();
    QString targetHostIP = settings->udpTargetHostIP()->rawValue().toString();
    if (!targetHostIP.isEmpty()) {
        addHost(targetHostIP, settings->udpTargetHostPort()->rawValue().toUInt());
    }
}

UDPConfiguration::UDPConfiguration(UDPConfiguration* source) : LinkConfiguration(source)
{
    _copyFrom(source);
}

UDPConfiguration::~UDPConfiguration()
{
    _clearTargetHosts();
}

void UDPConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    _copyFrom(source);
}

void UDPConfiguration::_copyFrom(LinkConfiguration *source)
{
    auto* usource = qobject_cast<UDPConfiguration*>(source);
    if (usource) {
        _localPort = usource->localPort();
        _clearTargetHosts();
        for(UDPCLient* target: usource->targetHosts()) {
            if(!contains_target(_targetHosts, target->address, target->port)) {
                UDPCLient* newTarget = new UDPCLient(target);
                _targetHosts.append(newTarget);
                _updateHostList();
            }
        }
    } else {
        qWarning() << "Internal error";
    }
}

void UDPConfiguration::_clearTargetHosts()
{
    qDeleteAll(_targetHosts);
    _targetHosts.clear();
}

/**
 * @param host Hostname in standard formatt, e.g. localhost:14551 or 192.168.1.1:14551
 */
void UDPConfiguration::addHost(const QString host)
{
    // Handle x.x.x.x:p
    if (host.contains(":"))
    {
        addHost(host.split(":").first(), host.split(":").last().toUInt());
    }
    // If no port, use default
    else
    {
        addHost(host, _localPort);
    }
}

void UDPConfiguration::addHost(const QString& host, quint16 port)
{
    QString ipAdd = get_ip_address(host);
    if(ipAdd.isEmpty()) {
        qWarning() << "UDP:" << "Could not resolve host:" << host << "port:" << port;
    } else {
        QHostAddress address(ipAdd);
        if(!contains_target(_targetHosts, address, port)) {
            UDPCLient* newTarget = new UDPCLient(address, port);
            _targetHosts.append(newTarget);
            _updateHostList();
        }
    }
}

void UDPConfiguration::removeHost(const QString host)
{
    if (host.contains(":"))
    {
        QHostAddress address = QHostAddress(get_ip_address(host.split(":").first()));
        quint16 port = host.split(":").last().toUInt();
        for(int i = 0; i < _targetHosts.size(); i++) {
            UDPCLient* target = _targetHosts.at(i);
            if(target->address == address && target->port == port) {
                _targetHosts.removeAt(i);
                delete target;
                _updateHostList();
                return;
            }
        }
    }
    qWarning() << "UDP:" << "Could not remove unknown host:" << host;
    _updateHostList();
}

void UDPConfiguration::setLocalPort(quint16 port)
{
    _localPort = port;
}

void UDPConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    settings.beginGroup(root);
    settings.setValue("port", (int)_localPort);
    settings.setValue("hostCount", _targetHosts.size());
    for(int i = 0; i < _targetHosts.size(); i++) {
        UDPCLient* target = _targetHosts.at(i);
        QString hkey = QString("host%1").arg(i);
        settings.setValue(hkey, target->address.toString());
        QString pkey = QString("port%1").arg(i);
        settings.setValue(pkey, target->port);
    }
    settings.endGroup();
}

void UDPConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    AutoConnectSettings* acSettings = qgcApp()->toolbox()->settingsManager()->autoConnectSettings();
    _clearTargetHosts();
    settings.beginGroup(root);
    _localPort = (quint16)settings.value("port", acSettings->udpListenPort()->rawValue().toInt()).toUInt();
    int hostCount = settings.value("hostCount", 0).toInt();
    for(int i = 0; i < hostCount; i++) {
        QString hkey = QString("host%1").arg(i);
        QString pkey = QString("port%1").arg(i);
        if(settings.contains(hkey) && settings.contains(pkey)) {
            addHost(settings.value(hkey).toString(), settings.value(pkey).toUInt());
        }
    }
    settings.endGroup();
    _updateHostList();
}

void UDPConfiguration::updateSettings()
{
    if(_link) {
        UDPLink* ulink = dynamic_cast<UDPLink*>(_link);
        if(ulink) {
            ulink->_restartConnection();
        }
    }
}

void UDPConfiguration::_updateHostList()
{
    _hostList.clear();
    for(int i = 0; i < _targetHosts.size(); i++) {
        UDPCLient* target = _targetHosts.at(i);
        QString host = QString("%1").arg(target->address.toString()) + ":" + QString("%1").arg(target->port);
        _hostList << host;
    }
    emit hostListChanged();
}
