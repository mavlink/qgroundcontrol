/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2015 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

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

#include <QList>
#include <QMultiMap>
#include <QMutex>

#include "LinkConfiguration.h"
#include "LinkInterface.h"

// Links
#include "SerialLink.h"
#include "UDPLink.h"
#include "TCPLink.h"

#ifdef UNITTEST_BUILD
#include "MockLink.h"
#endif

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
    DECLARE_QGC_SINGLETON(LinkManager, LinkManager)

    /// Unit Test has access to private constructor/destructor
    friend class LinkManagerTest;

public:


    /*!
      Add a new link configuration setting to the list
      @param[in] link An instance of the link setting.
    */
    void addLinkConfiguration(LinkConfiguration* link);

    /*!
      Removes (and deletes) an existing link configuration setting from the list
      @param[in] link An instance of the link setting.
    */
    void removeLinkConfiguration(LinkConfiguration* link);

    /// Load list of link configurations from disk
    void loadLinkConfigurationList();

    /// Save list of link configurations from disk
    void saveLinkConfigurationList();

    /// Get a list of the configured links. This is the list of configured links that can be used by QGC.
    const QList<LinkConfiguration*> getLinkConfigurationList();

    /// Suspend automatic confguration updates (during link maintenance for instance)
    void suspendConfigurationUpdates(bool suspend);

    /// Returns list of all links
    const QList<LinkInterface*> getLinks();

    // Returns list of all serial links
    const QList<SerialLink*> getSerialLinks();

    /// Sets the flag to suspend the all new connections
    ///     @param reason User visible reason to suspend connections
    void setConnectionsSuspended(QString reason);

    /// Sets the flag to allow new connections to be made
    void setConnectionsAllowed(void) { _connectionsSuspended = false; }

    /// Creates (and adds) a link  based on the given configuration instance. LinkManager takes ownership of this object. To delete
    /// it, call LinkManager::deleteLink.
    LinkInterface* createLink(LinkConfiguration* config);

    /// Creates (and adds) a link  based on the given configuration name. LinkManager takes ownership of this object. To delete
    /// it, call LinkManager::deleteLink.
    LinkInterface* createLink(const QString& name);

    /// Adds the link to the LinkManager. LinkManager takes ownership of this object. To delete
    /// it, call LinkManager::deleteLink.
    void addLink(LinkInterface* link);

    /// Deletes the specified link. Will disconnect if connected.
    // TODO Will also crash if called. MAVLink protocol is not handling the disconnect properly.
    void deleteLink(LinkInterface* link);

    /// Re-connects all existing links
    bool connectAll();

    /// Disconnects all existing links
    bool disconnectAll();

    /// Connect the specified link
    bool connectLink(LinkInterface* link);

    /// Disconnect the specified link
    bool disconnectLink(LinkInterface* link);

signals:
    void newLink(LinkInterface* link);
    void linkDeleted(LinkInterface* link);
    void linkConnected(LinkInterface* link);
    void linkDisconnected(LinkInterface* link);
    void linkConfigurationChanged();

private slots:
    void _linkConnected(void);
    void _linkDisconnected(void);

private:
    /// All access to LinkManager is through LinkManager::instance
    LinkManager(QObject* parent = NULL);
    ~LinkManager();

    virtual void _shutdown(void);

    bool _connectionsSuspendedMsg(void);
    void _updateConfigurationList(void);
    SerialConfiguration* _findSerialConfiguration(const QString& portName);

    QList<LinkConfiguration*>   _linkConfigurations;    ///< List of configured links
    QList<LinkInterface*>       _links;                 ///< List of available links
    QMutex                      _linkListMutex;         ///< Mutex for thread safe access to _links list

    bool    _configUpdateSuspended;                     ///< true: stop updating configuration list
    bool    _configurationsLoaded;                      ///< true: Link configurations have been loaded
    bool    _connectionsSuspended;                      ///< true: all new connections should not be allowed
    QString _connectionsSuspendedReason;                ///< User visible reason for suspension
    QTimer  _portListTimer;
};

#endif
