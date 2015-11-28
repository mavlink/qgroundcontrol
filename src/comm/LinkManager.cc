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

#ifndef __ios__
#include "QGCSerialPortInfo.h"
#endif

#include "LinkManager.h"
#include "MainWindow.h"
#include "QGCMessageBox.h"
#include "QGCApplication.h"
#include "QGCApplication.h"
#include "UDPLink.h"
#include "TCPLink.h"

QGC_LOGGING_CATEGORY(LinkManagerLog, "LinkManagerLog")
QGC_LOGGING_CATEGORY(LinkManagerVerboseLog, "LinkManagerVerboseLog")

const char* LinkManager::_settingsGroup =           "LinkManager";
const char* LinkManager::_autoconnectUDPKey =       "AutoconnectUDP";
const char* LinkManager::_autoconnectPixhawkKey =   "AutoconnectPixhawk";
const char* LinkManager::_autoconnect3DRRadioKey =  "Autoconnect3DRRadio";
const char* LinkManager::_autoconnectPX4FlowKey =   "AutoconnectPX4Flow";

LinkManager::LinkManager(QGCApplication* app)
    : QGCTool(app)
    , _configUpdateSuspended(false)
    , _configurationsLoaded(false)
    , _connectionsSuspended(false)
    , _mavlinkChannelsUsedBitMask(0)
    , _mavlinkProtocol(NULL)
    , _autoconnectUDPConfig(NULL)
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
}

LinkManager::~LinkManager()
{
    if (anyActiveLinks()) {
        qWarning() << "Why are there still active links?";
    }
}

void LinkManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);

   _mavlinkProtocol = _toolbox->mavlinkProtocol();
   connect(_mavlinkProtocol, &MAVLinkProtocol::vehicleHeartbeatInfo, this, &LinkManager::_vehicleHeartbeatInfo);

    connect(&_portListTimer, &QTimer::timeout, this, &LinkManager::_updateAutoConnectLinks);
    _portListTimer.start(1000);
    
}

LinkInterface* LinkManager::createConnectedLink(LinkConfiguration* config, bool autoconnectLink)
{
    Q_ASSERT(config);
    LinkInterface* pLink = NULL;
    switch(config->type()) {
#ifndef __ios__
        case LinkConfiguration::TypeSerial:
            pLink = new SerialLink(dynamic_cast<SerialConfiguration*>(config));
            break;
#endif
        case LinkConfiguration::TypeUdp:
            pLink = new UDPLink(dynamic_cast<UDPConfiguration*>(config));
            break;
        case LinkConfiguration::TypeTcp:
            pLink = new TCPLink(dynamic_cast<TCPConfiguration*>(config));
            break;
        case LinkConfiguration::TypeLogReplay:
            pLink = new LogReplayLink(dynamic_cast<LogReplayLinkConfiguration*>(config));
            break;
#ifdef QT_DEBUG
        case LinkConfiguration::TypeMock:
            pLink = new MockLink(dynamic_cast<MockConfiguration*>(config));
            break;
#endif
    }
    if(pLink) {
        pLink->setAutoconnect(autoconnectLink);
        _addLink(pLink);
        connectLink(pLink);
    }
    return pLink;
}

