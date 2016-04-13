/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
#include <QDebug>
#include <QSignalSpy>

#ifndef __ios__
#include "QGCSerialPortInfo.h"
#endif

#include "LinkManager.h"
#include "QGCApplication.h"
#include "UDPLink.h"
#include "TCPLink.h"
#ifdef QGC_ENABLE_BLUETOOTH
#include "BluetoothLink.h"
#endif

QGC_LOGGING_CATEGORY(LinkManagerLog, "LinkManagerLog")
QGC_LOGGING_CATEGORY(LinkManagerVerboseLog, "LinkManagerVerboseLog")

const char* LinkManager::_settingsGroup =           "LinkManager";
const char* LinkManager::_autoconnectUDPKey =       "AutoconnectUDP";
const char* LinkManager::_autoconnectPixhawkKey =   "AutoconnectPixhawk";
const char* LinkManager::_autoconnect3DRRadioKey =  "Autoconnect3DRRadio";
const char* LinkManager::_autoconnectPX4FlowKey =   "AutoconnectPX4Flow";
const char* LinkManager::_defaultUPDLinkName =      "Default UDP Link";

const int LinkManager::_autoconnectUpdateTimerMSecs =   1000;
#ifdef Q_OS_WIN
// Have to manually let the bootloader go by on Windows to get a working connect
const int LinkManager::_autoconnectConnectDelayMSecs =  6000;
#else
const int LinkManager::_autoconnectConnectDelayMSecs =  1000;
#endif

LinkManager::LinkManager(QGCApplication* app)
    : QGCTool(app)
    , _configUpdateSuspended(false)
    , _configurationsLoaded(false)
    , _connectionsSuspended(false)
    , _mavlinkChannelsUsedBitMask(1)    // We never use channel 0 to avoid sequence numbering problems
    , _mavlinkProtocol(NULL)
    , _autoconnectUDP(true)
    , _autoconnectPixhawk(true)
    , _autoconnect3DRRadio(true)
    , _autoconnectPX4Flow(true)

{
    qmlRegisterUncreatableType<LinkManager>         ("QGroundControl", 1, 0, "LinkManager",         "Reference only");
    qmlRegisterUncreatableType<LinkConfiguration>   ("QGroundControl", 1, 0, "LinkConfiguration",   "Reference only");
    qmlRegisterUncreatableType<LinkInterface>       ("QGroundControl", 1, 0, "LinkInterface",       "Reference only");

    QSettings settings;

    settings.beginGroup(_settingsGroup);
    _autoconnectUDP =       settings.value(_autoconnectUDPKey, true).toBool();
    _autoconnectPixhawk =   settings.value(_autoconnectPixhawkKey, true).toBool();
    _autoconnect3DRRadio =  settings.value(_autoconnect3DRRadioKey, true).toBool();
    _autoconnectPX4Flow =   settings.value(_autoconnectPX4FlowKey, true).toBool();

#ifndef __ios__
    _activeLinkCheckTimer.setInterval(_activeLinkCheckTimeoutMSecs);
    _activeLinkCheckTimer.setSingleShot(false);
    connect(&_activeLinkCheckTimer, &QTimer::timeout, this, &LinkManager::_activeLinkCheck);
#endif
}

LinkManager::~LinkManager()
{

}

void LinkManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);

   _mavlinkProtocol = _toolbox->mavlinkProtocol();

    connect(&_portListTimer, &QTimer::timeout, this, &LinkManager::_updateAutoConnectLinks);
    _portListTimer.start(_autoconnectUpdateTimerMSecs); // timeout must be long enough to get past bootloader on second pass

}

