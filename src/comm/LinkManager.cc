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

QGC_LOGGING_CATEGORY(LinkManagerLog, "LinkManagerLog")

LinkManager::LinkManager(QGCApplication* app)
    : QGCTool(app)
    , _configUpdateSuspended(false)
    , _configurationsLoaded(false)
    , _connectionsSuspended(false)
    , _mavlinkChannelsUsedBitMask(0)
    , _nullSharedLink(NULL)
    , _mavlinkProtocol(NULL)
{

}

LinkManager::~LinkManager()
{
    // Clear configuration list
    while(_linkConfigurations.count()) {
        LinkConfiguration* pLink = _linkConfigurations.at(0);
        if(pLink) delete pLink;
        _linkConfigurations.removeAt(0);
    }

    if (anyConnectedLinks()) {
        qWarning() << "Why are there still connected links?";
    }
}

void LinkManager::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);

   _mavlinkProtocol = _toolbox->mavlinkProtocol();
   connect(_mavlinkProtocol, &MAVLinkProtocol::vehicleHeartbeatInfo, this, &LinkManager::_vehicleHeartbeatInfo);

#ifndef __ios__
    connect(&_portListTimer, &QTimer::timeout, this, &LinkManager::_updateAutoConnectLinks);
    _portListTimer.start(1000);
#endif
}

LinkInterface* LinkManager::createConnectedLink(LinkConfiguration* config, bool persistentLink)
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
        pLink->setPersistentLink(persistentLink);
        _addLink(pLink);
        connectLink(pLink);
    }
    return pLink;
}

LinkInterface* LinkManager::createConnectedLink(const QString& name, bool persistentLink)
{
    Q_ASSERT(name.isEmpty() == false);
    for(int i = 0; i < _linkConfigurations.count(); i++) {
        LinkConfiguration* conf = _linkConfigurations.at(i);
        if(conf && conf->name() == name)
            return createConnectedLink(conf, persistentLink);
    }
    return NULL;
}

