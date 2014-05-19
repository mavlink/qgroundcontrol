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
#include "LinkManager.h"
#include <iostream>

#include <QDebug>

LinkManager* LinkManager::instance()
{
    static LinkManager* _instance = 0;
    if(_instance == 0) {
        _instance = new LinkManager();

        /* Set the application as parent to ensure that this object
         * will be destroyed when the main application exits */
        _instance->setParent(qApp);
    }
    return _instance;
}

/**
 * @brief Private singleton constructor
 *
 * This class implements the singleton design pattern and has therefore only a private constructor.
 **/
LinkManager::LinkManager()
{
    links = QList<LinkInterface*>();
    protocolLinks = QMap<ProtocolInterface*, LinkInterface*>();
}

LinkManager::~LinkManager()
{
    disconnectAll();
    dataMutex.lock();
    foreach (LinkInterface* link, links)
    {
        if(link) link->deleteLater();
    }
    dataMutex.unlock();
}

void LinkManager::add(LinkInterface* link)
{
    dataMutex.lock();
    if (!links.contains(link))
    {
        if(!link) return;
        connect(link, SIGNAL(destroyed(QObject*)), this, SLOT(removeObj(QObject*)));
        links.append(link);
        dataMutex.unlock();
        emit newLink(link);
    } else {
        dataMutex.unlock();
    }
}

void LinkManager::addProtocol(LinkInterface* link, ProtocolInterface* protocol)
{
    // Connect link to protocol
    // the protocol will receive new bytes from the link
    if (!link || !protocol) return;

    dataMutex.lock();
    QList<LinkInterface*> linkList = protocolLinks.values(protocol);

    // If protocol has not been added before (list length == 0)
    // OR if link has not been added to protocol, add
    if (!linkList.contains(link))
    {
        // Protocol is new, add
        connect(link, SIGNAL(bytesReceived(LinkInterface*, QByteArray)), protocol, SLOT(receiveBytes(LinkInterface*, QByteArray)));
        // Add status
        connect(link, SIGNAL(connected(bool)), protocol, SLOT(linkStatusChanged(bool)));
        // Store the connection information in the protocol links map
        protocolLinks.insertMulti(protocol, link);
        dataMutex.unlock();
        // Make sure the protocol clears its metadata for this link.
        protocol->resetMetadataForLink(link);
    } else {
        dataMutex.unlock();
    }
    //qDebug() << __FILE__ << __LINE__ << "ADDED LINK TO PROTOCOL" << link->getName() << protocol->getName() << "NEW SIZE OF LINK LIST:" << protocolLinks.size();
}

QList<LinkInterface*> LinkManager::getLinksForProtocol(ProtocolInterface* protocol)
{
    dataMutex.lock();
    QList<LinkInterface*> links = protocolLinks.values(protocol);
    dataMutex.unlock();
    return links;
}

ProtocolInterface* LinkManager::getProtocolForLink(LinkInterface* link)
{
    dataMutex.lock();
    ProtocolInterface* interface = protocolLinks.key(link);
    dataMutex.unlock();
    return interface;
}

bool LinkManager::connectAll()
{
    bool allConnected = true;

    dataMutex.lock();
    foreach (LinkInterface* link, links)
    {
        if(!link) {}
        else if(!link->connect()) allConnected = false;
    }
    dataMutex.unlock();

    return allConnected;
}

bool LinkManager::disconnectAll()
{
    bool allDisconnected = true;

    dataMutex.lock();
    foreach (LinkInterface* link, links)
    {
        //static int i=0;
        if(!link) {}
        else if(!link->disconnect()) allDisconnected = false;
    }
    dataMutex.unlock();

    return allDisconnected;
}

bool LinkManager::connectLink(LinkInterface* link)
{
    if(!link) return false;
    return link->connect();
}

bool LinkManager::disconnectLink(LinkInterface* link)
{
    if(!link) return false;
    return link->disconnect();
}

void LinkManager::removeObj(QObject* link)
{
    LinkInterface* linkInterface = dynamic_cast<LinkInterface*>(link);
    if (linkInterface)
    {
        removeLink(linkInterface);
    }
}

bool LinkManager::removeLink(LinkInterface* link)
{
    if(link)
    {
        dataMutex.lock();
        for (int i=0; i < QList<LinkInterface*>(links).size(); i++)
        {
            if(link==links.at(i))
            {
                links.removeAt(i); //remove from link list
            }
        }
        // Remove link from protocol map
        QList<ProtocolInterface* > protocols = protocolLinks.keys(link);
        foreach (ProtocolInterface* proto, protocols)
        {
            protocolLinks.remove(proto, link);
        }
        dataMutex.unlock();

        // Emit removal of link
        emit linkRemoved(link);

        return true;
    }
    return false;
}

/**
 * The access time is linear in the number of links.
 *
 * @param id link identifier to search for
 * @return A pointer to the link or NULL if not found
 */
LinkInterface* LinkManager::getLinkForId(int id)
{
    dataMutex.lock();
    LinkInterface* linkret = NULL;
    foreach (LinkInterface* link, links)
    {
        if (link->getId() == id)
        {
            linkret = link;
        }
    }
    dataMutex.unlock();
    return linkret;
}

/**
 *
 */
const QList<LinkInterface*> LinkManager::getLinks()
{
    dataMutex.lock();
    QList<LinkInterface*> ret(links);
    dataMutex.unlock();
    return ret;
}

const QList<SerialLink*> LinkManager::getSerialLinks()
{
    dataMutex.lock();
    QList<SerialLink*> s;

    foreach (LinkInterface* i, links)
    {
        SerialLink* link = qobject_cast<SerialLink*>(i);

        if (link)
            s.append(link);
    }
    dataMutex.unlock();

    return s;
}