LinkInterface* LinkManager::createConnectedLink(LinkConfiguration* config)
{
    Q_ASSERT(config);
    LinkInterface* pLink = NULL;
    switch(config->type()) {
#ifndef __ios__
        case LinkConfiguration::TypeSerial:
        {
            SerialConfiguration* serialConfig = dynamic_cast<SerialConfiguration*>(config);
            if (serialConfig) {
                pLink = new SerialLink(serialConfig);
                if (serialConfig->usbDirect()) {
                    _activeLinkCheckList.append((SerialLink*)pLink);
                    if (!_activeLinkCheckTimer.isActive()) {
                        _activeLinkCheckTimer.start();
                    }
                }
            }
        }
        break;
#endif
        case LinkConfiguration::TypeUdp:
            pLink = new UDPLink(dynamic_cast<UDPConfiguration*>(config));
            break;
        case LinkConfiguration::TypeTcp:
            pLink = new TCPLink(dynamic_cast<TCPConfiguration*>(config));
            break;
#ifdef QGC_ENABLE_BLUETOOTH
        case LinkConfiguration::TypeBluetooth:
            pLink = new BluetoothLink(dynamic_cast<BluetoothConfiguration*>(config));
            break;
#endif
#ifndef __mobile__
        case LinkConfiguration::TypeLogReplay:
            pLink = new LogReplayLink(dynamic_cast<LogReplayLinkConfiguration*>(config));
            break;
#endif
#ifdef QT_DEBUG
        case LinkConfiguration::TypeMock:
            pLink = new MockLink(dynamic_cast<MockConfiguration*>(config));
            break;
#endif
        case LinkConfiguration::TypeLast:
        default:
            break;
    }
    if(pLink) {
        _addLink(pLink);
        connectLink(pLink);
    }
    return pLink;
}

LinkInterface* LinkManager::createConnectedLink(const QString& name)
{
    Q_ASSERT(name.isEmpty() == false);
    for(int i = 0; i < _linkConfigurations.count(); i++) {
        LinkConfiguration* conf = _linkConfigurations.value<LinkConfiguration*>(i);
        if(conf && conf->name() == name)
            return createConnectedLink(conf);
    }
    return NULL;
}

void LinkManager::_addLink(LinkInterface* link)
{
    if (thread() != QThread::currentThread()) {
        qWarning() << "_deleteLink called from incorrect thread";
        return;
    }

    if (!link) {
        return;
    }

    if (!_links.contains(link)) {
        bool channelSet = false;

        // Find a mavlink channel to use for this link
        for (int i=0; i<32; i++) {
            if (!(_mavlinkChannelsUsedBitMask & 1 << i)) {
                mavlink_reset_channel_status(i);
                link->_setMavlinkChannel(i);
                _mavlinkChannelsUsedBitMask |= i << i;
                channelSet = true;
                break;
            }
        }

        if (!channelSet) {
            qWarning() << "Ran out of mavlink channels";
        }

        _links.append(link);
        emit newLink(link);
    }

    connect(link, &LinkInterface::communicationError,   _app,               &QGCApplication::criticalMessageBoxOnMainThread);
    connect(link, &LinkInterface::bytesReceived,        _mavlinkProtocol,   &MAVLinkProtocol::receiveBytes);

    _mavlinkProtocol->resetMetadataForLink(link);

    connect(link, &LinkInterface::connected,            this, &LinkManager::_linkConnected);
    connect(link, &LinkInterface::disconnected,         this, &LinkManager::_linkDisconnected);

    // This connection is queued since it will cloe the link. So we want the link emitter to return otherwise we would
    // close the link our from under itself.
    connect(link, &LinkInterface::connectionRemoved,    this, &LinkManager::_linkConnectionRemoved, Qt::QueuedConnection);
}

void LinkManager::disconnectAll(void)
{
    // Walk list in reverse order to preserve indices during delete
    for (int i=_links.count()-1; i>=0; i--) {
        disconnectLink(_links.value<LinkInterface*>(i));
    }
}

bool LinkManager::connectLink(LinkInterface* link)
{
    Q_ASSERT(link);

    if (_connectionsSuspendedMsg()) {
        return false;
    }

    return link->_connect();
}

void LinkManager::disconnectLink(LinkInterface* link)
{
    if (!link || !_links.contains(link)) {
        return;
    }

    link->_disconnect();
    LinkConfiguration* config = link->getLinkConfiguration();
    if (config) {
        if (_autoconnectConfigurations.contains(config)) {
            config->setLink(NULL);
        }
    }
    _deleteLink(link);
    if (_autoconnectConfigurations.contains(config)) {
        qCDebug(LinkManagerLog) << "Removing disconnected autoconnect config" << config->name();
        _autoconnectConfigurations.removeOne(config);
        delete config;
    }
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
    _mavlinkChannelsUsedBitMask &= ~(1 << link->getMavlinkChannel());

    _links.removeOne(link);
    delete link;

    // Emit removal of link
    emit linkDeleted(link);
}

