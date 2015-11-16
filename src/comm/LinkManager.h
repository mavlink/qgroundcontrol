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
#include "QGCLoggingCategory.h"
#include "QGCToolbox.h"

// Links
#ifndef __ios__
#include "SerialLink.h"
#endif
#include "UDPLink.h"
#include "TCPLink.h"
#include "LogReplayLink.h"

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

#include "ProtocolInterface.h"
#include "MAVLinkProtocol.h"

Q_DECLARE_LOGGING_CATEGORY(LinkManagerLog)
Q_DECLARE_LOGGING_CATEGORY(LinkManagerVerboseLog)

class QGCApplication;

/// Manage communication links
///
/// The Link Manager organizes the physical Links. It can manage arbitrary
/// links and takes care of connecting them as well assigning the correct
/// protocol instance to transport the link data into the application.

class LinkManager : public QGCTool
{
    Q_OBJECT

    /// Unit Test has access to private constructor/destructor
    friend class LinkManagerTest;

public:
    LinkManager(QGCApplication* app);
    ~LinkManager();

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
#ifndef __ios__
    const QList<SerialLink*> getSerialLinks();
#endif
    /// Sets the flag to suspend the all new connections
    ///     @param reason User visible reason to suspend connections
    void setConnectionsSuspended(QString reason);

    /// Sets the flag to allow new connections to be made
    void setConnectionsAllowed(void) { _connectionsSuspended = false; }

    /// Creates, connects (and adds) a link  based on the given configuration instance.
    /// Link takes ownership of config.
    LinkInterface* createConnectedLink(LinkConfiguration* config, bool persistenLink = false);

    /// Creates, connects (and adds) a link  based on the given configuration name.
    LinkInterface* createConnectedLink(const QString& name, bool persistenLink = false);

    /// Returns true if the link manager is holding this link
    bool containsLink(LinkInterface* link);
    
    /// Returns the QSharedPointer for this link. You must use SharedLinkInterface if you are going to
    /// keep references to a link in a thread other than the main ui thread.
    SharedLinkInterface& sharedPointerForLink(LinkInterface* link);

    /// Disconnects all existing links, including persistent links.
    ///     @param disconnectPersistentLink See disconnectLink
    void disconnectAll(bool disconnectPersistentLink);

    /// Connect the specified link
    bool connectLink(LinkInterface* link);

    /// Disconnect the specified link.
    ///     @param disconnectPersistentLink
    ///                 true: link is disconnected no matter what type
    ///                 false: if persistent link, link is marked as inactive and linkInactive is signalled
    ///                 false: if not persistent link, link is disconnected
    bool disconnectLink(LinkInterface* link, bool disconnectPersistentLink);
    
    /// Called to notify that a heartbeat was received with the specified information. Will transition
    /// a link to active as needed.
    ///     @param link Heartbeat came through on this link
    ///     @param vehicleId Mavlink system id for vehicle
    ///     @param heartbeat Mavlink heartbeat message
    /// @return true: continue further processing of this message, false: disregard this message
    bool notifyHeartbeatInfo(LinkInterface* link, int vehicleId, mavlink_heartbeat_t& heartbeat);

    bool anyConnectedLinks(void);
    bool anyActiveLinks(void);
    
    // The following APIs are public but should not be called in normal use. The are mainly exposed
    // here for unit test code.
    void _deleteLink(LinkInterface* link);
    void _addLink(LinkInterface* link);

    // Called to signal app shutdown. Disconnects all links while turning off auto-connect.
    void shutdown(void);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

signals:
    void newLink(LinkInterface* link);

    // Link has been deleted. You may not necessarily get a linkInactive before the link is deleted.
    void linkDeleted(LinkInterface* link);

    // Link has been connected, but no Vehicle seen on link yet.
    void linkConnected(LinkInterface* link);

    // Link disconnected, all vehicles on link should be gone as well.
    void linkDisconnected(LinkInterface* link);

    // New vehicle has been seen on the link.
    void linkActive(LinkInterface* link, int vehicleId, int vehicleFirmwareType, int vehicleType);

    // No longer hearing from any vehicles on this link.
    void linkInactive(LinkInterface* link);

    void linkConfigurationChanged();

private slots:
    void _linkConnected(void);
    void _linkDisconnected(void);
    void _vehicleHeartbeatInfo(LinkInterface* link, int vehicleId, int vehicleMavlinkVersion, int vehicleFirmwareType, int vehicleType);

private:
    bool _connectionsSuspendedMsg(void);
    void _updateAutoConnectLinks(void);
#ifndef __ios__
    SerialConfiguration* _findSerialConfiguration(const QString& portName);
#endif
    QList<LinkConfiguration*>   _linkConfigurations;    ///< List of configured links
    
    /// List of available links kept as QSharedPointers. We use QSharedPointer since
    /// there are other objects that maintain copies of these links in other threads.
    /// The reference counting allows for orderly deletion.
    QList<SharedLinkInterface>  _links;
    
    QMutex                      _linkListMutex;         ///< Mutex for thread safe access to _links list

    bool    _configUpdateSuspended;                     ///< true: stop updating configuration list
    bool    _configurationsLoaded;                      ///< true: Link configurations have been loaded
    bool    _connectionsSuspended;                      ///< true: all new connections should not be allowed
    QString _connectionsSuspendedReason;                ///< User visible reason for suspension
#ifndef __ios__
    QTimer  _portListTimer;
#endif
    uint32_t _mavlinkChannelsUsedBitMask;
    
    SharedLinkInterface _nullSharedLink;

    MAVLinkProtocol*    _mavlinkProtocol;
    QList<int>          _ignoreVehicleIds;  ///< List of vehicle id for which we ignore further communication
    bool                _allowAutoConnect;
};

#endif
