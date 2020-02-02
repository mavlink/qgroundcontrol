/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QList>
#include <QApplication>
#include <QDebug>
#include <QSignalSpy>

#ifndef NO_SERIAL_LINK
#include "QGCSerialPortInfo.h"
#endif

#include "LinkManager.h"
#include "QGCApplication.h"
#include "UDPLink.h"
#include "TCPLink.h"
#include "SettingsManager.h"
#include "LogReplayLink.h"
#ifdef QGC_ENABLE_BLUETOOTH
#include "BluetoothLink.h"
#endif

#ifndef __mobile__
#include "GPSManager.h"
#include "PositionManager.h"
#endif

QGC_LOGGING_CATEGORY(LinkManagerLog, "LinkManagerLog")
QGC_LOGGING_CATEGORY(LinkManagerVerboseLog, "LinkManagerVerboseLog")

const char* LinkManager::_defaultUPDLinkName =       "UDP Link (AutoConnect)";

const int LinkManager::_autoconnectUpdateTimerMSecs =   1000;
#ifdef Q_OS_WIN
// Have to manually let the bootloader go by on Windows to get a working connect
const int LinkManager::_autoconnectConnectDelayMSecs =  6000;
#else
const int LinkManager::_autoconnectConnectDelayMSecs =  1000;
#endif

LinkManager::LinkManager(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
    , _configUpdateSuspended(false)
    , _configurationsLoaded(false)
    , _connectionsSuspended(false)
    , _mavlinkChannelsUsedBitMask(1)    // We never use channel 0 to avoid sequence numbering problems
    , _autoConnectSettings(nullptr)
    , _mavlinkProtocol(nullptr)
#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    , _nmeaPort(nullptr)
#endif
#endif
{
    qmlRegisterUncreatableType<LinkManager>         ("QGroundControl", 1, 0, "LinkManager",         "Reference only");
    qmlRegisterUncreatableType<LinkConfiguration>   ("QGroundControl", 1, 0, "LinkConfiguration",   "Reference only");
    qmlRegisterUncreatableType<LinkInterface>       ("QGroundControl", 1, 0, "LinkInterface",       "Reference only");

#ifndef NO_SERIAL_LINK
    _activeLinkCheckTimer.setInterval(_activeLinkCheckTimeoutMSecs);
    _activeLinkCheckTimer.setSingleShot(false);
    connect(&_activeLinkCheckTimer, &QTimer::timeout, this, &LinkManager::_activeLinkCheck);
#endif
}

LinkManager::~LinkManager()
{
#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    delete _nmeaPort;
#endif
#endif
}

void LinkManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);

    _autoConnectSettings = toolbox->settingsManager()->autoConnectSettings();
    _mavlinkProtocol = _toolbox->mavlinkProtocol();

    connect(_mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &LinkManager::_mavlinkMessageReceived);

    connect(&_portListTimer, &QTimer::timeout, this, &LinkManager::_updateAutoConnectLinks);
    _portListTimer.start(_autoconnectUpdateTimerMSecs); // timeout must be long enough to get past bootloader on second pass

}

// This should only be used by Qml code
void LinkManager::createConnectedLink(LinkConfiguration* config)
{
    for(int i = 0; i < _sharedConfigurations.count(); i++) {
        SharedLinkConfigurationPointer& sharedConf = _sharedConfigurations[i];
        if (sharedConf->name() == config->name())
            createConnectedLink(sharedConf);
    }
}

