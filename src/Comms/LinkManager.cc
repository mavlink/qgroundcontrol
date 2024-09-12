/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "LinkManager.h"
#include "QGCApplication.h"
#include "UDPLink.h"
#include "TCPLink.h"
#include "SettingsManager.h"
#include "LogReplayLink.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "DeviceInfo.h"
#include "QGCLoggingCategory.h"

#ifdef QGC_ENABLE_BLUETOOTH
#include "BluetoothLink.h"
#endif

#include "PositionManager.h"
#ifndef NO_SERIAL_LINK
#include "GPSManager.h"
#include "SerialLink.h"
#endif

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

#ifndef QGC_AIRLINK_DISABLED
#include <AirlinkLink.h>
#endif

#ifdef QGC_ZEROCONF_ENABLED
#include <qmdnsengine/browser.h>
#include <qmdnsengine/cache.h>
#include <qmdnsengine/mdns.h>
#include <qmdnsengine/server.h>
#include <qmdnsengine/service.h>
#endif

#include <QtQml/QtQml>
#include <QtCore/QList>

#include <memory>

QGC_LOGGING_CATEGORY(LinkManagerLog, "LinkManagerLog")
QGC_LOGGING_CATEGORY(LinkManagerVerboseLog, "LinkManagerVerboseLog")

LinkManager::LinkManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _configUpdateSuspended(false)
    , _configurationsLoaded(false)
    , _connectionsSuspended(false)
    , _mavlinkChannelsUsedBitMask(1)    // We never use channel 0 to avoid sequence numbering problems
    , _autoConnectSettings(nullptr)
    , _mavlinkProtocol(nullptr)
{
    qmlRegisterUncreatableType<LinkManager>      ("QGroundControl", 1, 0, "LinkManager",         "Reference only");
    qmlRegisterUncreatableType<LinkConfiguration>("QGroundControl", 1, 0, "LinkConfiguration",   "Reference only");
    qmlRegisterUncreatableType<LinkInterface>    ("QGroundControl", 1, 0, "LinkInterface",       "Reference only");

    qmlRegisterUncreatableType<LinkInterface>("QGroundControl.Vehicle", 1, 0, "LinkInterface", "Reference only");
    qmlRegisterUncreatableType<LogReplayLink>("QGroundControl",         1, 0, "LogReplayLink", "Reference only");
    qmlRegisterType<LogReplayLinkController> ("QGroundControl",         1, 0, "LogReplayLinkController");

    qRegisterMetaType<QAbstractSocket::SocketError>();
    qRegisterMetaType<LinkInterface*>("LinkInterface*");
    qRegisterMetaType<QGCSerialPortInfo>("QGCSerialPortInfo");
}

LinkManager::~LinkManager()
{

}

void LinkManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    _autoConnectSettings = toolbox->settingsManager()->autoConnectSettings();
    _mavlinkProtocol = _toolbox->mavlinkProtocol();

    connect(&_portListTimer, &QTimer::timeout, this, &LinkManager::_updateAutoConnectLinks);
    _portListTimer.start(_autoconnectUpdateTimerMSecs); // timeout must be long enough to get past bootloader on second pass
}

// This should only be used by Qml code
void LinkManager::createConnectedLink(LinkConfiguration* config)
{
    for(int i = 0; i < _rgLinkConfigs.count(); i++) {
        SharedLinkConfigurationPtr& sharedConfig = _rgLinkConfigs[i];
        if (sharedConfig.get() == config)
            createConnectedLink(sharedConfig);
    }
}