LinkInterface* LinkManager::createConnectedLink(const QString& name, bool autoconnectLink)
{
    Q_ASSERT(name.isEmpty() == false);
    for(int i = 0; i < _linkConfigurations.count(); i++) {
        LinkConfiguration* conf = _linkConfigurations.value<LinkConfiguration*>(i);
        if(conf && conf->name() == name)
            return createConnectedLink(conf, autoconnectLink);
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
        // Find a mavlink channel to use for this link
        for (int i=0; i<32; i++) {
            if (!(_mavlinkChannelsUsedBitMask && 1 << i)) {
                mavlink_reset_channel_status(i);
                link->_setMavlinkChannel(i);
                _mavlinkChannelsUsedBitMask |= i << i;
                break;
            }
        }
        
        _links.append(link);
        emit newLink(link);
    }

    // MainWindow may be around when doing things like running unit tests
    if (MainWindow::instance()) {
        connect(link, &LinkInterface::communicationError, _app, &QGCApplication::criticalMessageBoxOnMainThread);
    }

    connect(link, &LinkInterface::bytesReceived,    _mavlinkProtocol, &MAVLinkProtocol::receiveBytes);
    connect(link, &LinkInterface::connected,        _mavlinkProtocol, &MAVLinkProtocol::linkConnected);
    connect(link, &LinkInterface::disconnected,     _mavlinkProtocol, &MAVLinkProtocol::linkDisconnected);
    _mavlinkProtocol->resetMetadataForLink(link);

    connect(link, &LinkInterface::connected,    this, &LinkManager::_linkConnected);
    connect(link, &LinkInterface::disconnected, this, &LinkManager::_linkDisconnected);
}

void LinkManager::disconnectAll(bool disconnectAutoconnectLink)
{
    // Walk list in reverse order to preserve indices during delete
    for (int i=_links.count()-1; i>=0; i--) {
        disconnectLink(_links.value<LinkInterface*>(i), disconnectAutoconnectLink);
    }
}

bool LinkManager::connectLink(LinkInterface* link)
{
    Q_ASSERT(link);

    if (_connectionsSuspendedMsg()) {
        return false;
    }

    bool previousAnyConnectedLinks = anyConnectedLinks();
    bool previousAnyNonAutoconnectConnectedLinks = anyNonAutoconnectConnectedLinks();

    if (link->_connect()) {
        if (!previousAnyConnectedLinks) {
            emit anyConnectedLinksChanged(true);
        }
        if (!previousAnyNonAutoconnectConnectedLinks && anyNonAutoconnectConnectedLinks()) {
            emit anyNonAutoconnectConnectedLinksChanged(true);
        }
        return true;
    } else {
        return false;
    }
}

bool LinkManager::disconnectLink(LinkInterface* link, bool disconnectAutoconnectLink)
{
    Q_ASSERT(link);

    if (disconnectAutoconnectLink || !link->autoconnect()) {
        link->_disconnect();
        LinkConfiguration* config = link->getLinkConfiguration();
        if(config) {
            config->setLink(NULL);
        }
        _deleteLink(link);
        return true;
    }

    return false;
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
        QGCMessageBox::information(tr("Connect not allowed"),
                                   tr("Connect not allowed: %1").arg(_connectionsSuspendedReason));
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

void LinkManager::suspendConfigurationUpdates(bool suspend)
{
    _configUpdateSuspended = suspend;
}

void LinkManager::saveLinkConfigurationList()
{
    QSettings settings;
    settings.remove(LinkConfiguration::settingsRoot());

    for (int i=0; i<_linkConfigurations.count(); i++) {
        LinkConfiguration* linkConfig = _linkConfigurations.value<LinkConfiguration*>(i);

        if (linkConfig) {
            if(!linkConfig->isDynamic())
            {
                QString root = LinkConfiguration::settingsRoot();
                root += QString("/Link%1").arg(i);
                settings.setValue(root + "/name", linkConfig->name());
                settings.setValue(root + "/type", linkConfig->type());
                // Have the instance save its own values
                linkConfig->saveSettings(settings, root);
            }
        } else {
            qWarning() << "Internal error";
        }
    }

    QString root(LinkConfiguration::settingsRoot());
    settings.setValue(root + "/count", _linkConfigurations.count());
    emit linkConfigurationChanged();
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
                int type = settings.value(root + "/type").toInt();
                if(type < LinkConfiguration::TypeLast) {
                    if(settings.contains(root + "/name")) {
                        QString name = settings.value(root + "/name").toString();
                        if(!name.isEmpty()) {
                            LinkConfiguration* pLink = NULL;
                            switch(type) {
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
                                case LinkConfiguration::TypeLogReplay:
                                    pLink = (LinkConfiguration*)new LogReplayLinkConfiguration(name);
                                    break;
#ifdef QT_DEBUG
                                case LinkConfiguration::TypeMock:
                                    pLink = (LinkConfiguration*)new MockConfiguration(name);
                                    break;
#endif
                            }
                            if(pLink) {
                                // Have the instance load its own values
                                pLink->loadSettings(settings, root);
                                _linkConfigurations.append(pLink);
                                linksChanged = true;
                            }
                        } else {
                            qWarning() << "Link Configuration " << root << " has an empty name." ;
                        }
                    } else {
                        qWarning() << "Link Configuration " << root << " has no name." ;
                    }
                } else {
                    qWarning() << "Link Configuration " << root << " an invalid type: " << type;
                }
            } else {
                qWarning() << "Link Configuration " << root << " has no type." ;
            }
        }
    }
    
    // Debug buids always add MockLink automatically
