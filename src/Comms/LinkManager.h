/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LinkConfiguration.h"
#include "LinkInterface.h"
#include "QGCToolbox.h"
#include "QmlObjectListModel.h"
#include "UdpIODevice.h"
#ifndef NO_SERIAL_LINK
    #include "QGCSerialPortInfo.h"
#endif

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#include <limits>

Q_DECLARE_LOGGING_CATEGORY(LinkManagerLog)
Q_DECLARE_LOGGING_CATEGORY(LinkManagerVerboseLog)

class QGCApplication;
class MAVLinkProtocol;
class UDPConfiguration;
class AutoConnectSettings;
class LogReplayLink;
#ifndef NO_SERIAL_LINK
    class SerialLink;
#endif

/// @brief Manage communication links
///
/// The Link Manager organizes the physical Links. It can manage arbitrary
/// links and takes care of connecting them as well assigning the correct
/// protocol instance to transport the link data into the application.

class LinkManager : public QGCTool
{
    Q_OBJECT

public:
    LinkManager(QGCApplication* app, QGCToolbox* toolbox);
    ~LinkManager();

    Q_PROPERTY(bool                 isBluetoothAvailable            READ isBluetoothAvailable            NOTIFY isBluetoothAvailableChanged)
    Q_PROPERTY(QmlObjectListModel*  linkConfigurations              READ _qmlLinkConfigurations          CONSTANT)
    Q_PROPERTY(QStringList          linkTypeStrings                 READ linkTypeStrings                 CONSTANT)
    Q_PROPERTY(bool                 mavlinkSupportForwardingEnabled READ mavlinkSupportForwardingEnabled NOTIFY mavlinkSupportForwardingEnabledChanged)

    /// Create/Edit Link Configuration
    Q_INVOKABLE LinkConfiguration*  createConfiguration                (int type, const QString& name);
    Q_INVOKABLE LinkConfiguration*  startConfigurationEditing          (LinkConfiguration* config);
    Q_INVOKABLE void                cancelConfigurationEditing         (LinkConfiguration* config) { delete config; }
    Q_INVOKABLE bool                endConfigurationEditing            (LinkConfiguration* config, LinkConfiguration* editedConfig);
    Q_INVOKABLE bool                endCreateConfiguration             (LinkConfiguration* config);
    Q_INVOKABLE void                removeConfiguration                (LinkConfiguration* config);
    Q_INVOKABLE void                createMavlinkForwardingSupportLink (void);

    // Called to signal app shutdown. Disconnects all links while turning off auto-connect.
    Q_INVOKABLE void shutdown(void);

    Q_INVOKABLE LogReplayLink* startLogReplay(const QString& logFile);

    // Property accessors

    bool isBluetoothAvailable       (void);

    QList<SharedLinkInterfacePtr>   links                           (void) { return _rgLinks; }
    QStringList                     linkTypeStrings                 (void) const;
    bool                            mavlinkSupportForwardingEnabled (void) { return _mavlinkSupportForwardingEnabled; }

    void loadLinkConfigurationList();
    void saveLinkConfigurationList();

    /// Suspend automatic confguration updates (during link maintenance for instance)
    void suspendConfigurationUpdates(bool suspend);

    /// Sets the flag to suspend the all new connections
    ///     @param reason User visible reason to suspend connections
    void setConnectionsSuspended(QString reason);

    /// Sets the flag to allow new connections to be made
    void setConnectionsAllowed(void) { _connectionsSuspended = false; }

    /// Creates, connects (and adds) a link  based on the given configuration instance.
    bool createConnectedLink(SharedLinkConfigurationPtr& config, bool isPX4Flow = false);

    // This should only be used by Qml code
    Q_INVOKABLE void createConnectedLink(LinkConfiguration* config);

    /// Returns pointer to the mavlink forwarding link, or nullptr if it does not exist
    SharedLinkInterfacePtr mavlinkForwardingLink();

    /// Returns pointer to the mavlink support forwarding link, or nullptr if it does not exist
    SharedLinkInterfacePtr mavlinkForwardingSupportLink();

    /// Re-initilize the mavlink signing for all links. Used when the signing key changes.
    void resetMavlinkSigning();

    void disconnectAll(void);

#ifdef QT_DEBUG
    // Only used by unit test tp restart after a shutdown
    void restart(void) { setConnectionsAllowed(); }
#endif

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    static constexpr uint8_t invalidMavlinkChannel(void) { return std::numeric_limits<uint8_t>::max(); }

