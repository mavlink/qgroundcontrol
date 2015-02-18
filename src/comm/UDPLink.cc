/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of UDP connection (server) for unmanned vehicles
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QTimer>
#include <QList>
#include <QDebug>
#include <QMutexLocker>
#include <QNetworkProxy>
#include <iostream>

#include "UDPLink.h"
#include "QGC.h"
#include <QHostInfo>

UDPLink::UDPLink(UDPConfiguration* config)
    : _socket(NULL)
    , _connectState(false)
{
    Q_ASSERT(config != NULL);
    _config = config;
    _config->setLink(this);

    // We're doing it wrong - because the Qt folks got the API wrong:
    // http://blog.qt.digia.com/blog/2010/06/17/youre-doing-it-wrong/
    moveToThread(this);

    // Set unique ID and add link to the list of links
    _id = getNextLinkId();
    qDebug() << "UDP Created " << _config->name();
}

UDPLink::~UDPLink()
{
    // Disconnect link from configuration
    _config->setLink(NULL);
    _disconnect();
    // Tell the thread to exit
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
    _hardwareConnect();
    exec();
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
    return _config->name();
}

void UDPLink::addHost(const QString& host)
{
    qDebug() << "UDP:" << "ADDING HOST:" << host;
    _config->addHost(host);
}

void UDPLink::removeHost(const QString& host)
{
    _config->removeHost(host);
}

#define UDPLINK_DEBUG 0

void UDPLink::writeBytes(const char* data, qint64 size)
{
    // Broadcast to all connected systems
    QString host;
    int port;
    if(_config->firstHost(host, port)) {
        do {
            if(UDPLINK_DEBUG) {
                QString bytes;
                QString ascii;
                for (int i=0; i<size; i++)
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
                qDebug() << "Sent" << size << "bytes to" << host << ":" << port << "data:";
                qDebug() << bytes;
                qDebug() << "ASCII:" << ascii;
            }
            QHostAddress currentHost(host);
            _socket->writeDatagram(data, size, currentHost, (quint16)port);
            // Log the amount and time written out for future data rate calculations.
            QMutexLocker dataRateLocker(&dataRateMutex);
            logDataRateToBuffer(outDataWriteAmounts, outDataWriteTimes, &outDataIndex, size, QDateTime::currentMSecsSinceEpoch());
        } while (_config->nextHost(host, port));
    }
}

/**
 * @brief Read a number of bytes from the interface.
 **/
