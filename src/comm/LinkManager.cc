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
        new LinkManager(qgcApp());
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
    _connectionsSuspended(false),
    _mavlink(NULL)
{
    Q_ASSERT(_instance == NULL);
    _instance = this;
    
    _mavlink = new MAVLinkProtocol;
    Q_CHECK_PTR(_mavlink);
}

LinkManager::~LinkManager()
{
    disconnectAll();
    
    foreach (LinkInterface* link, _links) {
        Q_ASSERT(link);
        deleteLink(link);
    }
    _links.clear();
    
    // Clear out the queue so disconnects make it all the way through threads
    qgcApp()->processEvents(QEventLoop::ExcludeUserInputEvents);
    
    delete _mavlink;
}

void LinkManager::addLink(LinkInterface* link)
{
    Q_ASSERT(link);
    
    // Take ownership for delete
    link->_ownedByLinkManager = true;
    
    _linkListMutex.lock();
    
    if (!_links.contains(link)) {
        _links.append(link);
        _linkListMutex.unlock();
        emit newLink(link);
    } else {
        _linkListMutex.unlock();
    }
    
    // MainWindow may be around when doing things like running unit tests
    if (MainWindow::instance()) {
        connect(link, SIGNAL(communicationError(QString,QString)), MainWindow::instance(), SLOT(showCriticalMessage(QString,QString)), Qt::QueuedConnection);
    }
    
    connect(link, &LinkInterface::bytesReceived, _mavlink, &MAVLinkProtocol::receiveBytes);
    connect(link, &LinkInterface::connected, _mavlink, &MAVLinkProtocol::linkConnected);
    connect(link, &LinkInterface::disconnected, _mavlink, &MAVLinkProtocol::linkDisconnected);
    _mavlink->resetMetadataForLink(link);
}

bool LinkManager::connectAll()
{
    if (_connectionsSuspendedMsg()) {
        return false;
    }
    
    bool allConnected = true;

    _linkListMutex.lock();
    foreach (LinkInterface* link, _links) {
        Q_ASSERT(link);
        if (!link->_connect()) {
            allConnected = false;
        }
    }
    _linkListMutex.unlock();

    return allConnected;
}

bool LinkManager::disconnectAll()
{
    bool allDisconnected = true;

    _linkListMutex.lock();
    foreach (LinkInterface* link, _links)
    {
        Q_ASSERT(link);
        if (!link->_disconnect()) {
            allDisconnected = false;
        }
    }
    _linkListMutex.unlock();

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
    
    _linkListMutex.lock();
    
    Q_ASSERT(_links.contains(link));
    _links.removeOne(link);
    Q_ASSERT(!_links.contains(link));

    _linkListMutex.unlock();
             
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
    _linkListMutex.lock();
    QList<LinkInterface*> ret(_links);
    _linkListMutex.unlock();
    return ret;
}

const QList<SerialLink*> LinkManager::getSerialLinks()
{
    _linkListMutex.lock();
    QList<SerialLink*> s;

    foreach (LinkInterface* link, _links)
    {
        Q_ASSERT(link);
        
        SerialLink* serialLink = qobject_cast<SerialLink*>(link);

        if (serialLink)
            s.append(serialLink);
    }
    _linkListMutex.unlock();

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
