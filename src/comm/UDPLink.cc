/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#include "UDPLink.h"
#include "QGC.h"
#include <QHostInfo>

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

UDPLink::UDPLink(SharedLinkConfigurationPointer& config)
    : LinkInterface(config)
#if defined(QGC_ZEROCONF_ENABLED)
    , _dnssServiceRef(NULL)
#endif
    , _running(false)
    , _socket(NULL)
    , _udpConfig(qobject_cast<UDPConfiguration*>(config.data()))
    , _connectState(false)
{
    Q_ASSERT(_udpConfig);
    moveToThread(this);
}

UDPLink::~UDPLink()
{
    _disconnect();
    // Tell the thread to exit
    _running = false;
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

void UDPLink::addHost(const QString& host)
{
    _udpConfig->addHost(host);
}

void UDPLink::removeHost(const QString& host)
{
    _udpConfig->removeHost(host);
}

void UDPLink::_writeBytes(const QByteArray data)
{
    if (!_socket)
        return;

    QStringList goneHosts;
    // Send to all connected systems
    QString host;
    int port;
    if(_udpConfig->firstHost(host, port)) {
        do {
            QHostAddress currentHost(host);
            if(_socket->writeDatagram(data, currentHost, (quint16)port) < 0) {
                // This host is gone. Add to list to be removed
                // We should keep track of hosts that were manually added (static) and
                // hosts that were added because we heard from them (dynamic). Only
                // dynamic hosts should be removed and even then, after a few tries, not
                // the first failure. In the mean time, we don't remove anything.
                if(REMOVE_GONE_HOSTS) {
                    goneHosts.append(host);
                }
            } else {
                // Only log rate if data actually got sent. Not sure about this as
                // "host not there" takes time too regardless of size of data. In fact,
                // 1 byte or "UDP frame size" bytes are the same as that's the data
                // unit sent by UDP.
                _logOutputDataRate(data.size(), QDateTime::currentMSecsSinceEpoch());
            }
        } while (_udpConfig->nextHost(host, port));
        //-- Remove hosts that are no longer there
        foreach (const QString& ghost, goneHosts) {
            _udpConfig->removeHost(ghost);
        }
    }
}

/**
 * @brief Read a number of bytes from the interface.
 **/
void UDPLink::readBytes()
{
    QByteArray databuffer;
    while (_socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        _socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        databuffer.append(datagram);
        //-- Wait a bit before sending it over
        if(databuffer.size() > 10 * 1024) {
            emit bytesReceived(this, databuffer);
            databuffer.clear();
        }
        _logInputDataRate(datagram.length(), QDateTime::currentMSecsSinceEpoch());
        // TODO This doesn't validade the sender. Anything sending UDP packets to this port gets
        // added to the list and will start receiving datagrams from here. Even a port scanner
        // would trigger this.
        // Add host to broadcast list if not yet present, or update its port
        _udpConfig->addHost(sender.toString(), (int)senderPort);
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
        _socket = NULL;
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
        _socket = NULL;
    }
    QHostAddress host = QHostAddress::AnyIPv4;
    _socket = new QUdpSocket();
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
        emit communicationError("UDP Link Error", "Error registering Zeroconf");
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
    _localPort = QGC_UDP_LOCAL_PORT;
}

UDPConfiguration::UDPConfiguration(UDPConfiguration* source) : LinkConfiguration(source)
{
    _localPort = source->localPort();
    QString host;
    int port;
    _hostList.clear();
    if(source->firstHost(host, port)) {
        do {
            addHost(host, port);
        } while(source->nextHost(host, port));
    }
}

void UDPConfiguration::copyFrom(LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);
    UDPConfiguration* usource = dynamic_cast<UDPConfiguration*>(source);
    Q_ASSERT(usource != NULL);
    _localPort = usource->localPort();
    _hosts.clear();
    QString host;
    int port;
    if(usource->firstHost(host, port)) {
        do {
            addHost(host, port);
        } while(usource->nextHost(host, port));
    }
}

/**
 * @param host Hostname in standard formatt, e.g. localhost:14551 or 192.168.1.1:14551
 */
void UDPConfiguration::addHost(const QString host)
{
    // Handle x.x.x.x:p
    if (host.contains(":"))
    {
        addHost(host.split(":").first(), host.split(":").last().toInt());
    }
    // If no port, use default
    else
    {
        addHost(host, (int)_localPort);
    }
}

