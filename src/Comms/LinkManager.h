/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>

#include <limits>

#include "LinkConfiguration.h"
#include "LinkInterface.h"
#ifndef QGC_NO_SERIAL_LINK
    #include "QGCSerialPortInfo.h"
#endif

Q_DECLARE_LOGGING_CATEGORY(LinkManagerLog)
Q_DECLARE_LOGGING_CATEGORY(LinkManagerVerboseLog)

class AutoConnectSettings;
class LogReplayLink;
class MAVLinkProtocol;
class QmlObjectListModel;
class QTimer;
class SerialLink;
class UDPConfiguration;
class UdpIODevice;

/// @brief Manage communication links
///        The Link Manager organizes the physical Links. It can manage arbitrary
///        links and takes care of connecting them as well assigning the correct
///        protocol instance to transport the link data into the application.
class LinkManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(bool isBluetoothAvailable READ isBluetoothAvailable NOTIFY isBluetoothAvailableChanged)
    Q_PROPERTY(QmlObjectListModel *linkConfigurations READ _qmlLinkConfigurations CONSTANT)
    Q_PROPERTY(QStringList linkTypeStrings READ linkTypeStrings CONSTANT)
    Q_PROPERTY(bool mavlinkSupportForwardingEnabled READ mavlinkSupportForwardingEnabled NOTIFY mavlinkSupportForwardingEnabledChanged)

public:
    explicit LinkManager(QObject *parent = nullptr);
    ~LinkManager();

    static LinkManager *instance();
    static void registerQmlTypes();

    void init();

    /// Create/Edit Link Configuration
    Q_INVOKABLE LinkConfiguration *createConfiguration(int type, const QString &name);
    Q_INVOKABLE LinkConfiguration *startConfigurationEditing(LinkConfiguration *config);
    Q_INVOKABLE void cancelConfigurationEditing(LinkConfiguration *config) const { delete config; }
    Q_INVOKABLE void endConfigurationEditing(LinkConfiguration *config, LinkConfiguration *editedConfig);
    Q_INVOKABLE void endCreateConfiguration(LinkConfiguration *config);
    Q_INVOKABLE void removeConfiguration(LinkConfiguration *config);
    /// This should only be used by Qml code
    Q_INVOKABLE void createConnectedLink(const LinkConfiguration *config);
    Q_INVOKABLE void createMavlinkForwardingSupportLink();
    /// Called to signal app shutdown. Disconnects all links while turning off auto-connect.
    Q_INVOKABLE void shutdown();
    Q_INVOKABLE LogReplayLink *startLogReplay(const QString &logFile);

    QList<SharedLinkInterfacePtr> links() { return _rgLinks; }
    QStringList linkTypeStrings() const;
    bool mavlinkSupportForwardingEnabled() const { return _mavlinkSupportForwardingEnabled; }

    void loadLinkConfigurationList();
    void saveLinkConfigurationList();

    /// Sets the flag to suspend the all new connections
    ///     @param reason User visible reason to suspend connections
    void setConnectionsSuspended(const QString &reason) { _connectionsSuspended = true; _connectionsSuspendedReason = reason; }

    /// Sets the flag to allow new connections to be made
    void setConnectionsAllowed() { _connectionsSuspended = false; }

    /// Creates, connects (and adds) a link  based on the given configuration instance.
    bool createConnectedLink(SharedLinkConfigurationPtr &config);

    /// Returns pointer to the mavlink forwarding link, or nullptr if it does not exist
    SharedLinkInterfacePtr mavlinkForwardingLink();

    /// Returns pointer to the mavlink support forwarding link, or nullptr if it does not exist
    SharedLinkInterfacePtr mavlinkForwardingSupportLink();

    /// Re-initilize the mavlink signing for all links. Used when the signing key changes.
    void resetMavlinkSigning();

    void disconnectAll();

    /// Allocates a mavlink channel for use
    ///     @return Mavlink channel index, invalidMavlinkChannel() for no channels available
    uint8_t allocateMavlinkChannel();
    void freeMavlinkChannel(uint8_t channel);

    /// If you are going to hold a reference to a LinkInterface* in your object you must reference count it
    /// by using this method to get access to the shared pointer.
    SharedLinkInterfacePtr sharedLinkInterfacePointerForLink(const LinkInterface *link);

    bool containsLink(const LinkInterface *link) const;

    SharedLinkConfigurationPtr addConfiguration(LinkConfiguration *config);

    void startAutoConnectedLinks();

    static bool isBluetoothAvailable();

    static bool isLinkUSBDirect(const LinkInterface *link);

    static constexpr uint8_t invalidMavlinkChannel() { return std::numeric_limits<uint8_t>::max(); }

