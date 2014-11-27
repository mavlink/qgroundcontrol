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
 *   @brief Manage communication links
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef _LINKMANAGER_H_
#define _LINKMANAGER_H_

#include <QThread>
#include <QList>
#include <QMultiMap>
#include <QMutex>

#include "LinkInterface.h"
#include "SerialLink.h"
#include "ProtocolInterface.h"
#include "QGCSingleton.h"

class LinkManagerTest;

/**
 * The Link Manager organizes the physical Links. It can manage arbitrary
 * links and takes care of connecting them as well assigning the correct
 * protocol instance to transport the link data into the application.
 *
 **/
class LinkManager : public QGCSingleton
{
    Q_OBJECT

public:
    /// @brief Returns the LinkManager singleton
    static LinkManager* instance(void);
    
    virtual void deleteInstance(void);

    ~LinkManager();

    QList<LinkInterface*> getLinksForProtocol(ProtocolInterface* protocol);

    ProtocolInterface* getProtocolForLink(LinkInterface* link);

    /** @brief Get a list of all links */
    const QList<LinkInterface*> getLinks();

    /** @brief Get a list of all serial links */
    const QList<SerialLink*> getSerialLinks();

    /** @brief Get a list of all protocols */
    const QList<ProtocolInterface*> getProtocols() {
        return _protocolLinks.uniqueKeys();
    }
    
    /// @brief Sets the lag to suspend the all new connections
    ///     @param reason User visible reason to suspend connections
    void setConnectionsSuspended(QString reason);
    
    /// @brief Sets the flag to allow new connections to be made
    void setConnectionsAllowed(void) { _connectionsSuspended = false; }
    
    /// @brief Deletes the specified link. Will disconnect if connected.
    void deleteLink(LinkInterface* link);

public slots:
    /// @brief Adds the link to the LinkManager. LinkManager takes ownership of this object. To delete
    //          it, call LinkManager::deleteLink.
    void add(LinkInterface* link);
    
    void addProtocol(LinkInterface* link, ProtocolInterface* protocol);

    bool connectAll();
    bool connectLink(LinkInterface* link);

    bool disconnectAll();
    bool disconnectLink(LinkInterface* link);

signals:
    void newLink(LinkInterface* link);
    void linkDeleted(LinkInterface* link);
    
private:
    /// @brief All access to LinkManager is through LinkManager::instance
    LinkManager(QObject* parent = NULL, bool registerSingleton = true);
    
    // LinkManager unit test is allowed to new LinkManager objects
    friend class LinkManagerTest;
    
    static LinkManager* _instance;
    
    QList<LinkInterface*> _links;
    QMultiMap<ProtocolInterface*,LinkInterface*> _protocolLinks;
    QMutex _dataMutex;
    
    bool _connectionsSuspendedMsg(void);
    
    bool _connectionsSuspended;              ///< true: all new connections should not be allowed
    QString _connectionsSuspendedReason;     ///< User visible reason for suspension
};

#endif
