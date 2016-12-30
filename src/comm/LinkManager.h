/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
#include "ProtocolInterface.h"
#include "MAVLinkProtocol.h"
#if !defined(__mobile__)
#include "LogReplayLink.h"
#endif
#include "QmlObjectListModel.h"

#ifndef NO_SERIAL_LINK
    #include "SerialLink.h"
#endif

#ifdef QT_DEBUG
    #include "MockLink.h"
#endif

class UDPConfiguration;

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

    Q_PROPERTY(bool autoconnectUDP                      READ autoconnectUDP                     WRITE setAutoconnectUDP         NOTIFY autoconnectUDPChanged)
    Q_PROPERTY(bool autoconnectPixhawk                  READ autoconnectPixhawk                 WRITE setAutoconnectPixhawk     NOTIFY autoconnectPixhawkChanged)
    Q_PROPERTY(bool autoconnect3DRRadio                 READ autoconnect3DRRadio                WRITE setAutoconnect3DRRadio    NOTIFY autoconnect3DRRadioChanged)
    Q_PROPERTY(bool autoconnectPX4Flow                  READ autoconnectPX4Flow                 WRITE setAutoconnectPX4Flow     NOTIFY autoconnectPX4FlowChanged)
    Q_PROPERTY(bool autoconnectRTKGPS                   READ autoconnectRTKGPS                  WRITE setAutoconnectRTKGPS      NOTIFY autoconnectRTKGPSChanged)
    Q_PROPERTY(bool autoconnectLibrePilot               READ autoconnectLibrePilot              WRITE setAutoconnectLibrePilot  NOTIFY autoconnectLibrePilotChanged)
    Q_PROPERTY(bool isBluetoothAvailable                READ isBluetoothAvailable                                               CONSTANT)

    Q_PROPERTY(QmlObjectListModel*  linkConfigurations  READ _qmlLinkConfigurations                                             NOTIFY linkConfigurationsChanged)
    Q_PROPERTY(QStringList          linkTypeStrings     READ linkTypeStrings                                                    CONSTANT)
    Q_PROPERTY(QStringList          serialBaudRates     READ serialBaudRates                                                    CONSTANT)
    Q_PROPERTY(QStringList          serialPortStrings   READ serialPortStrings                                                  NOTIFY commPortStringsChanged)
    Q_PROPERTY(QStringList          serialPorts         READ serialPorts                                                        NOTIFY commPortsChanged)

    // Create/Edit Link Configuration
    Q_INVOKABLE LinkConfiguration*  createConfiguration         (int type, const QString& name);
    Q_INVOKABLE LinkConfiguration*  startConfigurationEditing   (LinkConfiguration* config);
    Q_INVOKABLE void                cancelConfigurationEditing  (LinkConfiguration* config) { delete config; }
    Q_INVOKABLE bool                endConfigurationEditing     (LinkConfiguration* config, LinkConfiguration* editedConfig);
    Q_INVOKABLE bool                endCreateConfiguration      (LinkConfiguration* config);
    Q_INVOKABLE void                removeConfiguration         (LinkConfiguration* config);

    // Property accessors

    bool autoconnectUDP             (void)  { return _autoconnectUDP; }
    bool autoconnectPixhawk         (void)  { return _autoconnectPixhawk; }
    bool autoconnect3DRRadio        (void)  { return _autoconnect3DRRadio; }
    bool autoconnectPX4Flow         (void)  { return _autoconnectPX4Flow; }
    bool autoconnectRTKGPS          (void)  { return _autoconnectRTKGPS; }
    bool autoconnectLibrePilot      (void)  { return _autoconnectLibrePilot; }
    bool isBluetoothAvailable       (void);

    QList<LinkInterface*> links                 (void);
    QStringList         linkTypeStrings         (void) const;
    QStringList         serialBaudRates         (void);
    QStringList         serialPortStrings       (void);
    QStringList         serialPorts             (void);

    void setAutoconnectUDP        (bool autoconnect);
    void setAutoconnectPixhawk    (bool autoconnect);
    void setAutoconnect3DRRadio   (bool autoconnect);
    void setAutoconnectPX4Flow    (bool autoconnect);
    void setAutoconnectRTKGPS     (bool autoconnect);
    void setAutoconnectLibrePilot (bool autoconnect);

    /// Load list of link configurations from disk
    void loadLinkConfigurationList();

    /// Save list of link configurations from disk
    void saveLinkConfigurationList();

    /// Suspend automatic confguration updates (during link maintenance for instance)
    void suspendConfigurationUpdates(bool suspend);

    /// Sets the flag to suspend the all new connections
    ///     @param reason User visible reason to suspend connections
    void setConnectionsSuspended(QString reason);

    /// Sets the flag to allow new connections to be made
    void setConnectionsAllowed(void) { _connectionsSuspended = false; }

    /// Creates, connects (and adds) a link  based on the given configuration instance.
    /// Link takes ownership of config.
    LinkInterface* createConnectedLink(SharedLinkConfigurationPointer& config);

    // This should only be used by Qml code
    Q_INVOKABLE void createConnectedLink(LinkConfiguration* config);

    /// Creates, connects (and adds) a link  based on the given configuration name.
    LinkInterface* createConnectedLink(const QString& name);

    /// Disconnects all existing links
    void disconnectAll(void);

    /// Connect the specified link
    bool connectLink(LinkInterface* link);

    /// Disconnect the specified link
    Q_INVOKABLE void disconnectLink(LinkInterface* link);

    // The following APIs are public but should not be called in normal use. The are mainly exposed
    // here for unit test code.
    void _deleteLink(LinkInterface* link);
    void _addLink(LinkInterface* link);

    // Called to signal app shutdown. Disconnects all links while turning off auto-connect.
    Q_INVOKABLE void shutdown(void);