#ifdef QT_DEBUG
    MockConfiguration* pMock = new MockConfiguration("Mock Link PX4");
    pMock->setDynamic(true);
    _linkConfigurations.append(pMock);
    linksChanged = true;
#endif

    if(linksChanged) {
        emit linkConfigurationChanged();
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

    if (_autoconnectUDP && !_autoconnectUDPConfig) {
        _autoconnectUDPConfig = new UDPConfiguration("Default UDP Link");
        _autoconnectUDPConfig->setLocalPort(QGC_UDP_LOCAL_PORT);
        _autoconnectUDPConfig->setDynamic(true);
        createConnectedLink(_autoconnectUDPConfig, true /* persistenLink */);
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
                continue;
            }
            
            if (!_autoconnectConfigurationsContainsPort(portInfo.systemLocation())) {
                SerialConfiguration* pSerialConfig = NULL;

                switch (boardType) {
                case QGCSerialPortInfo::BoardTypePX4FMUV1:
                case QGCSerialPortInfo::BoardTypePX4FMUV2:
                    if (_autoconnectPixhawk) {
                        pSerialConfig = new SerialConfiguration(QString("Pixhawk on %1").arg(portInfo.portName().trimmed()));
                    }
                    break;
                case QGCSerialPortInfo::BoardTypeAeroCore:
                    if (_autoconnectPixhawk) {
                        pSerialConfig = new SerialConfiguration(QString("AeroCore on %1").arg(portInfo.portName().trimmed()));
                    }
                    break;
                case QGCSerialPortInfo::BoardTypePX4Flow:
                    if (_autoconnectPX4Flow) {
                        pSerialConfig = new SerialConfiguration(QString("PX4Flow on %1").arg(portInfo.portName().trimmed()));
                    }
                    break;
                case QGCSerialPortInfo::BoardType3drRadio:
                    if (_autoconnect3DRRadio) {
                        pSerialConfig = new SerialConfiguration(QString("3DR Radio on %1").arg(portInfo.portName().trimmed()));
                    }
                    break;
                default:
                    qWarning() << "Internal error";
                    continue;
                }

                if (pSerialConfig) {
                    qCDebug(LinkManagerLog) << "New auto-connect port added: " << pSerialConfig->name() << portInfo.systemLocation();
                    pSerialConfig->setBaud(boardType == QGCSerialPortInfo::BoardType3drRadio ? 57600 : 115200);
                    pSerialConfig->setDynamic(true);
                    pSerialConfig->setPortName(portInfo.systemLocation());

                    _autoconnectConfigurations.append(pSerialConfig);

                    createConnectedLink(pSerialConfig, true /* persistenLink */);
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
                _confToDelete.append(linkConfig);
            }
        } else {
            qWarning() << "Internal error";
        }
    }

    // Now disconnect/remove all links that are gone
    foreach (LinkConfiguration* pDeleteConfig, _confToDelete) {
        LinkInterface* link = pDeleteConfig->link();
        if (link) {
            disconnectLink(link, true /* disconnectAutoconnectLink */);
        }
        _autoconnectConfigurations.removeOne(pDeleteConfig);
        delete pDeleteConfig;
    }