bool LinkManager::createConnectedLink(SharedLinkConfigurationPtr& config, bool isPX4Flow)
{
    SharedLinkInterfacePtr link = nullptr;

    switch(config->type()) {
#ifndef NO_SERIAL_LINK
    case LinkConfiguration::TypeSerial:
        link = std::make_shared<SerialLink>(config, isPX4Flow);
        break;
#else
    Q_UNUSED(isPX4Flow)
#endif
    case LinkConfiguration::TypeUdp:
        link = std::make_shared<UDPLink>(config);
        break;
    case LinkConfiguration::TypeTcp:
        link = std::make_shared<TCPLink>(config);
        break;
#ifdef QGC_ENABLE_BLUETOOTH
    case LinkConfiguration::TypeBluetooth:
        link = std::make_shared<BluetoothLink>(config);
        break;
#endif
    case LinkConfiguration::TypeLogReplay:
        link = std::make_shared<LogReplayLink>(config);
        break;
#ifdef QT_DEBUG
    case LinkConfiguration::TypeMock:
        link = std::make_shared<MockLink>(config);
        break;
#endif
#ifndef QGC_AIRLINK_DISABLED
    case LinkConfiguration::Airlink:
        link = std::make_shared<AirlinkLink>(config);
        break;
#endif
    case LinkConfiguration::TypeLast:
        break;
    }

    if (link) {
        if (false == link->_allocateMavlinkChannel() ) {
            qCWarning(LinkManagerLog) << "Link failed to setup mavlink channels";
            return false;
        }

        _rgLinks.append(link);
        config->setLink(link);

        connect(link.get(), &LinkInterface::communicationError,  _app,                &QGCApplication::criticalMessageBoxOnMainThread);
        connect(link.get(), &LinkInterface::bytesReceived,       _mavlinkProtocol,    &MAVLinkProtocol::receiveBytes);
        connect(link.get(), &LinkInterface::bytesSent,           _mavlinkProtocol,    &MAVLinkProtocol::logSentBytes);
        connect(link.get(), &LinkInterface::disconnected,        this,                &LinkManager::_linkDisconnected);

        _mavlinkProtocol->resetMetadataForLink(link.get());
        _mavlinkProtocol->setVersion(_mavlinkProtocol->getCurrentVersion());

        if (!link->_connect()) {
            link->_freeMavlinkChannel();
            _rgLinks.removeAt(_rgLinks.indexOf(link));
            config->setLink(nullptr);
            return false;
        }

        return true;
    }

    return false;
}

SharedLinkInterfacePtr LinkManager::mavlinkForwardingLink()
{
    for (auto& link : _rgLinks) {
        SharedLinkConfigurationPtr linkConfig = link->linkConfiguration();
        if (linkConfig->type() == LinkConfiguration::TypeUdp && linkConfig->name() == _mavlinkForwardingLinkName) {
            return link;
        }
    }

    return nullptr;
}

SharedLinkInterfacePtr LinkManager::mavlinkForwardingSupportLink()
{
    for (auto& link : _rgLinks) {
        SharedLinkConfigurationPtr linkConfig = link->linkConfiguration();
        if (linkConfig->type() == LinkConfiguration::TypeUdp && linkConfig->name() == _mavlinkForwardingSupportLinkName) {
            return link;
        }
    }

    return nullptr;
}

void LinkManager::disconnectAll(void)
{
    QList<SharedLinkInterfacePtr> links = _rgLinks;

    for (const SharedLinkInterfacePtr& sharedLink: links) {
        sharedLink->disconnect();
    }
}

void LinkManager::_linkDisconnected(void)
{
    LinkInterface* link = qobject_cast<LinkInterface*>(sender());

    if (!link || !containsLink(link)) {
        return;
    }

    disconnect(link, &LinkInterface::communicationError,  _app,                &QGCApplication::criticalMessageBoxOnMainThread);
    disconnect(link, &LinkInterface::bytesReceived,       _mavlinkProtocol,    &MAVLinkProtocol::receiveBytes);
    disconnect(link, &LinkInterface::bytesSent,           _mavlinkProtocol,    &MAVLinkProtocol::logSentBytes);
    disconnect(link, &LinkInterface::disconnected,        this,                &LinkManager::_linkDisconnected);

    link->_freeMavlinkChannel();
    for (int i=0; i<_rgLinks.count(); i++) {
        if (_rgLinks[i].get() == link) {
            qCDebug(LinkManagerLog) << "LinkManager::_linkDisconnected" << _rgLinks[i]->linkConfiguration()->name() << _rgLinks[i].use_count();
            _rgLinks.removeAt(i);
            return;
        }
    }
}

SharedLinkInterfacePtr LinkManager::sharedLinkInterfacePointerForLink(LinkInterface* link, bool ignoreNull)
{
    for (int i=0; i<_rgLinks.count(); i++) {
        if (_rgLinks[i].get() == link) {
            return _rgLinks[i];
        }
    }

    if (!ignoreNull)
        qWarning() << "LinkManager::sharedLinkInterfaceForLink returning nullptr";
    return SharedLinkInterfacePtr(nullptr);
}

/// @brief If all new connections should be suspended a message is displayed to the user and true
///         is returned;
bool LinkManager::_connectionsSuspendedMsg(void)
{
    if (_connectionsSuspended) {
        qgcApp()->showAppMessage(tr("Connect not allowed: %1").arg(_connectionsSuspendedReason));
        return true;
    } else {
        return false;
    }
}

void LinkManager::setConnectionsSuspended(QString reason)
{
    _connectionsSuspended = true;
    _connectionsSuspendedReason = reason;
}

void LinkManager::suspendConfigurationUpdates(bool suspend)
{
    _configUpdateSuspended = suspend;
}