void UDPLink::readBytes()
{
    while (_socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(_socket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;
        _socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // FIXME TODO Check if this method is better than retrieving the data by individual processes
        emit bytesReceived(this, datagram);

        // Log this data reception for this timestep
        QMutexLocker dataRateLocker(&dataRateMutex);
        logDataRateToBuffer(inDataWriteAmounts, inDataWriteTimes, &inDataIndex, datagram.length(), QDateTime::currentMSecsSinceEpoch());

//        // Echo data for debugging purposes
//        std::cerr << __FILE__ << __LINE__ << "Received datagram:" << std::endl;
//        int i;
//        for (i=0; i<s; i++)
//        {
//            unsigned int v=data[i];
//            fprintf(stderr,"%02x ", v);
//        }
//        std::cerr << std::endl;

        // TODO This doesn't validade the sender. Anything sending UDP packets to this port gets
        // added to the list and will start receiving datagrams from here. Even a port scanner
        // would trigger this.
        // Add host to broadcast list if not yet present, or update its port
        QString host(sender.toString() + ":" + QString("%1").arg((int)senderPort));
        _config->addHost(host);
    }
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool UDPLink::_disconnect(void)
{
    this->quit();
    this->wait();
    if (_socket) {
        // Make sure delete happen on correct thread
        _socket->deleteLater();
        _socket = NULL;
        emit disconnected();
    }
    // TODO When would this ever return false?
    _connectState = false;
    return !_connectState;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool UDPLink::_connect(void)
{
    if(this->isRunning())
    {
        this->quit();
        this->wait();
    }
    // TODO When would this ever return false?
    bool connected = true;
    start(HighPriority);
    return connected;
}

bool UDPLink::_hardwareConnect()
{
    QHostAddress host = QHostAddress::Any;
    _socket = new QUdpSocket();
    _socket->setProxy(QNetworkProxy::NoProxy);
    _connectState = _socket->bind(host, _config->localPort());
    QObject::connect(_socket, SIGNAL(readyRead()), this, SLOT(readBytes()));
    if (_connectState) {
        emit connected();
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

int UDPLink::getId() const
{
    return _id;
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

//--------------------------------------------------------------------------
//-- UDPConfiguration

UDPConfiguration::UDPConfiguration(const QString& name) : LinkConfiguration(name)
{
    _localPort = QGC_UDP_PORT;
}

UDPConfiguration::UDPConfiguration(UDPConfiguration* source) : LinkConfiguration(source)
{
    _localPort = source->localPort();
    _hosts.clear();
    QString host;
    int port;
    if(source->firstHost(host, port)) {
        do {
            addHost(host, port);
        } while(source->nextHost(host, port));
    }
}

void UDPConfiguration::copyFrom(LinkConfiguration *source)
{
    _confMutex.lock();
    LinkConfiguration::copyFrom(source);
    UDPConfiguration* usource = dynamic_cast<UDPConfiguration*>(source);
    Q_ASSERT(usource != NULL);
    _localPort = usource->localPort();
    QString host;
    int port;
    if(usource->firstHost(host, port)) {
        do {
            addHost(host, port);
        } while(usource->nextHost(host, port));
    }
    _confMutex.unlock();
}

/**
 * @param host Hostname in standard formatt, e.g. localhost:14551 or 192.168.1.1:14551
 */
void UDPConfiguration::addHost(const QString& host)
{
    qDebug() << "UDP:" << "ADDING HOST:" << host;
    if (host.contains(":"))
    {
        QHostInfo info = QHostInfo::fromName(host.split(":").first());
        if (info.error() == QHostInfo::NoError)
        {
            // Add host
            QList<QHostAddress> hostAddresses = info.addresses();
            QHostAddress address;
            for (int i = 0; i < hostAddresses.size(); i++)
            {
                // Exclude loopback IPv4 and all IPv6 addresses
                if (!hostAddresses.at(i).toString().contains(":"))
                {
                    address = hostAddresses.at(i);
                }
            }
            _hosts[address.toString()] = host.split(":").last().toInt();
        }
    }
    else
    {
        QHostInfo info = QHostInfo::fromName(host);
        if (info.error() == QHostInfo::NoError)
        {
            // Set port according to default (same as local port)
            _hosts[info.addresses().first().toString()] = (int)_localPort;
        }
    }
}

void UDPConfiguration::addHost(const QString& host, int port)
{
    _hosts[host.trimmed()] = port;
}

void UDPConfiguration::removeHost(const QString& host)
{
    QString tHost = host;
    if (tHost.contains(":")) {
        tHost = tHost.split(":").first();
    }
    tHost = tHost.trimmed();
    QMap<QString, int>::iterator i = _hosts.find(tHost);
    if(i != _hosts.end()) {
        _hosts.erase(i);
    }
}

bool UDPConfiguration::firstHost(QString& host, int& port)
{
    _it = _hosts.begin();
    if(_it == _hosts.end()) {
        return false;
    }
    return nextHost(host, port);
}

bool UDPConfiguration::nextHost(QString& host, int& port)
{
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
    settings.beginGroup(root);
    _hosts.clear();
    _localPort = (quint16)settings.value("port", QGC_UDP_PORT).toUInt();
    int hostCount = settings.value("hostCount", 0).toInt();
    for(int i = 0; i < hostCount; i++) {
        QString hkey = QString("host%1").arg(i);
        QString pkey = QString("port%1").arg(i);
        if(settings.contains(hkey) && settings.contains(pkey)) {
            addHost(settings.value(hkey).toString(), settings.value(pkey).toInt());
        }
    }
    settings.endGroup();
    _confMutex.unlock();
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