/// @brief If all new connections should be suspended a message is displayed to the user and true
///         is returned;
bool LinkManager::_connectionsSuspendedMsg(void)
{
    if (_connectionsSuspended) {
        qgcApp()->showMessage(QString("Connect not allowed: %1").arg(_connectionsSuspendedReason));
        return true;
    } else {
        return false;
    }
}

void LinkManager::setConnectionsSuspended(QString reason)
{
    _connectionsSuspended = true;
    _connectionsSuspendedReason = reason;
    Q_ASSERT(!reason.isEmpty());
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
    for (int i = 0; i < _linkConfigurations.count(); i++) {
        LinkConfiguration* linkConfig = _linkConfigurations.value<LinkConfiguration*>(i);
        if (linkConfig) {
            if(!linkConfig->isDynamic())
            {
                QString root = LinkConfiguration::settingsRoot();
                root += QString("/Link%1").arg(trueCount++);
                settings.setValue(root + "/name", linkConfig->name());
                settings.setValue(root + "/type", linkConfig->type());
                settings.setValue(root + "/auto", linkConfig->isAutoConnect());
                // Have the instance save its own values
                linkConfig->saveSettings(settings, root);
            }
        } else {
            qWarning() << "Internal error";
        }
    }
    QString root(LinkConfiguration::settingsRoot());
    settings.setValue(root + "/count", trueCount);
    emit linkConfigurationsChanged();
}