void LinkManager::saveLinkConfigurationList()
{
    QSettings settings;
    settings.remove(LinkConfiguration::settingsRoot());
    int trueCount = 0;
    for (int i = 0; i < _rgLinkConfigs.count(); i++) {
        SharedLinkConfigurationPtr linkConfig = _rgLinkConfigs[i];
        if (linkConfig) {
            if (!linkConfig->isDynamic()) {
                QString root = LinkConfiguration::settingsRoot();
                root += QString("/Link%1").arg(trueCount++);
                settings.setValue(root + "/name", linkConfig->name());
                settings.setValue(root + "/type", linkConfig->type());
                settings.setValue(root + "/auto", linkConfig->isAutoConnect());
                settings.setValue(root + "/high_latency", linkConfig->isHighLatency());
                // Have the instance save its own values
                linkConfig->saveSettings(settings, root);
            }
        } else {
            qWarning() << "Internal error for link configuration in LinkManager";
        }
    }
    QString root(LinkConfiguration::settingsRoot());
    settings.setValue(root + "/count", trueCount);
}

void LinkManager::loadLinkConfigurationList()
{
    QSettings settings;
    // Is the group even there?
    if(settings.contains(LinkConfiguration::settingsRoot() + "/count")) {
        // Find out how many configurations we have
        int count = settings.value(LinkConfiguration::settingsRoot() + "/count").toInt();
        for(int i = 0; i < count; i++) {
            QString root(LinkConfiguration::settingsRoot());
            root += QString("/Link%1").arg(i);
            if(settings.contains(root + "/type")) {
                LinkConfiguration::LinkType type = static_cast<LinkConfiguration::LinkType>(settings.value(root + "/type").toInt());
                if(type < LinkConfiguration::TypeLast) {
                    if(settings.contains(root + "/name")) {
                        QString name = settings.value(root + "/name").toString();
                        if(!name.isEmpty()) {
                            LinkConfiguration* link = nullptr;
                            bool autoConnect = settings.value(root + "/auto").toBool();
                            bool highLatency = settings.value(root + "/high_latency").toBool();

                            switch(type) {
#ifndef NO_SERIAL_LINK
                            case LinkConfiguration::TypeSerial:
                                link = new SerialConfiguration(name);
                                break;
#endif
                            case LinkConfiguration::TypeUdp:
                                link = new UDPConfiguration(name);
                                break;
                            case LinkConfiguration::TypeTcp:
                                link = new TCPConfiguration(name);
                                break;
#ifdef QGC_ENABLE_BLUETOOTH
                            case LinkConfiguration::TypeBluetooth:
                                link = new BluetoothConfiguration(name);
                                break;
#endif
                            case LinkConfiguration::TypeLogReplay:
                                link = new LogReplayLinkConfiguration(name);
                                break;
#ifdef QT_DEBUG
                            case LinkConfiguration::TypeMock:
                                link = new MockConfiguration(name);
                                break;
#endif
#ifndef QGC_AIRLINK_DISABLED
                            case LinkConfiguration::Airlink:
                                link = new AirlinkConfiguration(name);
                                break;
#endif
                            case LinkConfiguration::TypeLast:
                                break;
                            }
                            if(link) {
                                //-- Have the instance load its own values
                                link->setAutoConnect(autoConnect);
                                link->setHighLatency(highLatency);
                                link->loadSettings(settings, root);
                                addConfiguration(link);
                            }
                        } else {
                            qWarning() << "Link Configuration" << root << "has an empty name." ;
                        }
                    } else {
                        qWarning() << "Link Configuration" << root << "has no name." ;
                    }
                } else {
                    qWarning() << "Link Configuration" << root << "an invalid type: " << type;
                }
            } else {
                qWarning() << "Link Configuration" << root << "has no type." ;
            }
        }
    }

    // Enable automatic Serial PX4/3DR Radio hunting
    _configurationsLoaded = true;
}

void LinkManager::_addUDPAutoConnectLink(void)
{
    if (_autoConnectSettings->autoConnectUDP()->rawValue().toBool()) {
        bool foundUDP = false;

        for (int i = 0; i < _rgLinks.count(); i++) {
            SharedLinkConfigurationPtr linkConfig = _rgLinks[i]->linkConfiguration();
            if (linkConfig->type() == LinkConfiguration::TypeUdp && linkConfig->name() == _defaultUDPLinkName) {
                foundUDP = true;
                break;
            }
        }

        if (!foundUDP) {
            qCDebug(LinkManagerLog) << "New auto-connect UDP port added";
            //-- Default UDPConfiguration is set up for autoconnect
            UDPConfiguration* udpConfig = new UDPConfiguration(_defaultUDPLinkName);
            udpConfig->setDynamic(true);
            SharedLinkConfigurationPtr config = addConfiguration(udpConfig);
            createConnectedLink(config);
        }
    }
}