LinkInterface* LinkManager::createConnectedLink(SharedLinkConfigurationPointer& config, bool isPX4Flow)
{
    if (!config) {
        qWarning() << "LinkManager::createConnectedLink called with nullptr config";
        return nullptr;
    }

    LinkInterface* pLink = nullptr;
    switch(config->type()) {
#ifndef NO_SERIAL_LINK
    case LinkConfiguration::TypeSerial:
    {
        auto* serialConfig = qobject_cast<SerialConfiguration*>(config.data());
        if (serialConfig) {
            pLink = new SerialLink(config, isPX4Flow);
            if (serialConfig->usbDirect()) {
                _activeLinkCheckList.append(qobject_cast<SerialLink*>(pLink));
                if (!_activeLinkCheckTimer.isActive()) {
                    _activeLinkCheckTimer.start();
                }
            }
        }
    }
        break;
#else
    Q_UNUSED(isPX4Flow)
#endif
    case LinkConfiguration::TypeUdp:
        pLink = new UDPLink(config);
        break;
    case LinkConfiguration::TypeTcp:
        pLink = new TCPLink(config);
        break;
#ifdef QGC_ENABLE_BLUETOOTH
    case LinkConfiguration::TypeBluetooth:
        pLink = new BluetoothLink(config);
        break;
#endif
    case LinkConfiguration::TypeLogReplay:
        pLink = new LogReplayLink(config);
        break;
#ifdef QT_DEBUG
    case LinkConfiguration::TypeMock:
        pLink = new MockLink(config);
        break;
#endif
    case LinkConfiguration::TypeLast:
        break;
    }
    if (pLink) {
        _addLink(pLink);
        connectLink(pLink);
    }
    return pLink;
}

LinkInterface* LinkManager::createConnectedLink(const QString& name)
{
    if (name.isEmpty()) {
        qWarning() << "Internal error";
    } else {
        for(int i = 0; i < _sharedConfigurations.count(); i++) {
            SharedLinkConfigurationPointer& conf = _sharedConfigurations[i];
            if (conf->name() == name) {
                return createConnectedLink(conf);
            }
        }
    }
    return nullptr;
}

void LinkManager::_addLink(LinkInterface* link)
{
    if (thread() != QThread::currentThread()) {
        qWarning() << "_addLink called from incorrect thread";
        return;
    }

    if (!link) {
        return;
    }

    if (!containsLink(link)) {
        int mavlinkChannel = _reserveMavlinkChannel();
        if (mavlinkChannel != 0) {
            link->_setMavlinkChannel(mavlinkChannel);
        } else {
            qWarning() << "Ran out of mavlink channels";
            return;
        }

        _sharedLinks.append(SharedLinkInterfacePointer(link));
        emit newLink(link);
    }

    connect(link, &LinkInterface::communicationError,   _app,               &QGCApplication::criticalMessageBoxOnMainThread);
    connect(link, &LinkInterface::bytesReceived,        _mavlinkProtocol,   &MAVLinkProtocol::receiveBytes);
    connect(link, &LinkInterface::bytesSent,            _mavlinkProtocol,   &MAVLinkProtocol::logSentBytes);

    _mavlinkProtocol->resetMetadataForLink(link);
    _mavlinkProtocol->setVersion(_mavlinkProtocol->getCurrentVersion());

    connect(link, &LinkInterface::connected,            this, &LinkManager::_linkConnected);
    connect(link, &LinkInterface::disconnected,         this, &LinkManager::_linkDisconnected);

    // This connection is queued since it will cloe the link. So we want the link emitter to return otherwise we would
    // close the link our from under itself.
    connect(link, &LinkInterface::connectionRemoved,    this, &LinkManager::_linkConnectionRemoved, Qt::QueuedConnection);
}

void LinkManager::disconnectAll(void)
{
    // Walk list in reverse order to preserve indices during delete
    for (int i=_sharedLinks.count()-1; i>=0; i--) {
        disconnectLink(_sharedLinks[i].data());
    }
}

bool LinkManager::connectLink(LinkInterface* link)
{
    if (link) {
        if (_connectionsSuspendedMsg()) {
            return false;
        }
        return link->_connect();
    } else {
        qWarning() << "Internal error";
        return false;
    }
}

void LinkManager::disconnectLink(LinkInterface* link)
{
    if (!link || !containsLink(link)) {
        return;
    }

    link->_disconnect();

    LinkConfiguration* config = link->getLinkConfiguration();
    for (int i=0; i<_sharedAutoconnectConfigurations.count(); i++) {
        if (_sharedAutoconnectConfigurations[i].data() == config) {
            qCDebug(LinkManagerLog) << "Removing disconnected autoconnect config" << config->name();
            _sharedAutoconnectConfigurations.removeAt(i);
            break;
        }
    }

    _deleteLink(link);
}