#endif // __ios__
}

bool LinkManager::anyConnectedLinks(void)
{
    bool found = false;
    for (int i=0; i<_links.count(); i++) {
        LinkInterface* link = _links.value<LinkInterface*>(i);

        if (link && link->isConnected()) {
            found = true;
            break;
        }
    }
    return found;
}

bool LinkManager::anyNonAutoconnectConnectedLinks(void)
{
    // FIXME: Should remove this duplication with anyConnectedLinks
    bool found = false;
    for (int i=0; i<_links.count(); i++) {
        LinkInterface* link = _links.value<LinkInterface*>(i);

        if (link && !link->autoconnect() && link->isConnected()) {
            found = true;
            break;
        }
    }
    return found;
}

bool LinkManager::anyActiveLinks(void)
{
    bool found = false;
    for (int i=0; i<_links.count(); i++) {
        LinkInterface* link = _links.value<LinkInterface*>(i);

        if (link && link->active()) {
            found = true;
            break;
        }
    }
    return found;
}

bool LinkManager::anyNonAutoconnectActiveLinks(void)
{
    // FIXME: Should remove this duplication with anyActiveLinks
    bool found = false;
    for (int i=0; i<_links.count(); i++) {
        LinkInterface* link = _links.value<LinkInterface*>(i);

        if (link && !link->autoconnect() && link->active()) {
            found = true;
            break;
        }
    }
    return found;
}

void LinkManager::_vehicleHeartbeatInfo(LinkInterface* link, int vehicleId, int vehicleMavlinkVersion, int vehicleFirmwareType, int vehicleType)
{
    if (!link->active() && !_ignoreVehicleIds.contains(vehicleId)) {
        qCDebug(LinkManagerLog) << "New heartbeat on link:vehicleId:vehicleMavlinkVersion:vehicleFirmwareType:vehicleType "
                                << link->getName()
                                << vehicleId
                                << vehicleMavlinkVersion
                                << vehicleFirmwareType
                                << vehicleType;

        if (vehicleId == _mavlinkProtocol->getSystemId()) {
            _app->showToolBarMessage(QString("Warning: A vehicle is using the same system id as QGroundControl: %1").arg(vehicleId));
        }

        QSettings settings;
        bool mavlinkVersionCheck = settings.value("VERSION_CHECK_ENABLED", true).toBool();
        if (mavlinkVersionCheck && vehicleMavlinkVersion != MAVLINK_VERSION) {
            _ignoreVehicleIds += vehicleId;
            _app->showToolBarMessage(QString("The MAVLink protocol version on vehicle #%1 and QGroundControl differ! "
                                                 "It is unsafe to use different MAVLink versions. "
                                                 "QGroundControl therefore refuses to connect to vehicle #%1, which sends MAVLink version %2 (QGroundControl uses version %3).").arg(vehicleId).arg(vehicleMavlinkVersion).arg(MAVLINK_VERSION));
            return;
        }

        bool previousAnyActiveLinks = anyActiveLinks();
        bool previousAnyNonAutoconnectActiveLinks = anyNonAutoconnectActiveLinks();

        link->setActive(true);
        emit linkActive(link, vehicleId, vehicleFirmwareType, vehicleType);

        if (!previousAnyActiveLinks) {
            emit anyActiveLinksChanged(true);
        }
        if (!previousAnyNonAutoconnectActiveLinks && anyNonAutoconnectActiveLinks()) {
            emit anyNonAutoconnectActiveLinksChanged(true);
        }
    }
}

void LinkManager::shutdown(void)
{
    setConnectionsSuspended("Shutdown");
    disconnectAll(true /* disconnectAutoconnectLink */);
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