void LinkManager::_addMAVLinkForwardingLink(void)
{
    if (_toolbox->settingsManager()->appSettings()->forwardMavlink()->rawValue().toBool()) {
        bool foundMAVLinkForwardingLink = false;

        for (int i=0; i<_rgLinks.count(); i++) {
            SharedLinkConfigurationPtr linkConfig = _rgLinks[i]->linkConfiguration();
            if (linkConfig->type() == LinkConfiguration::TypeUdp && linkConfig->name() == _mavlinkForwardingLinkName) {
                foundMAVLinkForwardingLink = true;
                // TODO: should we check if the host/port matches the mavlinkForwardHostName setting and update if it does not match?
                break;
            }
        }

        if (!foundMAVLinkForwardingLink) {
            QString hostName = _toolbox->settingsManager()->appSettings()->forwardMavlinkHostName()->rawValue().toString();
            _createDynamicForwardLink(_mavlinkForwardingLinkName, hostName);
        }
    }
}

#ifdef QGC_ZEROCONF_ENABLED
void LinkManager::_addZeroConfAutoConnectLink(void)
{
    if (!_autoConnectSettings->autoConnectZeroConf()->rawValue().toBool()) {
        return;
    }

    static QSharedPointer<QMdnsEngine::Server> server;
    static QSharedPointer<QMdnsEngine::Browser> browser;
    server.reset(new QMdnsEngine::Server());
    browser.reset(new QMdnsEngine::Browser(server.get(), QMdnsEngine::MdnsBrowseType));

    auto checkIfConnectionLinkExist = [this](LinkConfiguration::LinkType linkType, const QString& linkName){
        for (const auto& link : std::as_const(_rgLinks)) {
            SharedLinkConfigurationPtr linkConfig = link->linkConfiguration();
            if (linkConfig->type() == linkType && linkConfig->name() == linkName) {
                return true;
            }
        }

        return false;
    };

    connect(browser.get(), &QMdnsEngine::Browser::serviceAdded, this, [checkIfConnectionLinkExist, this](const QMdnsEngine::Service &service) {
        qCDebug(LinkManagerVerboseLog) << "Found Zero-Conf:" << service.type() << service.name() << service.hostname() << service.port() << service.attributes();

        if(!service.type().startsWith("_mavlink")) {
            return;
        }

        // Windows dont accept trailling dots in mdns
        // http://www.dns-sd.org/trailingdotsindomainnames.html
        QString hostname = service.hostname();
        if(hostname.endsWith('.')) {
            hostname.chop(1);
        }

        if(service.type().startsWith("_mavlink._udp")) {
            static QString udpName("ZeroConf UDP");
            if (checkIfConnectionLinkExist(LinkConfiguration::TypeUdp, udpName)) {
                qCDebug(LinkManagerVerboseLog) << "Connection already exist";
                return;
            }

            auto link = new UDPConfiguration(udpName);
            link->addHost(hostname, service.port());
            link->setAutoConnect(true);
            link->setDynamic(true);
            SharedLinkConfigurationPtr config = addConfiguration(link);
            createConnectedLink(config);
            return;
        }

        if(service.type().startsWith("_mavlink._tcp")) {
            static QString tcpName("ZeroConf TCP");
            if (checkIfConnectionLinkExist(LinkConfiguration::TypeTcp, tcpName)) {
                qCDebug(LinkManagerVerboseLog) << "Connection already exist";
                return;
            }

            auto link = new TCPConfiguration(tcpName);
            link->setHost(hostname);
            link->setPort(service.port());
            link->setAutoConnect(true);
            link->setDynamic(true);
            SharedLinkConfigurationPtr config = addConfiguration(link);
            createConnectedLink(config);
            return;
        }
    });
}
#endif