void LinkManager::_deleteLink(LinkInterface* link)
{
    if (thread() != QThread::currentThread()) {
        qWarning() << "_deleteLink called from incorrect thread";
        return;
    }

    if (!link) {
        return;
    }

    // Free up the mavlink channel associated with this link
    _freeMavlinkChannel(link->mavlinkChannel());

    for (int i=0; i<_sharedLinks.count(); i++) {
        if (_sharedLinks[i].data() == link) {
            _sharedLinks.removeAt(i);
            break;
        }
    }

    // Emit removal of link
    emit linkDeleted(link);
}

SharedLinkInterfacePointer LinkManager::sharedLinkInterfacePointerForLink(LinkInterface* link)
{
    for (int i=0; i<_sharedLinks.count(); i++) {
        if (_sharedLinks[i].data() == link) {
            return _sharedLinks[i];
        }
    }

    qWarning() << "LinkManager::sharedLinkInterfaceForLink returning nullptr";
    return SharedLinkInterfacePointer(nullptr);
}

/// @brief If all new connections should be suspended a message is displayed to the user and true
///         is returned;
bool LinkManager::_connectionsSuspendedMsg(void)
{
    if (_connectionsSuspended) {
        qgcApp()->showMessage(tr("Connect not allowed: %1").arg(_connectionsSuspendedReason));
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

void LinkManager::_linkConnected(void)
{
    emit linkConnected((LinkInterface*)sender());
}

void LinkManager::_linkDisconnected(void)
{
    emit linkDisconnected((LinkInterface*)sender());
}

void LinkManager::_linkConnectionRemoved(LinkInterface* link)
{
    // Link has been removed from system, disconnect it automatically
    disconnectLink(link);
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
    for (int i = 0; i < _sharedConfigurations.count(); i++) {
        SharedLinkConfigurationPointer linkConfig = _sharedConfigurations[i];
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
    emit linkConfigurationsChanged();
}

void LinkManager::loadLinkConfigurationList()
{
    bool linksChanged = false;
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
                            LinkConfiguration* pLink = nullptr;
                            bool autoConnect = settings.value(root + "/auto").toBool();
                            bool highLatency = settings.value(root + "/high_latency").toBool();
                            switch(type) {
#ifndef NO_SERIAL_LINK
                            case LinkConfiguration::TypeSerial:
                                pLink = new SerialConfiguration(name);
                                break;
#endif
                            case LinkConfiguration::TypeUdp:
                                pLink = new UDPConfiguration(name);
                                break;
                            case LinkConfiguration::TypeTcp:
                                pLink = new TCPConfiguration(name);
                                break;
#ifdef QGC_ENABLE_BLUETOOTH
                            case LinkConfiguration::TypeBluetooth:
                                pLink = new BluetoothConfiguration(name);
                                break;
#endif
                            case LinkConfiguration::TypeLogReplay:
                                pLink = new LogReplayLinkConfiguration(name);
                                break;
#ifdef QT_DEBUG
                            case LinkConfiguration::TypeMock:
                                pLink = new MockConfiguration(name);
                                break;
#endif
                            case LinkConfiguration::TypeLast:
                                break;
                            }
                            if(pLink) {
                                //-- Have the instance load its own values
                                pLink->setAutoConnect(autoConnect);
                                pLink->setHighLatency(highLatency);
                                pLink->loadSettings(settings, root);
                                addConfiguration(pLink);
                                linksChanged = true;
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

    if(linksChanged) {
        emit linkConfigurationsChanged();
    }
    // Enable automatic Serial PX4/3DR Radio hunting
    _configurationsLoaded = true;
}

#ifndef NO_SERIAL_LINK
SerialConfiguration* LinkManager::_autoconnectConfigurationsContainsPort(const QString& portName)
{
    QString searchPort = portName.trimmed();

    for (int i=0; i<_sharedAutoconnectConfigurations.count(); i++) {
        SerialConfiguration* serialConfig = qobject_cast<SerialConfiguration*>(_sharedAutoconnectConfigurations[i].data());

        if (serialConfig) {
            if (serialConfig->portName() == searchPort) {
                return serialConfig;
            }
        } else {
            qWarning() << "Internal error";
        }
    }
    return nullptr;
}
#endif

void LinkManager::_updateAutoConnectLinks(void)
{
    if (_connectionsSuspended || qgcApp()->runningUnitTests()) {
        return;
    }
    // Re-add UDP if we need to
    bool foundUDP = false;
    for (int i = 0; i < _sharedLinks.count(); i++) {
        LinkConfiguration* linkConfig = _sharedLinks[i]->getLinkConfiguration();
        if (linkConfig->type() == LinkConfiguration::TypeUdp && linkConfig->name() == _defaultUPDLinkName) {
            foundUDP = true;
            break;
        }
    }
    if (!foundUDP && _autoConnectSettings->autoConnectUDP()->rawValue().toBool()) {
        qCDebug(LinkManagerLog) << "New auto-connect UDP port added";
        //-- Default UDPConfiguration is set up for autoconnect
        UDPConfiguration* udpConfig = new UDPConfiguration(_defaultUPDLinkName);
        udpConfig->setDynamic(true);
        SharedLinkConfigurationPointer config = addConfiguration(udpConfig);
        createConnectedLink(config);
        emit linkConfigurationsChanged();
    }
#ifndef __mobile__
#ifndef NO_SERIAL_LINK
    // check to see if nmea gps is configured for UDP input, if so, set it up to connect
    if (_autoConnectSettings->autoConnectNmeaPort()->cookedValueString() == "UDP Port") {
        if (_nmeaSocket.localPort() != _autoConnectSettings->nmeaUdpPort()->rawValue().toUInt()
                || _nmeaSocket.state() != UdpIODevice::BoundState) {
            qCDebug(LinkManagerLog) << "Changing port for UDP NMEA stream";
            _nmeaSocket.close();
            _nmeaSocket.bind(QHostAddress::AnyIPv4, _autoConnectSettings->nmeaUdpPort()->rawValue().toUInt());
            _toolbox->qgcPositionManager()->setNmeaSourceDevice(&_nmeaSocket);
        }
        //close serial port
        if (_nmeaPort) {
            _nmeaPort->close();
            delete _nmeaPort;
            _nmeaPort = nullptr;
            _nmeaDeviceName = "";
        }
    } else {
        _nmeaSocket.close();
    }
#endif
#endif

#ifndef NO_SERIAL_LINK
    QStringList currentPorts;
    QList<QGCSerialPortInfo> portList;
#ifdef __android__
    // Android builds only support a single serial connection. Repeatedly calling availablePorts after that one serial
    // port is connected leaks file handles due to a bug somewhere in android serial code. In order to work around that
    // bug after we connect the first serial port we stop probing for additional ports.
    if (!_sharedAutoconnectConfigurations.count()) {
        portList = QGCSerialPortInfo::availablePorts();
    }
#else
    portList = QGCSerialPortInfo::availablePorts();
#endif

    // Iterate Comm Ports
    for (const QGCSerialPortInfo& portInfo: portList) {
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

#ifndef NO_SERIAL_LINK
#ifndef __mobile__
        // check to see if nmea gps is configured for current Serial port, if so, set it up to connect
        if (portInfo.systemLocation().trimmed() == _autoConnectSettings->autoConnectNmeaPort()->cookedValueString()) {
            if (portInfo.systemLocation().trimmed() != _nmeaDeviceName) {
                _nmeaDeviceName = portInfo.systemLocation().trimmed();
                qCDebug(LinkManagerLog) << "Configuring nmea port" << _nmeaDeviceName;
                QSerialPort* newPort = new QSerialPort(portInfo);
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
        } else
#endif
#endif
        if (portInfo.getBoardInfo(boardType, boardName)) {
            if (portInfo.isBootloader()) {
                // Don't connect to bootloader
                qCDebug(LinkManagerLog) << "Waiting for bootloader to finish" << portInfo.systemLocation();
                continue;
            }
            if (_autoconnectConfigurationsContainsPort(portInfo.systemLocation()) || _autoConnectRTKPort == portInfo.systemLocation()) {
                qCDebug(LinkManagerVerboseLog) << "Skipping existing autoconnect" << portInfo.systemLocation();
            } else if (!_autoconnectWaitList.contains(portInfo.systemLocation())) {
                // We don't connect to the port the first time we see it. The ability to correctly detect whether we
                // are in the bootloader is flaky from a cross-platform standpoint. So by putting it on a wait list
                // and only connect on the second pass we leave enough time for the board to boot up.
                qCDebug(LinkManagerLog) << "Waiting for next autoconnect pass" << portInfo.systemLocation();
                _autoconnectWaitList[portInfo.systemLocation()] = 1;
            } else if (++_autoconnectWaitList[portInfo.systemLocation()] * _autoconnectUpdateTimerMSecs > _autoconnectConnectDelayMSecs) {
                SerialConfiguration* pSerialConfig = nullptr;
                _autoconnectWaitList.remove(portInfo.systemLocation());
                switch (boardType) {
                case QGCSerialPortInfo::BoardTypePixhawk:
                    if (_autoConnectSettings->autoConnectPixhawk()->rawValue().toBool()) {
                        pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName).arg(portInfo.portName().trimmed()));
                        pSerialConfig->setUsbDirect(true);
                    }
                    break;
                case QGCSerialPortInfo::BoardTypePX4Flow:
                    if (_autoConnectSettings->autoConnectPX4Flow()->rawValue().toBool()) {
                        pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName).arg(portInfo.portName().trimmed()));
                    }
                    break;
                case QGCSerialPortInfo::BoardTypeSiKRadio:
                    if (_autoConnectSettings->autoConnectSiKRadio()->rawValue().toBool()) {
                        pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName).arg(portInfo.portName().trimmed()));
                    }
                    break;
                case QGCSerialPortInfo::BoardTypeOpenPilot:
                    if (_autoConnectSettings->autoConnectLibrePilot()->rawValue().toBool()) {
                        pSerialConfig = new SerialConfiguration(tr("%1 on %2 (AutoConnect)").arg(boardName).arg(portInfo.portName().trimmed()));
                    }
                    break;
#ifndef __mobile__
                case QGCSerialPortInfo::BoardTypeRTKGPS:
                    if (_autoConnectSettings->autoConnectRTKGPS()->rawValue().toBool() && !_toolbox->gpsManager()->connected()) {
                        qCDebug(LinkManagerLog) << "RTK GPS auto-connected" << portInfo.portName().trimmed();
                        _autoConnectRTKPort = portInfo.systemLocation();
                        _toolbox->gpsManager()->connectGPS(portInfo.systemLocation(), boardName);
                    }
                    break;
#endif
                default:
                    qWarning() << "Internal error";
                    continue;
                }
                if (pSerialConfig) {
                    qCDebug(LinkManagerLog) << "New auto-connect port added: " << pSerialConfig->name() << portInfo.systemLocation();
                    pSerialConfig->setBaud(boardType == QGCSerialPortInfo::BoardTypeSiKRadio ? 57600 : 115200);
                    pSerialConfig->setDynamic(true);
                    pSerialConfig->setPortName(portInfo.systemLocation());
                    _sharedAutoconnectConfigurations.append(SharedLinkConfigurationPointer(pSerialConfig));
                    createConnectedLink(_sharedAutoconnectConfigurations.last(), boardType == QGCSerialPortInfo::BoardTypePX4Flow);
                }
            }
        }
    }

