/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

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
#include <iostream>
#include "UDPLink.h"
#include "LinkManager.h"
#include "MG.h"

UDPLink::UDPLink(QHostAddress host, quint16 port)
{
    this->host = host;
    this->port = port;
    this->connectState = false;
    this->hosts = new QList<QHostAddress>();
    //this->ports = new QMap<QHostAddress, quint16>();
    this->ports = new QList<quint16>();

    // Set unique ID and add link to the list of links
    this->id = getNextLinkId();
    this->name = tr("UDP link ") + QString::number(getId());
    LinkManager::instance()->add(this);
}

UDPLink::~UDPLink()
{
    disconnect();
}

/**
 * @brief Runs the thread
 *
 **/
void UDPLink::run()
{
}

void UDPLink::setAddress(QString address)
{
    Q_UNUSED(address);
    // FIXME TODO Implement address
    //socket->setLocalAddress(QHostAddress(address));
}

void UDPLink::setPort(quint16 port)
{
    this->port = port;
}


void UDPLink::writeBytes(const char* data, qint64 size)
{
    // Broadcast to all connected systems
    //QList<QHostAddress>::iterator h;
    // for (h = hosts->begin(); h != hosts->end(); ++h)

    for (int h = 0; h < hosts->size(); h++)
    {
        QHostAddress currentHost = hosts->at(h);
        quint16 currentPort = ports->at(h);
        //        QList<quint16> currentPorts = ports->values(currentHost);
        //        for (int p = 0; p < currentPorts.size(); p++)
        //        {
        //            quint16 currentPort = currentPorts.at(p);
        //qDebug() << "Sent message to " << currentHost << ":" << currentPort << "at" << __FILE__ << ":" << __LINE__;
        socket->writeDatagram(data, size, currentHost, currentPort);
        //        }
    }


    //if(socket->write(data, size) > 0) {

//    qDebug() << "Transmitted " << size << "bytes:";
//
//    /* Increase write counter */
//    bitsSentTotal += size * 8;
//
//    int i;
//    for (i=0; i<size; i++){
//        unsigned int v=data[i];
//
//        fprintf(stderr,"%02x ", v);
//    }
    //}
}

/**
 * @brief Read a number of bytes from the interface.
 *
 * @param data Pointer to the data byte array to write the bytes to
 * @param maxLength The maximum number of bytes to write
 **/
void UDPLink::readBytes()
{
    const qint64 maxLength = 2048;
    char data[maxLength];
    QHostAddress sender;
    quint16 senderPort;

    unsigned int s = socket->pendingDatagramSize();
    if (s > maxLength) std::cerr << __FILE__ << __LINE__ << " UDP datagram overflow, allowed to read less bytes than datagram size" << std::endl;
    socket->readDatagram(data, maxLength, &sender, &senderPort);

    // FIXME TODO Check if this method is better than retrieving the data by individual processes
    QByteArray b(data, s);
    emit bytesReceived(this, b);

//    // Echo data for debugging purposes
//    std::cerr << __FILE__ << __LINE__ << "Received datagram:" << std::endl;
//    int i;
//    for (i=0; i<s; i++)
//    {
//        unsigned int v=data[i];
//        fprintf(stderr,"%02x ", v);
//    }
//    std::cerr << std::endl;


    // Add host to broadcast list if not yet present
    if (!hosts->contains(sender))
    {
        hosts->append(sender);
        ports->append(senderPort);
        //        ports->insert(sender, senderPort);
    }
    else
    {
        int index = hosts->indexOf(sender);
        ports->replace(index, senderPort);
    }

}


/**
 * @brief Get the number of bytes to read.
 *
 * @return The number of bytes to read
 **/
qint64 UDPLink::bytesAvailable() {
    return socket->pendingDatagramSize();
}

/**
 * @brief Disconnect the connection.
 *
 * @return True if connection has been disconnected, false if connection couldn't be disconnected.
 **/
bool UDPLink::disconnect()
{
    delete socket;
    socket = NULL;

    connectState = false;

    emit disconnected();
    emit connected(false);
    return !connectState;
}

/**
 * @brief Connect the connection.
 *
 * @return True if connection has been established, false if connection couldn't be established.
 **/
bool UDPLink::connect()
{
    socket = new QUdpSocket(this);

    //QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
    QObject::connect(socket, SIGNAL(readyRead()), this, SLOT(readBytes()));

    connectState = socket->bind(host, port);

    emit connected(connectState);
    if (connectState)
    {
        emit connected();
        connectionStartTime = MG::TIME::getGroundTimeNow();
    }

    start(HighPriority);
    return connectState;
}

/**
 * @brief Check if connection is active.
 *
 * @return True if link is connected, false otherwise.
 **/
bool UDPLink::isConnected() {
    return connectState;
}

int UDPLink::getId()
{
    return id;
}

QString UDPLink::getName()
{
    return name;
}

void UDPLink::setName(QString name)
{
    this->name = name;
    emit nameChanged(this->name);
}


qint64 UDPLink::getNominalDataRate() {
    return 54000000; // 54 Mbit
}

qint64 UDPLink::getTotalUpstream() {
    statisticsMutex.lock();
    qint64 totalUpstream = bitsSentTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
    return totalUpstream;
}

qint64 UDPLink::getCurrentUpstream() {
    return 0; // TODO
}

qint64 UDPLink::getMaxUpstream() {
    return 0; // TODO
}

qint64 UDPLink::getBitsSent() {
    return bitsSentTotal;
}

qint64 UDPLink::getBitsReceived() {
    return bitsReceivedTotal;
}

qint64 UDPLink::getTotalDownstream() {
    statisticsMutex.lock();
    qint64 totalDownstream = bitsReceivedTotal / ((MG::TIME::getGroundTimeNow() - connectionStartTime) / 1000);
    statisticsMutex.unlock();
    return totalDownstream;
}

qint64 UDPLink::getCurrentDownstream() {
    return 0; // TODO
}

qint64 UDPLink::getMaxDownstream() {
    return 0; // TODO
}

bool UDPLink::isFullDuplex() {
    return true;
}

int UDPLink::getLinkQuality() {
    /* This feature is not supported with this interface */
    return -1;
}