void LinkManager::_updateAutoConnectLinks(void)
{
    if (_connectionsSuspended || qgcApp()->runningUnitTests()) {
        return;
    }

    _addUDPAutoConnectLink();
    _addMAVLinkForwardingLink();
#ifdef QGC_ZEROCONF_ENABLED
    _addZeroConfAutoConnectLink();
#endif

    // check to see if nmea gps is configured for UDP input, if so, set it up to connect
    if (_autoConnectSettings->autoConnectNmeaPort()->cookedValueString() == "UDP Port") {
        if (_nmeaSocket.localPort() != _autoConnectSettings->nmeaUdpPort()->rawValue().toUInt()
                || _nmeaSocket.state() != UdpIODevice::BoundState) {
            qCDebug(LinkManagerLog) << "Changing port for UDP NMEA stream";
            _nmeaSocket.close();
            _nmeaSocket.bind(QHostAddress::AnyIPv4, _autoConnectSettings->nmeaUdpPort()->rawValue().toUInt());
            _toolbox->qgcPositionManager()->setNmeaSourceDevice(&_nmeaSocket);
        }
#ifndef NO_SERIAL_LINK
        // close serial port
        if (_nmeaPort) {
            _nmeaPort->close();
            delete _nmeaPort;
            _nmeaPort = nullptr;
            _nmeaDeviceName = "";
        }
#endif
    } else {
        _nmeaSocket.close();
    }

#ifndef NO_SERIAL_LINK
    QStringList currentPorts;
    QList<QGCSerialPortInfo> portList;
#ifdef Q_OS_ANDROID
    // Android builds only support a single serial connection. Repeatedly calling availablePorts after that one serial
    // port is connected leaks file handles due to a bug somewhere in android serial code. In order to work around that
    // bug after we connect the first serial port we stop probing for additional ports.
    if (!_isSerialPortConnected()) {
        portList = QGCSerialPortInfo::availablePorts();
    }
#else
    portList = QGCSerialPortInfo::availablePorts();
#endif

    // Iterate Comm Ports
    for (const QGCSerialPortInfo &portInfo: portList) {
        qCDebug(LinkManagerVerboseLog) << "-----------------------------------------------------";
        qCDebug(LinkManagerVerboseLog) << "portName:          " << portInfo.portName();
        qCDebug(LinkManagerVerboseLog) << "systemLocation:    " << portInfo.systemLocation();
        qCDebug(LinkManagerVerboseLog) << "description:       " << portInfo.description();
        qCDebug(LinkManagerVerboseLog) << "manufacturer:      " << portInfo.manufacturer();
        qCDebug(LinkManagerVerboseLog) << "serialNumber:      " << portInfo.serialNumber();
        qCDebug(LinkManagerVerboseLog) << "vendorIdentifier:  " << portInfo.vendorIdentifier();
        qCDebug(LinkManagerVerboseLog) << "productIdentifier: " << portInfo.productIdentifier();

        // Save port name
        currentPorts << portInfo.systemLocation();

        QGCSerialPortInfo::BoardType_t boardType;
        QString boardName;

        // check to see if nmea gps is configured for current Serial port, if so, set it up to connect
        if (portInfo.systemLocation().trimmed() == _autoConnectSettings->autoConnectNmeaPort()->cookedValueString()) {
            if (portInfo.systemLocation().trimmed() != _nmeaDeviceName) {
                _nmeaDeviceName = portInfo.systemLocation().trimmed();
                qCDebug(LinkManagerLog) << "Configuring nmea port" << _nmeaDeviceName;
                QSerialPort* newPort = new QSerialPort(portInfo, this);
                _nmeaBaud = _autoConnectSettings->autoConnectNmeaBaud()->cookedValue().toUInt();
                newPort->setBaudRate(static_cast<qint32>(_nmeaBaud));
                qCDebug(LinkManagerLog) << "Configuring nmea baudrate" << _nmeaBaud;
                // This will stop polling old device if previously set
                _toolbox->qgcPositionManager()->setNmeaSourceDevice(newPort);
                if (_nmeaPort) {
                    delete _nmeaPort;
                }
                _nmeaPort = newPort;
            } else if (_autoConnectSettings->autoConnectNmeaBaud()->cookedValue().toUInt() != _nmeaBaud) {
                _nmeaBaud = _autoConnectSettings->autoConnectNmeaBaud()->cookedValue().toUInt();
                _nmeaPort->setBaudRate(static_cast<qint32>(_nmeaBaud));
                qCDebug(LinkManagerLog) << "Configuring nmea baudrate" << _nmeaBaud;
            }
        } else if (portInfo.getBoardInfo(boardType, boardName)) {
            // Should we be auto-connecting to this board type?
            if (!_allowAutoConnectToBoard(boardType)) {
                continue;
            }

            if (portInfo.isBootloader()) {
                // Don't connect to bootloader
                qCDebug(LinkManagerLog) << "Waiting for bootloader to finish" << portInfo.systemLocation();
                continue;
            }
            if (_portAlreadyConnected(portInfo.systemLocation()) || _autoConnectRTKPort == portInfo.systemLocation()) {
                qCDebug(LinkManagerVerboseLog) << "Skipping existing autoconnect" << portInfo.systemLocation();
            } else if (!_autoconnectPortWaitList.contains(portInfo.systemLocation())) {
                // We don't connect to the port the first time we see it. The ability to correctly detect whether we
                // are in the bootloader is flaky from a cross-platform standpoint. So by putting it on a wait list
                // and only connect on the second pass we leave enough time for the board to boot up.
                qCDebug(LinkManagerLog) << "Waiting for next autoconnect pass" << portInfo.systemLocation() << boardName;
                _autoconnectPortWaitList[portInfo.systemLocation()] = 1;
            } else if (++_autoconnectPortWaitList[portInfo.systemLocation()] * _autoconnectUpdateTimerMSecs > _autoconnectConnectDelayMSecs) {
                SerialConfiguration* pSerialConfig = nullptr;
                _autoconnectPortWaitList.remove(portInfo.systemLocation());
                switch (boardType) {
                case QGCSerialPortInfo::BoardTypePixhawk:
                    pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName).arg(portInfo.portName().trimmed()));
                    pSerialConfig->setUsbDirect(true);
                    break;
                case QGCSerialPortInfo::BoardTypePX4Flow:
                    pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName).arg(portInfo.portName().trimmed()));
                    break;
                case QGCSerialPortInfo::BoardTypeSiKRadio:
                    pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName).arg(portInfo.portName().trimmed()));
                    break;
                case QGCSerialPortInfo::BoardTypeOpenPilot:
                    pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName).arg(portInfo.portName().trimmed()));
                    break;
                case QGCSerialPortInfo::BoardTypeRTKGPS:
                    qCDebug(LinkManagerLog) << "RTK GPS auto-connected" << portInfo.portName().trimmed();
                    _autoConnectRTKPort = portInfo.systemLocation();
                    _toolbox->gpsManager()->connectGPS(portInfo.systemLocation(), boardName);
                    break;
                default:
                    qCWarning(LinkManagerLog) << "Internal error: Unknown board type" << boardType;
                    continue;
                }
                if (pSerialConfig) {
                    qCDebug(LinkManagerLog) << "New auto-connect port added: " << pSerialConfig->name() << portInfo.systemLocation();
                    pSerialConfig->setBaud      (boardType == QGCSerialPortInfo::BoardTypeSiKRadio ? 57600 : 115200);
                    pSerialConfig->setDynamic   (true);
                    pSerialConfig->setPortName  (portInfo.systemLocation());
                    pSerialConfig->setAutoConnect(true);

                    SharedLinkConfigurationPtr  sharedConfig(pSerialConfig);
                    createConnectedLink(sharedConfig, boardType == QGCSerialPortInfo::BoardTypePX4Flow);
                }
            }
        }
    }

    // Check for RTK GPS connection gone
    if (!_autoConnectRTKPort.isEmpty() && !currentPorts.contains(_autoConnectRTKPort)) {
        qCDebug(LinkManagerLog) << "RTK GPS disconnected" << _autoConnectRTKPort;
        _toolbox->gpsManager()->disconnectGPS();
        _autoConnectRTKPort.clear();
    }