#ifndef __android__
    // Android builds only support a single serial connection. Repeatedly calling availablePorts after that one serial
    // port is connected leaks file handles due to a bug somewhere in android serial code. In order to work around that
    // bug after we connect the first serial port we stop probing for additional ports. That means we must rely on
    // the port disconnecting itself when the radio is pulled to signal communication list as opposed to automatically
    // closing the Link.

    // Now we go through the current configuration list and make sure any dynamic config has gone away
    QList<LinkConfiguration*>  _confToDelete;
    for (int i=0; i<_sharedAutoconnectConfigurations.count(); i++) {
        SerialConfiguration* serialConfig = qobject_cast<SerialConfiguration*>(_sharedAutoconnectConfigurations[i].data());
        if (serialConfig) {
            if (!currentPorts.contains(serialConfig->portName())) {
                if (serialConfig->link()) {
                    if (serialConfig->link()->isConnected()) {
                        if (serialConfig->link()->active()) {
                            // We don't remove links which are still connected which have been active with a vehicle on them
                            // even though at this point the cable may have been pulled. Instead we wait for the user to
                            // Disconnect. Once the user disconnects, the link will be removed.
                            continue;
                        }
                    }
                }
                _confToDelete.append(serialConfig);
            }
        } else {
            qWarning() << "Internal error";
        }
    }

    // Now remove all configs that are gone
    for (LinkConfiguration* pDeleteConfig: _confToDelete) {
        qCDebug(LinkManagerLog) << "Removing unused autoconnect config" << pDeleteConfig->name();
        if (pDeleteConfig->link()) {
            disconnectLink(pDeleteConfig->link());
        }
        for (int i=0; i<_sharedAutoconnectConfigurations.count(); i++) {
            if (_sharedAutoconnectConfigurations[i].data() == pDeleteConfig) {
                _sharedAutoconnectConfigurations.removeAt(i);
                break;
            }
        }
    }

    // Check for RTK GPS connection gone