void LinkManager::_addLink(LinkInterface* link)
{
    Q_ASSERT(link);

    _linkListMutex.lock();

    if (!containsLink(link)) {
        // Find a mavlink channel to use for this link
        for (int i=0; i<32; i++) {
            if (!(_mavlinkChannelsUsedBitMask && 1 << i)) {
                mavlink_reset_channel_status(i);
                link->_setMavlinkChannel(i);
                _mavlinkChannelsUsedBitMask |= i << i;
                break;
            }
        }
        
        _links.append(QSharedPointer<LinkInterface>(link));
        _linkListMutex.unlock();
        emit newLink(link);
    } else {
        _linkListMutex.unlock();
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

void LinkManager::disconnectAll(bool disconnectPersistentLink)
{
    // Make a copy so the list is modified out from under us
    QList<SharedLinkInterface> links = _links;

    foreach (SharedLinkInterface sharedLink, links) {
        Q_ASSERT(sharedLink.data());
        disconnectLink(sharedLink.data(), disconnectPersistentLink);
    }
}

bool LinkManager::connectLink(LinkInterface* link)
{
    Q_ASSERT(link);

    if (_connectionsSuspendedMsg()) {
        return false;
    }

    if (link->_connect()) {
        return true;
    } else {
        return false;
    }
}

bool LinkManager::disconnectLink(LinkInterface* link, bool disconnectPersistenLink)
{
    Q_ASSERT(link);

    link->setActive(false);
    emit linkInactive(link);

    if (disconnectPersistenLink || !link->persistentLink()) {
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
    Q_ASSERT(link);

    _linkListMutex.lock();
    
    // Free up the mavlink channel associated with this link
    _mavlinkChannelsUsedBitMask &= ~(1 << link->getMavlinkChannel());

    bool found = false;
    for (int i=0; i<_links.count(); i++) {
        if (_links[i].data() == link) {
            _links.removeAt(i);
            found = true;
            break;
        }
    }
    Q_UNUSED(found);
    Q_ASSERT(found);

    _linkListMutex.unlock();

    // Emit removal of link
    emit linkDeleted(link);
}

/**
 *
 */
const QList<LinkInterface*> LinkManager::getLinks()
{
    QList<LinkInterface*> list;
    
    foreach (SharedLinkInterface sharedLink, _links) {
        LinkInterface* link = sharedLink.data();
        if (!link->persistentLink()) {
            list << sharedLink.data();
        }
    }
    
    return list;
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

void LinkManager::addLinkConfiguration(LinkConfiguration* link)
{
    Q_ASSERT(link != NULL);
    //-- If not there already, add it
    int idx = _linkConfigurations.indexOf(link);
    if(idx < 0)
    {
        _linkConfigurations.append(link);
    }
}

void LinkManager::removeLinkConfiguration(LinkConfiguration *link)
{
    Q_ASSERT(link != NULL);
    int idx = _linkConfigurations.indexOf(link);
    if(idx >= 0)
    {
        _linkConfigurations.removeAt(idx);
        delete link;
    }
}

const QList<LinkConfiguration*> LinkManager::getLinkConfigurationList()
{
    return _linkConfigurations;
}

void LinkManager::suspendConfigurationUpdates(bool suspend)
{
    _configUpdateSuspended = suspend;
}

void LinkManager::saveLinkConfigurationList()
{
    QSettings settings;
    settings.remove(LinkConfiguration::settingsRoot());
    int index = 0;
    foreach (LinkConfiguration* pLink, _linkConfigurations) {
        Q_ASSERT(pLink != NULL);
        if(!pLink->isDynamic())
        {
            QString root = LinkConfiguration::settingsRoot();
            root += QString("/Link%1").arg(index++);
            settings.setValue(root + "/name", pLink->name());
            settings.setValue(root + "/type", pLink->type());
            // Have the instance save its own values
            pLink->saveSettings(settings, root);
        }
    }
    QString root(LinkConfiguration::settingsRoot());
    settings.setValue(root + "/count", index);
    emit linkConfigurationChanged();
}

void LinkManager::loadLinkConfigurationList()
{
    UDPConfiguration*   defaultUdpLinkConfig = NULL;
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
                                addLinkConfiguration(pLink);
                                linksChanged = true;
                                // Check for UDP links
                                if(pLink->type() == LinkConfiguration::TypeUdp) {
                                    UDPConfiguration* uLink = dynamic_cast<UDPConfiguration*>(pLink);
                                    if(uLink && uLink->localPort() == QGC_UDP_LOCAL_PORT) {
                                        defaultUdpLinkConfig = uLink;
                                    }
                                }
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
    addLinkConfiguration(pMock);
    linksChanged = true;
#endif

    //-- If we don't have a configured UDP link, create a default one
    if(!defaultUdpLinkConfig) {
        defaultUdpLinkConfig = new UDPConfiguration("Default UDP Link");
        defaultUdpLinkConfig->setLocalPort(QGC_UDP_LOCAL_PORT);
        defaultUdpLinkConfig->setDynamic(true);
    }
    
    if(linksChanged) {
        emit linkConfigurationChanged();
    }

    // Start the default UDP link
    createConnectedLink(defaultUdpLinkConfig, true /* persistenLink */);

    // Enable automatic Serial PX4/3DR Radio hunting
    _configurationsLoaded = true;
}

#ifndef __ios__
SerialConfiguration* LinkManager::_findSerialConfiguration(const QString& portName)
{
    QString searchPort = portName.trimmed();
    foreach (LinkConfiguration* pLink, _linkConfigurations) {
        Q_ASSERT(pLink != NULL);
        if(pLink->type() == LinkConfiguration::TypeSerial) {
            SerialConfiguration* pSerial = dynamic_cast<SerialConfiguration*>(pLink);
            if(pSerial->portName() == searchPort) {
                return pSerial;
            }
        }
    }
    return NULL;
}
#endif

#ifndef __ios__
void LinkManager::_updateAutoConnectLinks(void)
{
    if (_configUpdateSuspended || !_configurationsLoaded) {
        return;
    }
    bool saveList = false;
    QStringList currentPorts;
    QList<QGCSerialPortInfo> portList = QGCSerialPortInfo::availablePorts();

    // Iterate Comm Ports
    foreach (QGCSerialPortInfo portInfo, portList) {
#if 1
        // Too noisy for most logging, so turn on as needed
        qCDebug(LinkManagerLog) << "-----------------------------------------------------";
        qCDebug(LinkManagerLog) << "portName:         " << portInfo.portName();
        qCDebug(LinkManagerLog) << "systemLocation:   " << portInfo.systemLocation();
        qCDebug(LinkManagerLog) << "description:      " << portInfo.description();
        qCDebug(LinkManagerLog) << "manufacturer:     " << portInfo.manufacturer();
        qCDebug(LinkManagerLog) << "serialNumber:     " << portInfo.serialNumber();
        qCDebug(LinkManagerLog) << "vendorIdentifier: " << portInfo.vendorIdentifier();
#endif
        // Save port name
        currentPorts << portInfo.systemLocation();

        QGCSerialPortInfo::BoardType_t boardType = portInfo.boardType();

        if (boardType != QGCSerialPortInfo::BoardTypeUnknown) {
            if (portInfo.isBootloader()) {
                // Don't connect to bootloader
                continue;
            }
            
            SerialConfiguration* pSerialConfig = _findSerialConfiguration(portInfo.systemLocation());
            if (!pSerialConfig) {
                switch (boardType) {
                case QGCSerialPortInfo::BoardTypePX4FMUV1:
                case QGCSerialPortInfo::BoardTypePX4FMUV2:
                    pSerialConfig = new SerialConfiguration(QString("Pixhawk on %1").arg(portInfo.portName().trimmed()));
                    break;
                case QGCSerialPortInfo::BoardTypeAeroCore:
                    pSerialConfig = new SerialConfiguration(QString("AeroCore on %1").arg(portInfo.portName().trimmed()));
                    break;
                case QGCSerialPortInfo::BoardTypePX4Flow:
                    pSerialConfig = new SerialConfiguration(QString("PX4Flow on %1").arg(portInfo.portName().trimmed()));
                    break;
                case QGCSerialPortInfo::BoardType3drRadio:
                    pSerialConfig = new SerialConfiguration(QString("3DR Radio on %1").arg(portInfo.portName().trimmed()));
                default:
                    qWarning() << "Internal error";
                    continue;
                }

                pSerialConfig->setBaud(boardType == QGCSerialPortInfo::BoardType3drRadio ? 57600 : 115200);
                pSerialConfig->setDynamic(true);
                pSerialConfig->setPortName(portInfo.systemLocation());
                addLinkConfiguration(pSerialConfig);
                saveList = true;
            }

            LinkInterface* link = pSerialConfig->getLink();
            if (link) {
                if (link->isConnected()) {
                    link->setPersistentLink(true);
                    connectLink(link);
                }
            } else {
                createConnectedLink(pSerialConfig, true /* persistenLink */);
            }
        }
    }

    // Now we go through the current configuration list and make sure any dynamic config has gone away
    QList<LinkConfiguration*>  _confToDelete;
    foreach (LinkConfiguration* pLinkConfig, _linkConfigurations) {
        // We only care about dynamic serial links
        if (pLinkConfig->isDynamic() && pLinkConfig->type() == LinkConfiguration::TypeSerial) {
            SerialConfiguration* pSerialConfig = dynamic_cast<SerialConfiguration*>(pLinkConfig);
            if (!currentPorts.contains(pSerialConfig->portName())) {
                _confToDelete.append(pSerialConfig);
            }
        }
    }

    // Now disconnect/remove all links that are gone
    foreach (LinkConfiguration* pDeleteConfig, _confToDelete) {
        saveList = true;

        LinkInterface* link = pDeleteConfig->getLink();
        if (link) {
            disconnectLink(link, true /* disconnectPersistentLink */);
        }
        removeLinkConfiguration(pDeleteConfig);
    }

    if (saveList) {
        saveLinkConfigurationList();
    }
}
#endif

bool LinkManager::containsLink(LinkInterface* link)
{
    bool found = false;
    foreach (SharedLinkInterface sharedLink, _links) {
        if (sharedLink.data() == link) {
            found = true;
            break;
        }
    }
    return found;
}

bool LinkManager::anyConnectedLinks(void)
{
    bool found = false;
    foreach (SharedLinkInterface sharedLink, _links) {
        LinkInterface* link = sharedLink.data();
        if (link->isConnected()) {
            found = true;
            break;
        }
    }
    return found;
}

bool LinkManager::anyActiveLinks(void)
{
    bool found = false;
    foreach (SharedLinkInterface sharedLink, _links) {
        LinkInterface* link = sharedLink.data();
        if (link->active()) {
            found = true;
            break;
        }
    }
    return found;
}

SharedLinkInterface& LinkManager::sharedPointerForLink(LinkInterface* link)
{
    for (int i=0; i<_links.count(); i++) {
        if (_links[i].data() == link) {
            return _links[i];
        }
    }

    qWarning() << "Internal error";
    return _nullSharedLink;
}

void LinkManager::_vehicleHeartbeatInfo(LinkInterface* link, int vehicleId, int vehicleMavlinkVersion, int vehicleFirmwareType, int vehicleType)
{
    if (!link->active() && !_ignoreVehicleIds.contains(vehicleId)) {
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

        link->setActive(true);
        emit linkActive(link, vehicleId, vehicleFirmwareType, vehicleType);
    }
}