#endif // NO_SERIAL_LINK
}

void LinkManager::shutdown(void)
{
    setConnectionsSuspended(tr("Shutdown"));
    disconnectAll();

    // Wait for all the vehicles to go away to ensure an orderly shutdown and deletion of all objects
    while (_toolbox->multiVehicleManager()->vehicles()->count()) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

QStringList LinkManager::linkTypeStrings(void) const
{
    //-- Must follow same order as enum LinkType in LinkConfiguration.h
    static QStringList list;
    if(!list.size())
    {
#ifndef NO_SERIAL_LINK
        list += tr("Serial");
#endif
        list += tr("UDP");
        list += tr("TCP");
#ifdef QGC_ENABLE_BLUETOOTH
        list += "Bluetooth";
#endif
#ifdef QT_DEBUG
        list += tr("Mock Link");
#endif
#ifndef QGC_AIRLINK_DISABLED
        list += tr("AirLink");
#endif
        list += tr("Log Replay");
        if (list.size() != static_cast<int>(LinkConfiguration::TypeLast)) {
            qWarning() << "Internal error";
        }
    }
    return list;
}

bool LinkManager::endConfigurationEditing(LinkConfiguration* config, LinkConfiguration* editedConfig)
{
    if (config && editedConfig) {
        config->copyFrom(editedConfig);
        saveLinkConfigurationList();
        emit config->nameChanged(config->name());
        // Discard temporary duplicate
        delete editedConfig;
    } else {
        qWarning() << "Internal error";
    }
    return true;
}

bool LinkManager::endCreateConfiguration(LinkConfiguration* config)
{
    if (config) {
        addConfiguration(config);
        saveLinkConfigurationList();
    } else {
        qWarning() << "Internal error";
    }
    return true;
}

LinkConfiguration* LinkManager::createConfiguration(int type, const QString& name)
{
#ifndef NO_SERIAL_LINK
    if (static_cast<LinkConfiguration::LinkType>(type) == LinkConfiguration::TypeSerial) {
        _updateSerialPorts();
    }
#endif
    return LinkConfiguration::createSettings(type, name);
}

LinkConfiguration* LinkManager::startConfigurationEditing(LinkConfiguration* config)
{
    if (config) {
#ifndef NO_SERIAL_LINK
        if (config->type() == LinkConfiguration::TypeSerial) {
            _updateSerialPorts();
        }
#endif
        return LinkConfiguration::duplicateSettings(config);
    } else {
        qWarning() << "Internal error";
        return nullptr;
    }
}

void LinkManager::removeConfiguration(LinkConfiguration* config)
{
    if (config) {
        LinkInterface* link = config->link();
        if (link) {
            link->disconnect();
        }

        _removeConfiguration(config);
        saveLinkConfigurationList();
    } else {
        qWarning() << "Internal error";
    }
}

void LinkManager::createMavlinkForwardingSupportLink(void)
{
    QString hostName = _toolbox->settingsManager()->appSettings()->forwardMavlinkAPMSupportHostName()->rawValue().toString();
    _createDynamicForwardLink(_mavlinkForwardingSupportLinkName, hostName);
    _mavlinkSupportForwardingEnabled = true;
    emit mavlinkSupportForwardingEnabledChanged();
}

void LinkManager::_removeConfiguration(LinkConfiguration* config)
{
    _qmlConfigurations.removeOne(config);
    for (int i=0; i<_rgLinkConfigs.count(); i++) {
        if (_rgLinkConfigs[i].get() == config) {
            _rgLinkConfigs.removeAt(i);
            return;
        }
    }
    qWarning() << "LinkManager::_removeConfiguration called with unknown config";
}

bool LinkManager::isBluetoothAvailable(void)
{
    return QGCDeviceInfo::isBluetoothAvailable();
}

bool LinkManager::containsLink(LinkInterface* link)
{
    for (int i=0; i<_rgLinks.count(); i++) {
        if (_rgLinks[i].get() == link) {
            return true;
        }
    }
    return false;
}

SharedLinkConfigurationPtr LinkManager::addConfiguration(LinkConfiguration* config)
{
    _qmlConfigurations.append(config);
    _rgLinkConfigs.append(SharedLinkConfigurationPtr(config));
    return _rgLinkConfigs.last();
}

void LinkManager::startAutoConnectedLinks(void)
{
    SharedLinkConfigurationPtr conf;
    for(int i = 0; i < _rgLinkConfigs.count(); i++) {
        conf = _rgLinkConfigs[i];
        if (conf->isAutoConnect())
            createConnectedLink(conf);
    }
}

uint8_t LinkManager::allocateMavlinkChannel(void)
{
    // Find a mavlink channel to use for this link
    for (uint8_t mavlinkChannel = 0; mavlinkChannel < MAVLINK_COMM_NUM_BUFFERS; mavlinkChannel++) {
        if (!(_mavlinkChannelsUsedBitMask & 1 << mavlinkChannel)) {
            mavlink_reset_channel_status(mavlinkChannel);
            // Start the channel on Mav 1 protocol
            mavlink_status_t* mavlinkStatus = mavlink_get_channel_status(mavlinkChannel);
            mavlinkStatus->flags |= MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
            _mavlinkChannelsUsedBitMask |= 1 << mavlinkChannel;
            qCDebug(LinkManagerLog) << "allocateMavlinkChannel" << mavlinkChannel;
            return mavlinkChannel;
        }
    }
    qWarning(LinkManagerLog) << "allocateMavlinkChannel: all channels reserved!";
    return invalidMavlinkChannel();   // All channels reserved
}

void LinkManager::freeMavlinkChannel(uint8_t channel)
{
    qCDebug(LinkManagerLog) << "freeMavlinkChannel" << channel;
    if (invalidMavlinkChannel() == channel) {
        return;
    }
    _mavlinkChannelsUsedBitMask &= ~(1 << channel);
}

LogReplayLink* LinkManager::startLogReplay(const QString& logFile)
{
    LogReplayLinkConfiguration* linkConfig = new LogReplayLinkConfiguration(tr("Log Replay"));
    linkConfig->setLogFilename(logFile);
    linkConfig->setName(linkConfig->logFilenameShort());

    SharedLinkConfigurationPtr sharedConfig = addConfiguration(linkConfig);
    if (createConnectedLink(sharedConfig)) {
        return qobject_cast<LogReplayLink*>(sharedConfig->link());
    } else {
        return nullptr;
    }
}

void LinkManager::_createDynamicForwardLink(const char* linkName, QString hostName)
{
    UDPConfiguration* udpConfig = new UDPConfiguration(linkName);
    udpConfig->setDynamic(true);

    udpConfig->addHost(hostName);

    SharedLinkConfigurationPtr config = addConfiguration(udpConfig);
    createConnectedLink(config);

    qCDebug(LinkManagerLog) << "New dynamic MAVLink forwarding port added: " << linkName << " hostname: " << hostName;
}

bool LinkManager::isLinkUSBDirect(LinkInterface* link)
{
#ifndef NO_SERIAL_LINK
    SerialLink* serialLink = qobject_cast<SerialLink*>(link);
    if (serialLink) {
        SharedLinkConfigurationPtr config = serialLink->linkConfiguration();
        if (config) {
            SerialConfiguration* serialConfig = qobject_cast<SerialConfiguration*>(config.get());
            if (serialConfig && serialConfig->usbDirect()) {
                return link;
            }
        }
    }
#endif

    return false;
}

void LinkManager::resetMavlinkSigning(void)
{
    for (const SharedLinkInterfacePtr& sharedLink: _rgLinks) {
        sharedLink->initMavlinkSigning();
    }
}

#ifndef NO_SERIAL_LINK // Serial Only Functions

bool LinkManager::_allowAutoConnectToBoard(QGCSerialPortInfo::BoardType_t boardType)
{
    switch (boardType) {
    case QGCSerialPortInfo::BoardTypePixhawk:
        if (_autoConnectSettings->autoConnectPixhawk()->rawValue().toBool()) {
            return true;
        }
        break;
    case QGCSerialPortInfo::BoardTypePX4Flow:
        if (_autoConnectSettings->autoConnectPX4Flow()->rawValue().toBool()) {
            return true;
        }
        break;
    case QGCSerialPortInfo::BoardTypeSiKRadio:
        if (_autoConnectSettings->autoConnectSiKRadio()->rawValue().toBool()) {
            return true;
        }
        break;
    case QGCSerialPortInfo::BoardTypeOpenPilot:
        if (_autoConnectSettings->autoConnectLibrePilot()->rawValue().toBool()) {
            return true;
        }
        break;
    case QGCSerialPortInfo::BoardTypeRTKGPS:
        if (_autoConnectSettings->autoConnectRTKGPS()->rawValue().toBool() && !_toolbox->gpsManager()->connected()) {
            return true;
        }
        break;
    default:
        qCWarning(LinkManagerLog) << "Internal error: Unknown board type" << boardType;
        return false;
    }

    return false;
}

bool LinkManager::_portAlreadyConnected(const QString &portName)
{
    const QString searchPort = portName.trimmed();
    for (const SharedLinkInterfacePtr &linkInterface : _rgLinks) {
        const SharedLinkConfigurationPtr linkConfig = linkInterface->linkConfiguration();
        const SerialConfiguration* const serialConfig = qobject_cast<const SerialConfiguration*>(linkConfig.get());
        if (serialConfig && (serialConfig->portName() == searchPort)) {
            return true;
        }
    }

    return false;
}

void LinkManager::_updateSerialPorts()
{
    _commPortList.clear();
    _commPortDisplayList.clear();
    const QList<QGCSerialPortInfo> portList = QGCSerialPortInfo::availablePorts();
    for (const QGCSerialPortInfo &info: portList) {
        const QString port = info.systemLocation().trimmed(); // + " " + info.description();
        _commPortList += port;
        _commPortDisplayList += SerialConfiguration::cleanPortDisplayname(port);
    }
}

QStringList LinkManager::serialPortStrings()
{
    if (!_commPortDisplayList.size()) {
        _updateSerialPorts();
    }

    return _commPortDisplayList;
}

QStringList LinkManager::serialPorts()
{
    if (!_commPortList.size()) {
        _updateSerialPorts();
    }

    return _commPortList;
}

QStringList LinkManager::serialBaudRates()
{
    return SerialConfiguration::supportedBaudRates();
}

bool LinkManager::_isSerialPortConnected()
{
    for (const SharedLinkInterfacePtr &link: _rgLinks) {
        if (qobject_cast<const SerialLink*>(link.get())) {
            return true;
        }
    }

    return false;
}

#endif // NO_SERIAL_LINK