signals:
    void mavlinkSupportForwardingEnabledChanged();
    void isBluetoothAvailableChanged();

private slots:
    void _linkDisconnected();

private:
    QmlObjectListModel *_qmlLinkConfigurations();
    /// If all new connections should be suspended a message is displayed to the user and true is returned;
    bool _connectionsSuspendedMsg() const;
    void _updateAutoConnectLinks();
    void _removeConfiguration(const LinkConfiguration *config);
    void _addUDPAutoConnectLink();
    void _addMAVLinkForwardingLink();
    void _createDynamicForwardLink(const char *linkName, const QString &hostName);
#ifdef QGC_ZEROCONF_ENABLED
    void _addZeroConfAutoConnectLink();
#endif

    QTimer *_portListTimer = nullptr;
    QmlObjectListModel *_qmlConfigurations = nullptr;
    AutoConnectSettings *_autoConnectSettings = nullptr;

    bool _configUpdateSuspended = false;            ///< true: stop updating configuration list
    bool _configurationsLoaded = false;             ///< true: Link configurations have been loaded
    bool _connectionsSuspended = false;             ///< true: all new connections should not be allowed
    bool _mavlinkSupportForwardingEnabled = false;
    uint32_t _mavlinkChannelsUsedBitMask = 1;
    QString _connectionsSuspendedReason;            ///< User visible reason for suspension

    QList<SharedLinkInterfacePtr> _rgLinks;
    QList<SharedLinkConfigurationPtr> _rgLinkConfigs;

    static constexpr const char *_defaultUDPLinkName = "UDP Link (AutoConnect)";
    static constexpr const char *_mavlinkForwardingLinkName = "MAVLink Forwarding Link";
    static constexpr const char *_mavlinkForwardingSupportLinkName = "MAVLink Support Forwarding Link";

    static constexpr int _autoconnectUpdateTimerMSecs = 1000;
#ifdef Q_OS_WIN
    // Have to manually let the bootloader go by on Windows to get a working connect
    static constexpr int _autoconnectConnectDelayMSecs = 6000;
#else
    static constexpr int _autoconnectConnectDelayMSecs = 1000;
#endif

#ifndef QGC_NO_SERIAL_LINK
private:
    Q_PROPERTY(QStringList serialBaudRates   READ serialBaudRates   CONSTANT)
    Q_PROPERTY(QStringList serialPortStrings READ serialPortStrings NOTIFY commPortStringsChanged)
    Q_PROPERTY(QStringList serialPorts       READ serialPorts       NOTIFY commPortsChanged)

public:
    static QStringList serialBaudRates();
    QStringList serialPortStrings();
    QStringList serialPorts();

signals:
    void commPortStringsChanged();
    void commPortsChanged();

private:
    bool _isSerialPortConnected() const;
    void _updateSerialPorts();
    bool _allowAutoConnectToBoard(QGCSerialPortInfo::BoardType_t boardType) const;
    void _addSerialAutoConnectLink();
    bool _portAlreadyConnected(const QString &portName) const;

    UdpIODevice *_nmeaSocket = nullptr;
    QMap<QString, int> _autoconnectPortWaitList;   ///< key: QGCSerialPortInfo::systemLocation, value: wait count
    QList<SerialLink*> _activeLinkCheckList;       ///< List of links we are waiting for a vehicle to show up on
    QStringList _commPortList;
    QStringList _commPortDisplayList;
    QString _autoConnectRTKPort;
    QString _nmeaDeviceName;
    uint32_t _nmeaBaud = 0;
    QSerialPort *_nmeaPort = nullptr;
#endif // QGC_NO_SERIAL_LINK
};
