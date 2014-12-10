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

/// @file
///     @author Lorenz Meier <mavteam@student.ethz.ch>

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
#include "MAVLinkProtocol.h"

class LinkManagerTest;

/// Manage communication links
///
/// The Link Manager organizes the physical Links. It can manage arbitrary
/// links and takes care of connecting them as well assigning the correct
/// protocol instance to transport the link data into the application.

class LinkManager : public QGCSingleton
{
    Q_OBJECT

public:
    /// @brief Returns the LinkManager singleton
    static LinkManager* instance(void);
    
    virtual void deleteInstance(void);

    ~LinkManager();

    /// Returns list of all links
    const QList<LinkInterface*> getLinks();

    // Returns list of all serial links
    const QList<SerialLink*> getSerialLinks();

    /// Sets the flag to suspend the all new connections
    ///     @param reason User visible reason to suspend connections
    void setConnectionsSuspended(QString reason);
    
    /// Sets the flag to allow new connections to be made
    void setConnectionsAllowed(void) { _connectionsSuspended = false; }
    
    /// Adds the link to the LinkManager. LinkManager takes ownership of this object. To delete
    /// it, call LinkManager::deleteLink.
    void addLink(LinkInterface* link);
    
    /// Deletes the specified link. Will disconnect if connected.
    void deleteLink(LinkInterface* link);
    
    /// Re-connects all existing links
    bool connectAll();
    
    /// Disconnects all existing links
    bool disconnectAll();
    
    /// Connect the specified link
    bool connectLink(LinkInterface* link);
    
    /// Disconnect the specified link
    bool disconnectLink(LinkInterface* link);
    
    /// Returns the one mavlink protocol object in the system
    MAVLinkProtocol* mavlink(void) { return _mavlink; }
    
signals:
    void newLink(LinkInterface* link);
    void linkDeleted(LinkInterface* link);
    
private:
    /// All access to LinkManager is through LinkManager::instance
    LinkManager(QObject* parent = NULL, bool registerSingleton = true);
    
    /// LinkManager unit test is allowed to new LinkManager objects
    friend class LinkManagerTest;
    
    bool _connectionsSuspendedMsg(void);
    
    static LinkManager* _instance;  /// LinkManager singleton
    
    QList<LinkInterface*>   _links;         ///< List of available links
    QMutex                  _linkListMutex; ///< Mutex for thread safe access to _links list
    
    
    bool    _connectionsSuspended;          ///< true: all new connections should not be allowed
    QString _connectionsSuspendedReason;    ///< User visible reason for suspension
    
    MAVLinkProtocol* _mavlink;  ///< The one and only mavlink protocol
};

#endif