#ifdef QT_DEBUG
    // Only used by unit test tp restart after a shutdown
    void restart(void) { setConnectionsAllowed(); }
#endif

    /// @return true: specified link is an autoconnect link
    bool isAutoconnectLink(LinkInterface* link);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    /// @return This mavlink channel is never assigned to a vehicle.
    uint8_t reservedMavlinkChannel(void) { return 0; }

    /// If you are going to hold a reference to a LinkInterface* in your object you must reference count it
    /// by using this method to get access to the shared pointer.
    SharedLinkInterfacePointer sharedLinkInterfacePointerForLink(LinkInterface* link);

    bool containsLink(LinkInterface* link);

    SharedLinkConfigurationPointer addConfiguration(LinkConfiguration* config);

signals:
    void autoconnectUDPChanged        (bool autoconnect);
    void autoconnectPixhawkChanged    (bool autoconnect);
    void autoconnect3DRRadioChanged   (bool autoconnect);
    void autoconnectPX4FlowChanged    (bool autoconnect);
    void autoconnectRTKGPSChanged     (bool autoconnect);
    void autoconnectLibrePilotChanged (bool autoconnect);


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

    void commPortStringsChanged();
    void commPortsChanged();
    void linkConfigurationsChanged();

private slots:
    void _linkConnected(void);
    void _linkDisconnected(void);
    void _linkConnectionRemoved(LinkInterface* link);
#ifndef NO_SERIAL_LINK
    void _activeLinkCheck(void);
#endif

private:
    QmlObjectListModel* _qmlLinkConfigurations  (void) { return &_qmlConfigurations; }
    bool _connectionsSuspendedMsg(void);
    void _updateAutoConnectLinks(void);
    void _updateSerialPorts();
    void _fixUnnamed(LinkConfiguration* config);
    bool _setAutoconnectWorker(bool& currentAutoconnect, bool newAutoconnect, const char* autoconnectKey);
    void _removeConfiguration(LinkConfiguration* config);

#ifndef NO_SERIAL_LINK
    SerialConfiguration* _autoconnectConfigurationsContainsPort(const QString& portName);
#endif

    bool    _configUpdateSuspended;                     ///< true: stop updating configuration list
    bool    _configurationsLoaded;                      ///< true: Link configurations have been loaded
    bool    _connectionsSuspended;                      ///< true: all new connections should not be allowed
    QString _connectionsSuspendedReason;                ///< User visible reason for suspension
    QTimer  _portListTimer;
    uint32_t _mavlinkChannelsUsedBitMask;

    MAVLinkProtocol*    _mavlinkProtocol;


    QList<SharedLinkInterfacePointer>       _sharedLinks;
    QList<SharedLinkConfigurationPointer>   _sharedConfigurations;
    QList<SharedLinkConfigurationPointer>   _sharedAutoconnectConfigurations;
    QmlObjectListModel                      _qmlConfigurations;

    QMap<QString, int>  _autoconnectWaitList;   ///< key: QGCSerialPortInfo.systemLocation, value: wait count
    QStringList _commPortList;
    QStringList _commPortDisplayList;

    bool _autoconnectUDP;
    bool _autoconnectPixhawk;
    bool _autoconnect3DRRadio;
    bool _autoconnectPX4Flow;
    bool _autoconnectRTKGPS;
    bool _autoconnectLibrePilot;
#ifndef NO_SERIAL_LINK
    QTimer              _activeLinkCheckTimer;                  ///< Timer which checks for a vehicle showing up on a usb direct link
    QList<SerialLink*>  _activeLinkCheckList;                   ///< List of links we are waiting for a vehicle to show up on
    static const int    _activeLinkCheckTimeoutMSecs = 15000;   ///< Amount of time to wait for a heatbeat. Keep in mind ArduPilot stack heartbeat is slow to come.
#endif

    static const char*  _settingsGroup;
    static const char*  _autoconnectUDPKey;
    static const char*  _autoconnectPixhawkKey;
    static const char*  _autoconnect3DRRadioKey;
    static const char*  _autoconnectPX4FlowKey;
    static const char*  _autoconnectRTKGPSKey;
    static const char*  _autoconnectLibrePilotKey;
    static const char*  _defaultUPDLinkName;
    static const int    _autoconnectUpdateTimerMSecs;
    static const int    _autoconnectConnectDelayMSecs;
};

#endif