    /// Allocates a mavlink channel for use
    /// @return Mavlink channel index, invalidMavlinkChannel() for no channels available
    uint8_t allocateMavlinkChannel(void);
    void freeMavlinkChannel(uint8_t channel);

    /// If you are going to hold a reference to a LinkInterface* in your object you must reference count it
    /// by using this method to get access to the shared pointer.
    SharedLinkInterfacePtr sharedLinkInterfacePointerForLink(LinkInterface* link, bool ignoreNull=false);

    bool containsLink(LinkInterface* link);

    static bool isLinkUSBDirect(LinkInterface* link);

    SharedLinkConfigurationPtr addConfiguration(LinkConfiguration* config);

    void startAutoConnectedLinks(void);

signals:
    void mavlinkSupportForwardingEnabledChanged();
    void isBluetoothAvailableChanged();

private slots:
    void _linkDisconnected  (void);

private:
    QmlObjectListModel* _qmlLinkConfigurations      (void) { return &_qmlConfigurations; }
    bool                _connectionsSuspendedMsg    (void);
    void                _updateAutoConnectLinks     (void);
    void                _removeConfiguration        (LinkConfiguration* config);
    void                _addUDPAutoConnectLink      (void);
#ifdef QGC_ZEROCONF_ENABLED
    void                _addZeroConfAutoConnectLink (void);
#endif
    void                _addMAVLinkForwardingLink   (void);
    void                _createDynamicForwardLink   (const char* linkName, QString hostName);

    bool                                _configUpdateSuspended;                     ///< true: stop updating configuration list
    bool                                _configurationsLoaded;                      ///< true: Link configurations have been loaded
    bool                                _connectionsSuspended;                      ///< true: all new connections should not be allowed
    QString                             _connectionsSuspendedReason;                ///< User visible reason for suspension
    QTimer                              _portListTimer;
    uint32_t                            _mavlinkChannelsUsedBitMask;

    AutoConnectSettings*                _autoConnectSettings;
    MAVLinkProtocol*                    _mavlinkProtocol;

    QList<SharedLinkInterfacePtr>       _rgLinks;
    QList<SharedLinkConfigurationPtr>   _rgLinkConfigs;
    QmlObjectListModel                  _qmlConfigurations;

    UdpIODevice                         _nmeaSocket;

    bool                _mavlinkSupportForwardingEnabled = false;

    static constexpr const char* _defaultUDPLinkName =                  "UDP Link (AutoConnect)";
    static constexpr const char* _mavlinkForwardingLinkName =           "MAVLink Forwarding Link";
    static constexpr const char* _mavlinkForwardingSupportLinkName =    "MAVLink Support Forwarding Link";

    static constexpr int _autoconnectUpdateTimerMSecs =   1000;
#ifdef Q_OS_WIN
    // Have to manually let the bootloader go by on Windows to get a working connect
    static constexpr int _autoconnectConnectDelayMSecs =  6000;
#else
    static constexpr int _autoconnectConnectDelayMSecs =  1000;
#endif

#ifndef NO_SERIAL_LINK
public:
    Q_PROPERTY(QStringList serialBaudRates   READ serialBaudRates   CONSTANT)
    Q_PROPERTY(QStringList serialPortStrings READ serialPortStrings NOTIFY commPortStringsChanged)
    Q_PROPERTY(QStringList serialPorts       READ serialPorts       NOTIFY commPortsChanged)

    static QStringList serialBaudRates();
    QStringList serialPortStrings();
    QStringList serialPorts();

signals:
    void commPortStringsChanged();
    void commPortsChanged();

private:
    bool _isSerialPortConnected();
    void _updateSerialPorts();
    bool _allowAutoConnectToBoard(QGCSerialPortInfo::BoardType_t boardType);
    bool _portAlreadyConnected(const QString &portName);

    QMap<QString, int> _autoconnectPortWaitList;   ///< key: QGCSerialPortInfo::systemLocation, value: wait count
    QList<SerialLink*> _activeLinkCheckList;       ///< List of links we are waiting for a vehicle to show up on
    QStringList _commPortList;
    QStringList _commPortDisplayList;
    QString _autoConnectRTKPort;
    QString _nmeaDeviceName;
    uint32_t _nmeaBaud = 0;
    QSerialPort *_nmeaPort = nullptr;
#endif // NO_SERIAL_LINK
};