void UDPConfiguration::addHost(const QString& host, int port)
{
    bool changed = false;
    QMutexLocker locker(&_confMutex);
    if(_hosts.contains(host)) {
        if(_hosts[host] != port) {
            _hosts[host] = port;
            changed = true;
        }
    } else {
        QString ipAdd = get_ip_address(host);
        if(ipAdd.isEmpty()) {
            qWarning() << "UDP:" << "Could not resolve host:" << host << "port:" << port;
        } else {
            // In simulation and testing setups the vehicle and the GCS can be
            // running on the same host. This leads to packets arriving through
            // the local network or the loopback adapter, which makes it look
            // like the vehicle is connected through two different links,
            // complicating routing.
            //
            // We detect this case and force all traffic to a simulated instance
            // onto the local loopback interface.
            bool not_local = true;
            // Run through all IPv4 interfaces and check if their canonical
            // IP address in string representation matches the source IP address
            foreach (const QHostAddress &address, QNetworkInterface::allAddresses()) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                    if (ipAdd.endsWith(address.toString())) {
                        // This is a local address of the same host
                        not_local = false;
                    }
                }
            }
            if (not_local) {
                // This is a normal remote host, add it using its IPv4 address
                _hosts[ipAdd] = port;
                //qDebug() << "UDP:" << "Adding Host:" << ipAdd << ":" << port;
            } else {
                // It is localhost, so talk to it through the IPv4 loopback interface
                _hosts["127.0.0.1"] = port;
            }
            changed = true;
        }
    }
    if(changed) {
        _updateHostList();
    }
}

void UDPConfiguration::removeHost(const QString host)
{
    QMutexLocker locker(&_confMutex);
    QString tHost = host;
    if (tHost.contains(":")) {
        tHost = tHost.split(":").first();
    }
    tHost = tHost.trimmed();
    QMap<QString, int>::iterator i = _hosts.find(tHost);
    if(i != _hosts.end()) {
        //qDebug() << "UDP:" << "Removed host:" << host;
        _hosts.erase(i);
    } else {
        qWarning() << "UDP:" << "Could not remove unknown host:" << host;
    }
    _updateHostList();
}

bool UDPConfiguration::firstHost(QString& host, int& port)
{
    _confMutex.lock();
    _it = _hosts.begin();
    if(_it == _hosts.end()) {
        _confMutex.unlock();
        return false;
    }
    _confMutex.unlock();
    return nextHost(host, port);
}

bool UDPConfiguration::nextHost(QString& host, int& port)
{
    QMutexLocker locker(&_confMutex);
    if(_it != _hosts.end()) {
        host = _it.key();
        port = _it.value();
        _it++;
        return true;
    }
    return false;
}

void UDPConfiguration::setLocalPort(quint16 port)
{
    _localPort = port;
}

void UDPConfiguration::saveSettings(QSettings& settings, const QString& root)
{
    _confMutex.lock();
    settings.beginGroup(root);
    settings.setValue("port", (int)_localPort);
    settings.setValue("hostCount", _hosts.count());
    int index = 0;
    QMap<QString, int>::const_iterator it = _hosts.begin();
    while(it != _hosts.end()) {
        QString hkey = QString("host%1").arg(index);
        settings.setValue(hkey, it.key());
        QString pkey = QString("port%1").arg(index);
        settings.setValue(pkey, it.value());
        it++;
        index++;
    }
    settings.endGroup();
    _confMutex.unlock();
}

void UDPConfiguration::loadSettings(QSettings& settings, const QString& root)
{
    _confMutex.lock();
    _hosts.clear();
    _confMutex.unlock();
    settings.beginGroup(root);
    _localPort = (quint16)settings.value("port", QGC_UDP_LOCAL_PORT).toUInt();
    int hostCount = settings.value("hostCount", 0).toInt();
    for(int i = 0; i < hostCount; i++) {
        QString hkey = QString("host%1").arg(i);
        QString pkey = QString("port%1").arg(i);
        if(settings.contains(hkey) && settings.contains(pkey)) {
            addHost(settings.value(hkey).toString(), settings.value(pkey).toInt());
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
    QMap<QString, int>::const_iterator it = _hosts.begin();
    while(it != _hosts.end()) {
        QString host = QString("%1").arg(it.key()) + ":" + QString("%1").arg(it.value());
        _hostList += host;
        it++;
    }
    emit hostListChanged();
}