#if !defined(__mobile__)
    if (!_autoConnectRTKPort.isEmpty() && !currentPorts.contains(_autoConnectRTKPort)) {
        qCDebug(LinkManagerLog) << "RTK GPS disconnected" << _autoConnectRTKPort;
        _toolbox->gpsManager()->disconnectGPS();
        _autoConnectRTKPort.clear();
    }
#endif

#endif
#endif // NO_SERIAL_LINK
}

void LinkManager::shutdown(void)
{
    setConnectionsSuspended(tr("Shutdown"));
    disconnectAll();
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
#ifndef __mobile__
        list += tr("Log Replay");
#endif
        if (list.size() != static_cast<int>(LinkConfiguration::TypeLast)) {
            qWarning() << "Internal error";
        }
    }
    return list;
}

void LinkManager::_updateSerialPorts()
{
    _commPortList.clear();
    _commPortDisplayList.clear();
#ifndef NO_SERIAL_LINK
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info: portList)
    {
        QString port = info.systemLocation().trimmed();
        _commPortList += port;
        _commPortDisplayList += SerialConfiguration::cleanPortDisplayname(port);
    }
#endif
}

QStringList LinkManager::serialPortStrings(void)
{
    if(!_commPortDisplayList.size())
    {
        _updateSerialPorts();
    }
    return _commPortDisplayList;
}