void LinkManager::loadLinkConfigurationList()
{
    bool linksChanged = false;
#ifdef QT_DEBUG
    bool mockPresent  = false;
#endif
    QSettings settings;
    // Is the group even there?
    if(settings.contains(LinkConfiguration::settingsRoot() + "/count")) {
        // Find out how many configurations we have
        int count = settings.value(LinkConfiguration::settingsRoot() + "/count").toInt();
        for(int i = 0; i < count; i++) {
            QString root(LinkConfiguration::settingsRoot());
            root += QString("/Link%1").arg(i);
            if(settings.contains(root + "/type")) {
                int type = settings.value(root + "/type").toInt();
                if((LinkConfiguration::LinkType)type < LinkConfiguration::TypeLast) {
                    if(settings.contains(root + "/name")) {
                        QString name = settings.value(root + "/name").toString();
                        if(!name.isEmpty()) {
                            LinkConfiguration* pLink = NULL;
                            bool autoConnect = settings.value(root + "/auto").toBool();
                            switch((LinkConfiguration::LinkType)type) {
#ifndef __ios__
                                case LinkConfiguration::TypeSerial:
                                    pLink = (LinkConfiguration*)new SerialConfiguration(name);
                                    break;
#endif
                                case LinkConfiguration::TypeUdp:
                                    pLink = (LinkConfiguration*)new UDPConfiguration(name);
                                    break;
                                case LinkConfiguration::TypeTcp:
                                    pLink = (LinkConfiguration*)new TCPConfiguration(name);
                                    break;
#ifdef QGC_ENABLE_BLUETOOTH
                                case LinkConfiguration::TypeBluetooth:
                                    pLink = (LinkConfiguration*)new BluetoothConfiguration(name);
                                    break;
#endif
#ifndef __mobile__
                                case LinkConfiguration::TypeLogReplay:
                                    pLink = (LinkConfiguration*)new LogReplayLinkConfiguration(name);
                                    break;
#endif
#ifdef QT_DEBUG
                                case LinkConfiguration::TypeMock:
                                    pLink = (LinkConfiguration*)new MockConfiguration(name);
                                    mockPresent = true;
                                    break;
#endif
                                default:
                                case LinkConfiguration::TypeLast:
                                    break;
                            }
                            if(pLink) {
                                //-- Have the instance load its own values
                                pLink->setAutoConnect(autoConnect);
                                pLink->loadSettings(settings, root);
                                _linkConfigurations.append(pLink);
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
    // Debug buids always add MockLink automatically (if one is not already there)
#ifdef QT_DEBUG
    if(!mockPresent)
    {
        MockConfiguration* pMock = new MockConfiguration("Mock Link PX4");
        pMock->setDynamic(true);
        _linkConfigurations.append(pMock);
        linksChanged = true;
    }
#endif

    if(linksChanged) {
        emit linkConfigurationsChanged();
    }
    // Enable automatic Serial PX4/3DR Radio hunting
    _configurationsLoaded = true;
}

#ifndef __ios__
SerialConfiguration* LinkManager::_autoconnectConfigurationsContainsPort(const QString& portName)
{
    QString searchPort = portName.trimmed();

    for (int i=0; i<_autoconnectConfigurations.count(); i++) {
        SerialConfiguration* linkConfig = _autoconnectConfigurations.value<SerialConfiguration*>(i);

        if (linkConfig) {
            if (linkConfig->portName() == searchPort) {
                return linkConfig;
            }
        } else {
            qWarning() << "Internal error";
        }
    }
    return NULL;
}
#endif

void LinkManager::_updateAutoConnectLinks(void)
{
    if (_connectionsSuspended || qgcApp()->runningUnitTests()) {
        return;
    }

    // Re-add UDP if we need to
    bool foundUDP = false;
    for (int i=0; i<_links.count(); i++) {
        LinkConfiguration* linkConfig = _links.value<LinkInterface*>(i)->getLinkConfiguration();
        if (linkConfig->type() == LinkConfiguration::TypeUdp && linkConfig->name() == _defaultUPDLinkName) {
            foundUDP = true;
            break;
        }
    }
    if (!foundUDP && _autoconnectUDP) {
        qCDebug(LinkManagerLog) << "New auto-connect UDP port added";
        UDPConfiguration* udpConfig = new UDPConfiguration(_defaultUPDLinkName);
        udpConfig->setLocalPort(QGC_UDP_LOCAL_PORT);
        udpConfig->setDynamic(true);
        _linkConfigurations.append(udpConfig);
        createConnectedLink(udpConfig);
        emit linkConfigurationsChanged();
    }

#ifndef __ios__
    QStringList currentPorts;
    QList<QGCSerialPortInfo> portList = QGCSerialPortInfo::availablePorts();

    // Iterate Comm Ports
    foreach (QGCSerialPortInfo portInfo, portList) {
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

        QGCSerialPortInfo::BoardType_t boardType = portInfo.boardType();

        if (boardType != QGCSerialPortInfo::BoardTypeUnknown) {
            if (portInfo.isBootloader()) {
                // Don't connect to bootloader
                qCDebug(LinkManagerLog) << "Waiting for bootloader to finish" << portInfo.systemLocation();
                continue;
            }

            if (_autoconnectConfigurationsContainsPort(portInfo.systemLocation())) {
                qCDebug(LinkManagerVerboseLog) << "Skipping existing autoconnect" << portInfo.systemLocation();
            } else if (!_autoconnectWaitList.contains(portInfo.systemLocation())) {
                // We don't connect to the port the first time we see it. The ability to correctly detect whether we
                // are in the bootloader is flaky from a cross-platform standpoint. So by putting it on a wait list
                // and only connect on the second pass we leave enough time for the board to boot up.
                qCDebug(LinkManagerLog) << "Waiting for next autoconnect pass" << portInfo.systemLocation();
                _autoconnectWaitList[portInfo.systemLocation()] = 1;
            } else if (++_autoconnectWaitList[portInfo.systemLocation()] * _autoconnectUpdateTimerMSecs > _autoconnectConnectDelayMSecs) {
                SerialConfiguration* pSerialConfig = NULL;

                _autoconnectWaitList.remove(portInfo.systemLocation());

                switch (boardType) {
                case QGCSerialPortInfo::BoardTypePX4FMUV1:
                case QGCSerialPortInfo::BoardTypePX4FMUV2:
                case QGCSerialPortInfo::BoardTypePX4FMUV4:
                    if (_autoconnectPixhawk) {
                        pSerialConfig = new SerialConfiguration(QString("Pixhawk on %1").arg(portInfo.portName().trimmed()));
                        pSerialConfig->setUsbDirect(true);
                    }
                    break;
                case QGCSerialPortInfo::BoardTypeAeroCore:
                    if (_autoconnectPixhawk) {
                        pSerialConfig = new SerialConfiguration(QString("AeroCore on %1").arg(portInfo.portName().trimmed()));
                        pSerialConfig->setUsbDirect(true);
                    }
                    break;
                case QGCSerialPortInfo::BoardTypePX4Flow:
                    if (_autoconnectPX4Flow) {
                        pSerialConfig = new SerialConfiguration(QString("PX4Flow on %1").arg(portInfo.portName().trimmed()));
                    }
                    break;
                case QGCSerialPortInfo::BoardTypeSikRadio:
                    if (_autoconnect3DRRadio) {
                        pSerialConfig = new SerialConfiguration(QString("SiK Radio on %1").arg(portInfo.portName().trimmed()));
                    }
                    break;
                default:
                    qWarning() << "Internal error";
                    continue;
                }

                if (pSerialConfig) {
                    qCDebug(LinkManagerLog) << "New auto-connect port added: " << pSerialConfig->name() << portInfo.systemLocation();
                    pSerialConfig->setBaud(boardType == QGCSerialPortInfo::BoardTypeSikRadio ? 57600 : 115200);
                    pSerialConfig->setDynamic(true);
                    pSerialConfig->setPortName(portInfo.systemLocation());
                    _autoconnectConfigurations.append(pSerialConfig);
                    createConnectedLink(pSerialConfig);
                }
            }
        }
    }

    // Now we go through the current configuration list and make sure any dynamic config has gone away
    QList<LinkConfiguration*>  _confToDelete;
    for (int i=0; i<_autoconnectConfigurations.count(); i++) {
        SerialConfiguration* linkConfig = _autoconnectConfigurations.value<SerialConfiguration*>(i);
        if (linkConfig) {
            if (!currentPorts.contains(linkConfig->portName())) {
                if (linkConfig->link()) {
                    if (linkConfig->link()->isConnected()) {
                        if (linkConfig->link()->active()) {
                            // We don't remove links which are still connected which have been active with a vehicle on them
                            // even though at this point the cable may have been pulled. Instead we wait for the user to
                            // Disconnect. Once the user disconnects, the link will be removed.
                            continue;
                        }
                    }
                }
                _confToDelete.append(linkConfig);
            }
        } else {
            qWarning() << "Internal error";
        }
    }

    // Now remove all configs that are gone
    foreach (LinkConfiguration* pDeleteConfig, _confToDelete) {
        qCDebug(LinkManagerLog) << "Removing unused autoconnect config" << pDeleteConfig->name();
        _autoconnectConfigurations.removeOne(pDeleteConfig);
        if (pDeleteConfig->link()) {
            disconnectLink(pDeleteConfig->link());
        }
        delete pDeleteConfig;
    }
#endif // __ios__
}

void LinkManager::shutdown(void)
{
    setConnectionsSuspended("Shutdown");
    disconnectAll();
}

void LinkManager::setAutoconnectUDP(bool autoconnect)
{
    if (_autoconnectUDP != autoconnect) {
        QSettings settings;

        settings.beginGroup(_settingsGroup);
        settings.setValue(_autoconnectUDPKey, autoconnect);

        _autoconnectUDP = autoconnect;
        emit autoconnectUDPChanged(autoconnect);
    }
}

void LinkManager::setAutoconnectPixhawk(bool autoconnect)
{
    if (_autoconnectPixhawk != autoconnect) {
        QSettings settings;

        settings.beginGroup(_settingsGroup);
        settings.setValue(_autoconnectPixhawkKey, autoconnect);

        _autoconnectPixhawk = autoconnect;
        emit autoconnectPixhawkChanged(autoconnect);
    }

}

void LinkManager::setAutoconnect3DRRadio(bool autoconnect)
{
    if (_autoconnect3DRRadio != autoconnect) {
        QSettings settings;

        settings.beginGroup(_settingsGroup);
        settings.setValue(_autoconnect3DRRadioKey, autoconnect);

        _autoconnect3DRRadio = autoconnect;
        emit autoconnect3DRRadioChanged(autoconnect);
    }

}

void LinkManager::setAutoconnectPX4Flow(bool autoconnect)
{
    if (_autoconnectPX4Flow != autoconnect) {
        QSettings settings;

        settings.beginGroup(_settingsGroup);
        settings.setValue(_autoconnectPX4FlowKey, autoconnect);

        _autoconnectPX4Flow = autoconnect;
        emit autoconnectPX4FlowChanged(autoconnect);
    }

}

QStringList LinkManager::linkTypeStrings(void) const
{
    //-- Must follow same order as enum LinkType in LinkConfiguration.h
    static QStringList list;
    if(!list.size())
    {
#ifndef __ios__
        list += "Serial";
#endif
        list += "UDP";
        list += "TCP";
#ifdef QGC_ENABLE_BLUETOOTH
        list += "Bluetooth";
#endif
#ifdef QT_DEBUG
        list += "Mock Link";
#endif
#ifndef __mobile__
        list += "Log Replay";
#endif
        Q_ASSERT(list.size() == (int)LinkConfiguration::TypeLast);
    }
    return list;
}

void LinkManager::_updateSerialPorts()
{
    _commPortList.clear();
    _commPortDisplayList.clear();
#ifndef __ios__
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo &info, portList)
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
#ifdef __ios__
    QStringList foo;
    return foo;
#else
    return SerialConfiguration::supportedBaudRates();
#endif
}

bool LinkManager::endConfigurationEditing(LinkConfiguration* config, LinkConfiguration* editedConfig)
{
    Q_ASSERT(config != NULL);
    Q_ASSERT(editedConfig != NULL);
    _fixUnnamed(editedConfig);
    config->copyFrom(editedConfig);
    saveLinkConfigurationList();
    // Tell link about changes (if any)
    config->updateSettings();
    // Discard temporary duplicate
    delete editedConfig;
    return true;
}

bool LinkManager::endCreateConfiguration(LinkConfiguration* config)
{
    Q_ASSERT(config != NULL);
    _fixUnnamed(config);
    _linkConfigurations.append(config);
    saveLinkConfigurationList();
    return true;
}

LinkConfiguration* LinkManager::createConfiguration(int type, const QString& name)
{
#ifndef __ios__
    if((LinkConfiguration::LinkType)type == LinkConfiguration::TypeSerial)
        _updateSerialPorts();
#endif
    return LinkConfiguration::createSettings(type, name);
}

LinkConfiguration* LinkManager::startConfigurationEditing(LinkConfiguration* config)
{
    Q_ASSERT(config != NULL);
#ifndef __ios__
    if(config->type() == LinkConfiguration::TypeSerial)
        _updateSerialPorts();
#endif
    return LinkConfiguration::duplicateSettings(config);
}


void LinkManager::_fixUnnamed(LinkConfiguration* config)
{
    Q_ASSERT(config != NULL);
    //-- Check for "Unnamed"
    if (config->name() == "Unnamed") {
        switch(config->type()) {
#ifndef __ios__
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
                            QString("TCP Link %1:%2").arg(tconfig->address().toString()).arg((int)tconfig->port()));
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
#ifndef __mobile__
            case LinkConfiguration::TypeLogReplay: {
                    LogReplayLinkConfiguration* tconfig = dynamic_cast<LogReplayLinkConfiguration*>(config);
                    if(tconfig) {
                        config->setName(QString("Log Replay %1").arg(tconfig->logFilenameShort()));
                    }
                }
                break;
#endif
#ifdef QT_DEBUG
            case LinkConfiguration::TypeMock:
                config->setName(
                    QString("Mock Link"));
                break;
#endif
            case LinkConfiguration::TypeLast:
            default:
                break;
        }
    }
}

void LinkManager::removeConfiguration(LinkConfiguration* config)
{
    Q_ASSERT(config != NULL);
    LinkInterface* iface = config->link();
    if(iface) {
        disconnectLink(iface);
    }
    // Remove configuration
    _linkConfigurations.removeOne(config);
    delete config;
    // Save list
    saveLinkConfigurationList();
}

bool LinkManager::isAutoconnectLink(LinkInterface* link)
{
    return _autoconnectConfigurations.contains(link->getLinkConfiguration());
}

bool LinkManager::isBluetoothAvailable(void)
{
    return qgcApp()->isBluetoothAvailable();
}

#ifndef __ios__
void LinkManager::_activeLinkCheck(void)
{
    SerialLink* link = NULL;
    bool found = false;

    if (_activeLinkCheckList.count() != 0) {
        link = _activeLinkCheckList.takeFirst();
        if (_links.contains(link) && link->isConnected()) {
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
            link = NULL;
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
            if (arguments[1].value<QByteArray>().contains("nsh>")) {
                foundNSHPrompt = true;
            }
        }

        qgcApp()->showMessage(foundNSHPrompt ?
                                  QStringLiteral("Please check to make sure you have an SD Card inserted in your Vehicle and try again.") :
                                  QStringLiteral("Your Vehicle is not responding. If this continues shutdown QGroundControl, restart the Vehicle letting it boot completely, then start QGroundControl."));
    }
}
#endif
