/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Brief Description
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QList>
#include <QApplication>
#include <QDebug>

#include "LinkManager.h"
#include "MainWindow.h"
#include "QGCMessageBox.h"
#include "QGCApplication.h"

LinkManager* LinkManager::_instance = NULL;

LinkManager* LinkManager::instance(void)
{
    if(_instance == 0) {
        _instance = new LinkManager(qgcApp());
        Q_CHECK_PTR(_instance);
    }
    
    Q_ASSERT(_instance);
    
    return _instance;
}

void LinkManager::deleteInstance(void)
{
    _instance = NULL;
    delete this;
}

/**
 * @brief Private singleton constructor
 *
 * This class implements the singleton design pattern and has therefore only a private constructor.
 **/
LinkManager::LinkManager(QObject* parent, bool registerSingleton) :
    QGCSingleton(parent, registerSingleton),
    _connectionsSuspended(false)
{
    _links = QList<LinkInterface*>();
    _protocolLinks = QMap<ProtocolInterface*, LinkInterface*>();
}

LinkManager::~LinkManager()
{
    disconnectAll();
    
    foreach (LinkInterface* link, _links) {
        Q_ASSERT(link);
        deleteLink(link);
    }
    _links.clear();
}

void LinkManager::add(LinkInterface* link)
{
    Q_ASSERT(link);
    
    // Take ownership for delete
    link->_ownedByLinkManager = true;
    
    _dataMutex.lock();
    
    if (!_links.contains(link)) {
        _links.append(link);
        _dataMutex.unlock();
        emit newLink(link);
    } else {
        _dataMutex.unlock();
    }
}

void LinkManager::addProtocol(LinkInterface* link, ProtocolInterface* protocol)
{
    Q_ASSERT(link);
    Q_ASSERT(protocol);
    
    // Connect link to protocol
    // the protocol will receive new bytes from the link
    _dataMutex.lock();
    QList<LinkInterface*> linkList = _protocolLinks.values(protocol);

    // If protocol has not been added before (list length == 0)
    // OR if link has not been added to protocol, add
    if (!linkList.contains(link))
    {
        // Protocol is new, add
        connect(link, SIGNAL(bytesReceived(LinkInterface*, QByteArray)), protocol, SLOT(receiveBytes(LinkInterface*, QByteArray)));
        // Add status
        connect(link, SIGNAL(connected(bool)), protocol, SLOT(linkStatusChanged(bool)));
        // Store the connection information in the protocol links map
        _protocolLinks.insertMulti(protocol, link);
        _dataMutex.unlock();
        // Make sure the protocol clears its metadata for this link.
        protocol->resetMetadataForLink(link);
    } else {
        _dataMutex.unlock();
    }
    //qDebug() << __FILE__ << __LINE__ << "ADDED LINK TO PROTOCOL" << link->getName() << protocol->getName() << "NEW SIZE OF LINK LIST:" << _protocolLinks.size();
}

QList<LinkInterface*> LinkManager::getLinksForProtocol(ProtocolInterface* protocol)
{
    _dataMutex.lock();
    QList<LinkInterface*> links = _protocolLinks.values(protocol);
    _dataMutex.unlock();
    return links;
}

ProtocolInterface* LinkManager::getProtocolForLink(LinkInterface* link)
{
    _dataMutex.lock();
    ProtocolInterface* protocol = _protocolLinks.key(link);
    _dataMutex.unlock();
	return protocol;
}

bool LinkManager::connectAll()
{
    if (_connectionsSuspendedMsg()) {
        return false;
    }
    
    bool allConnected = true;

    _dataMutex.lock();
    foreach (LinkInterface* link, _links) {
        Q_ASSERT(link);
        if (!link->_connect()) {
            allConnected = false;
        }
    }
    _dataMutex.unlock();

    return allConnected;
}

bool LinkManager::disconnectAll()
{
    bool allDisconnected = true;

    _dataMutex.lock();
    foreach (LinkInterface* link, _links)
    {
        Q_ASSERT(link);
        if (!link->_disconnect()) {
            allDisconnected = false;
        }
    }
    _dataMutex.unlock();

    return allDisconnected;
}

bool LinkManager::connectLink(LinkInterface* link)
{
    Q_ASSERT(link);
    
    if (_connectionsSuspendedMsg()) {
        return false;
    }

    return link->_connect();
}

bool LinkManager::disconnectLink(LinkInterface* link)
{
    Q_ASSERT(link);
    return link->_disconnect();
}

void LinkManager::deleteLink(LinkInterface* link)
{
    Q_ASSERT(link);
    
    _dataMutex.lock();
    
    Q_ASSERT(_links.contains(link));
    _links.removeOne(link);
    Q_ASSERT(!_links.contains(link));

    // Remove link from protocol map
    QList<ProtocolInterface* > protocols = _protocolLinks.keys(link);
    foreach (ProtocolInterface* proto, protocols) {
        _protocolLinks.remove(proto, link);
    }
             
    _dataMutex.unlock();
             
    // Emit removal of link
    emit linkDeleted(link);
    
    Q_ASSERT(link->_ownedByLinkManager);
    link->_deletedByLinkManager = true;   // Signal that this is a valid delete
    delete link;
}

/**
 *
 */
const QList<LinkInterface*> LinkManager::getLinks()
{
    _dataMutex.lock();
    QList<LinkInterface*> ret(_links);
    _dataMutex.unlock();
    return ret;
}

const QList<SerialLink*> LinkManager::getSerialLinks()
{
    _dataMutex.lock();
    QList<SerialLink*> s;

    foreach (LinkInterface* link, _links)
    {
        Q_ASSERT(link);
        
        SerialLink* serialLink = qobject_cast<SerialLink*>(link);

        if (serialLink)
            s.append(serialLink);
    }
    _dataMutex.unlock();

    return s;
}

/// @brief If all new connections should be suspended a message is displayed to the user and true
///         is returned;
bool LinkManager::_connectionsSuspendedMsg(void)
{
    if (_connectionsSuspended) {
        QGCMessageBox::information(tr("Connect not allowed"),
                                   tr("Connect not allowed: %1").arg(_connectionsSuspendedReason));
        return true;
    } else {
        return false;
    }
}

void LinkManager::setConnectionsSuspended(QString reason)
{
    _connectionsSuspended = true;
    _connectionsSuspendedReason = reason;
    Q_ASSERT(!reason.isEmpty());
}