QStringList LinkManager::serialPorts(void)
{
    if(!_commPortList.size())
    {
        _updateSerialPorts();
    }
    return _commPortList;
}

QStringList LinkManager::serialBaudRates(void)
{
#ifdef NO_SERIAL_LINK
    QStringList foo;
    return foo;
#else
    return SerialConfiguration::supportedBaudRates();
#endif
}

bool LinkManager::endConfigurationEditing(LinkConfiguration* config, LinkConfiguration* editedConfig)
{
    if (config && editedConfig) {
        _fixUnnamed(editedConfig);
        config->copyFrom(editedConfig);
        saveLinkConfigurationList();
        // Tell link about changes (if any)
        config->updateSettings();
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
        _fixUnnamed(config);
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
    if(static_cast<LinkConfiguration::LinkType>(type) == LinkConfiguration::TypeSerial)
        _updateSerialPorts();
#endif
    return LinkConfiguration::createSettings(type, name);
}

LinkConfiguration* LinkManager::startConfigurationEditing(LinkConfiguration* config)
{
    if (config) {
#ifndef NO_SERIAL_LINK
        if(config->type() == LinkConfiguration::TypeSerial)
            _updateSerialPorts();
#endif
        return LinkConfiguration::duplicateSettings(config);
    } else {
        qWarning() << "Internal error";
        return nullptr;
    }
}

void LinkManager::_fixUnnamed(LinkConfiguration* config)
{
    if (config) {
        //-- Check for "Unnamed"
        if (config->name() == "Unnamed") {
            switch(config->type()) {
#ifndef NO_SERIAL_LINK
            case LinkConfiguration::TypeSerial: {
                QString tname = dynamic_cast<SerialConfiguration*>(config)->portName();
#ifdef Q_OS_WIN
                tname.replace("\\\\.\\", "");
#else
                tname.replace("/dev/cu.", "");
                tname.replace("/dev/", "");
#endif
                config->setName(QString("Serial Device on %1").arg(tname));
                break;
            }
#endif
            case LinkConfiguration::TypeUdp:
                config->setName(
                    QString("UDP Link on Port %1").arg(dynamic_cast<UDPConfiguration*>(config)->localPort()));
                break;
            case LinkConfiguration::TypeTcp: {
                TCPConfiguration* tconfig = dynamic_cast<TCPConfiguration*>(config);
                if(tconfig) {
                    config->setName(
                        QString("TCP Link %1:%2").arg(tconfig->address().toString()).arg(static_cast<int>(tconfig->port())));
                }
            }
                break;
#ifdef QGC_ENABLE_BLUETOOTH
            case LinkConfiguration::TypeBluetooth: {
                BluetoothConfiguration* tconfig = dynamic_cast<BluetoothConfiguration*>(config);
                if(tconfig) {
                    config->setName(QString("%1 (Bluetooth Device)").arg(tconfig->device().name));
                }
            }
                break;
#endif
            case LinkConfiguration::TypeLogReplay: {
                LogReplayLinkConfiguration* tconfig = dynamic_cast<LogReplayLinkConfiguration*>(config);
                if(tconfig) {
                    config->setName(QString("Log Replay %1").arg(tconfig->logFilenameShort()));
                }
            }
                break;
#ifdef QT_DEBUG
            case LinkConfiguration::TypeMock:
                config->setName(QString("Mock Link"));
                break;
#endif
            case LinkConfiguration::TypeLast:
                break;
            }
        }
    } else {
        qWarning() << "Internal error";
    }
}

void LinkManager::removeConfiguration(LinkConfiguration* config)
{
    if (config) {
        LinkInterface* iface = config->link();
        if(iface) {
            disconnectLink(iface);
        }

        _removeConfiguration(config);
        saveLinkConfigurationList();
    } else {
        qWarning() << "Internal error";
    }
}

bool LinkManager::isAutoconnectLink(LinkInterface* link)
{
    for (int i=0; i<_sharedAutoconnectConfigurations.count(); i++) {
        if (_sharedAutoconnectConfigurations[i].data() == link->getLinkConfiguration()) {
            return true;
        }
    }
    return false;
}

bool LinkManager::isBluetoothAvailable(void)
{
    return qgcApp()->isBluetoothAvailable();
}

#ifndef NO_SERIAL_LINK
void LinkManager::_activeLinkCheck(void)
{
    SerialLink* link = nullptr;
    bool found = false;
    if (_activeLinkCheckList.count() != 0) {
        link = _activeLinkCheckList.takeFirst();
        if (containsLink(link) && link->isConnected()) {
            // Make sure there is a vehicle on the link
            QmlObjectListModel* vehicles = _toolbox->multiVehicleManager()->vehicles();
            for (int i=0; i<vehicles->count(); i++) {
                Vehicle* vehicle = qobject_cast<Vehicle*>(vehicles->get(i));
                if (vehicle->containsLink(link)) {
                    found = true;
                    break;
                }
            }
        } else {
            link = nullptr;
        }
    }
    if (_activeLinkCheckList.count() == 0) {
        _activeLinkCheckTimer.stop();
    }
    if (!found && link) {
        // See if we can get an NSH prompt on this link
        bool foundNSHPrompt = false;
        link->writeBytesSafe("\r", 1);
        QSignalSpy spy(link, SIGNAL(bytesReceived(LinkInterface*, QByteArray)));
        if (spy.wait(100)) {
            QList<QVariant> arguments = spy.takeFirst();
            if (arguments[1].toByteArray().contains("nsh>")) {
                foundNSHPrompt = true;
            }
        }
        qgcApp()->showMessage(
            foundNSHPrompt ?
                tr("Please check to make sure you have an SD Card inserted in your Vehicle and try again.") :
                tr("Your Vehicle is not responding. If this continues, shutdown %1, restart the Vehicle letting it boot completely, then start %1.").arg(qgcApp()->applicationName()));
    }
}
#endif

bool LinkManager::containsLink(LinkInterface* link)
{
    for (int i=0; i<_sharedLinks.count(); i++) {
        if (_sharedLinks[i].data() == link) {
            return true;
        }
    }
    return false;
}

SharedLinkConfigurationPointer LinkManager::addConfiguration(LinkConfiguration* config)
{
    _qmlConfigurations.append(config);
    _sharedConfigurations.append(SharedLinkConfigurationPointer(config));
    return _sharedConfigurations.last();
}

void LinkManager::_removeConfiguration(LinkConfiguration* config)
{
    _qmlConfigurations.removeOne(config);
    for (int i=0; i<_sharedConfigurations.count(); i++) {
        if (_sharedConfigurations[i].data() == config) {
            _sharedConfigurations.removeAt(i);
            return;
        }
    }
    qWarning() << "LinkManager::_removeConfiguration called with unknown config";
}

QList<LinkInterface*> LinkManager::links(void)
{
    QList<LinkInterface*> rawLinks;
    for (int i=0; i<_sharedLinks.count(); i++) {
        rawLinks.append(_sharedLinks[i].data());
    }
    return rawLinks;
}

void LinkManager::startAutoConnectedLinks(void)
{
    SharedLinkConfigurationPointer conf;
    for(int i = 0; i < _sharedConfigurations.count(); i++) {
        conf = _sharedConfigurations[i];
        if (conf->isAutoConnect())
            createConnectedLink(conf);
    }
}

int LinkManager::_reserveMavlinkChannel(void)
{
    // Find a mavlink channel to use for this link, Channel 0 is reserved for internal use.
    for (uint8_t mavlinkChannel = 1; mavlinkChannel < MAVLINK_COMM_NUM_BUFFERS; mavlinkChannel++) {
        if (!(_mavlinkChannelsUsedBitMask & 1 << mavlinkChannel)) {
            mavlink_reset_channel_status(mavlinkChannel);
            // Start the channel on Mav 1 protocol
            mavlink_status_t* mavlinkStatus = mavlink_get_channel_status(mavlinkChannel);
            mavlinkStatus->flags |= MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
            _mavlinkChannelsUsedBitMask |= 1 << mavlinkChannel;
            return mavlinkChannel;
        }
    }
    return 0;   // All channels reserved
}

void LinkManager::_freeMavlinkChannel(int channel)
{
    _mavlinkChannelsUsedBitMask &= ~(1 << channel);
}

void LinkManager::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message) {
    link->startMavlinkMessagesTimer(message.sysid);
}

LogReplayLink* LinkManager::startLogReplay(const QString& logFile)
{
    LogReplayLinkConfiguration* linkConfig = new LogReplayLinkConfiguration(tr("Log Replay"));
    linkConfig->setLogFilename(logFile);
    linkConfig->setName(linkConfig->logFilenameShort());

    SharedLinkConfigurationPointer sharedConfig = addConfiguration(linkConfig);
    return qobject_cast<LogReplayLink*>(createConnectedLink(sharedConfig));
}
