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
#ifdef __android__
#include "qserialportinfo.h"
#else
#include <QSerialPortInfo>
#endif

#include "LinkManager.h"
#include "MainWindow.h"
#include "QGCMessageBox.h"
#include "QGCApplication.h"

IMPLEMENT_QGC_SINGLETON(LinkManager, LinkManager)


/**
 * @brief Private singleton constructor
 *
 * This class implements the singleton design pattern and has therefore only a private constructor.
 **/
LinkManager::LinkManager(QObject* parent)
    : QGCSingleton(parent)
    , _configUpdateSuspended(false)
    , _configurationsLoaded(false)
    , _connectionsSuspended(false)
    , _mavlinkChannelsUsedBitMask(0)
    , _nullSharedLink(NULL)
{
    connect(&_portListTimer, &QTimer::timeout, this, &LinkManager::_updateConfigurationList);
    _portListTimer.start(1000);
}

LinkManager::~LinkManager()
{
    // Clear configuration list
    while(_linkConfigurations.count()) {
        LinkConfiguration* pLink = _linkConfigurations.at(0);
        if(pLink) delete pLink;
        _linkConfigurations.removeAt(0);
    }
    Q_ASSERT_X(_links.count() == 0, "LinkManager", "LinkManager::_shutdown should have been called previously");
}

LinkInterface* LinkManager::createConnectedLink(LinkConfiguration* config)
{
    Q_ASSERT(config);
    LinkInterface* pLink = NULL;
    switch(config->type()) {
        case LinkConfiguration::TypeSerial:
            pLink = new SerialLink(dynamic_cast<SerialConfiguration*>(config));
            break;
        case LinkConfiguration::TypeUdp:
            pLink = new UDPLink(dynamic_cast<UDPConfiguration*>(config));
            break;
        case LinkConfiguration::TypeTcp:
            pLink = new TCPLink(dynamic_cast<TCPConfiguration*>(config));
            break;
#ifdef QT_DEBUG
        case LinkConfiguration::TypeMock:
            pLink = new MockLink(dynamic_cast<MockConfiguration*>(config));
            break;
#endif
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
        LinkConfiguration* conf = _linkConfigurations.at(i);
        if(conf && conf->name() == name)
            return createConnectedLink(conf);
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
        connect(link, &LinkInterface::communicationError, qgcApp(), &QGCApplication::criticalMessageBoxOnMainThread);
    }

    MAVLinkProtocol* mavlink = MAVLinkProtocol::instance();
    connect(link, &LinkInterface::bytesReceived, mavlink, &MAVLinkProtocol::receiveBytes);
    connect(link, &LinkInterface::connected, mavlink, &MAVLinkProtocol::linkConnected);
    connect(link, &LinkInterface::disconnected, mavlink, &MAVLinkProtocol::linkDisconnected);
    mavlink->resetMetadataForLink(link);

    connect(link, &LinkInterface::connected, this, &LinkManager::_linkConnected);
    connect(link, &LinkInterface::disconnected, this, &LinkManager::_linkDisconnected);
}

bool LinkManager::connectAll()
{
    if (_connectionsSuspendedMsg()) {
        return false;
    }

    bool allConnected = true;

    foreach (SharedLinkInterface sharedLink, _links) {
        Q_ASSERT(sharedLink.data());
        if (!sharedLink.data()->_connect()) {
            allConnected = false;
        }
    }

    return allConnected;
}

bool LinkManager::disconnectAll()
{
    bool allDisconnected = true;

    // Make a copy so the list is modified out from under us
    QList<SharedLinkInterface> links = _links;

    foreach (SharedLinkInterface sharedLink, links) {
        Q_ASSERT(sharedLink.data());
        if (!disconnectLink(sharedLink.data())) {
            allDisconnected = false;
        }
    }

    return allDisconnected;
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

bool LinkManager::disconnectLink(LinkInterface* link)
{
    Q_ASSERT(link);
    if (link->_disconnect()) {
        LinkConfiguration* config = link->getLinkConfiguration();
        if(config) {
            config->setLink(NULL);
        }
        _deleteLink(link);
        return true;
    } else {
        return false;
    }
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
        list << sharedLink.data();
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

void LinkManager::_shutdown(void)
{
    while (_links.count() != 0) {
        disconnectLink(_links[0].data());
    }
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
            settings.setValue(root + "/preferred", pLink->isPreferred());
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
                            bool preferred = false;
                            if(settings.contains(root + "/preferred")) {
                                preferred = settings.value(root + "/preferred").toBool();
                            }
                            LinkConfiguration* pLink = NULL;
                            switch(type) {
                                case LinkConfiguration::TypeSerial:
                                    pLink = (LinkConfiguration*)new SerialConfiguration(name);
                                    pLink->setPreferred(preferred);
                                    break;
                                case LinkConfiguration::TypeUdp:
                                    pLink = (LinkConfiguration*)new UDPConfiguration(name);
                                    pLink->setPreferred(preferred);
                                    break;
                                case LinkConfiguration::TypeTcp:
                                    pLink = (LinkConfiguration*)new TCPConfiguration(name);
                                    pLink->setPreferred(preferred);
                                    break;
#ifdef QT_DEBUG
                                case LinkConfiguration::TypeMock:
                                    pLink = (LinkConfiguration*)new MockConfiguration(name);
                                    pLink->setPreferred(false);
                                    break;
#endif
                            }
                            if(pLink) {
                                // Have the instance load its own values
                                pLink->loadSettings(settings, root);
                                addLinkConfiguration(pLink);
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
        emit linkConfigurationChanged();
    }
    
    // Debug buids always add MockLink automatically
#ifdef QT_DEBUG
    MockConfiguration* pMock = new MockConfiguration("Mock Link");
    pMock->setDynamic(true);
    addLinkConfiguration(pMock);
    emit linkConfigurationChanged();
#endif
    
    // Enable automatic PX4 hunting
    _configurationsLoaded = true;
}

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

void LinkManager::_updateConfigurationList(void)
{
    if (_configUpdateSuspended || !_configurationsLoaded) {
        return;
    }
    bool saveList = false;
    QStringList currentPorts;
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    // Iterate Comm Ports
    foreach (QSerialPortInfo portInfo, portList) {
#if 0
        qDebug() << "-----------------------------------------------------";
        qDebug() << "portName:         " << portInfo.portName();
        qDebug() << "systemLocation:   " << portInfo.systemLocation();
        qDebug() << "description:      " << portInfo.description();
        qDebug() << "manufacturer:     " << portInfo.manufacturer();
        qDebug() << "serialNumber:     " << portInfo.serialNumber();
        qDebug() << "vendorIdentifier: " << portInfo.vendorIdentifier();
#endif
        // Save port name
        currentPorts << portInfo.systemLocation();
        // Is this a PX4?
        if (portInfo.vendorIdentifier() == 9900) {
            SerialConfiguration* pSerial = _findSerialConfiguration(portInfo.systemLocation());
            if (pSerial) {
                //-- If this port is configured make sure it has the preferred flag set
                if(!pSerial->isPreferred()) {
                    pSerial->setPreferred(true);
                    saveList = true;
                }
            } else {
                // Lets create a new Serial configuration automatically
                pSerial = new SerialConfiguration(QString("Pixhawk on %1").arg(portInfo.portName().trimmed()));
                pSerial->setDynamic(true);
                pSerial->setPreferred(true);
                pSerial->setBaud(115200);
                pSerial->setPortName(portInfo.systemLocation());
                addLinkConfiguration(pSerial);
                saveList = true;
            }
        }
        // Is this an FTDI Chip? It could be a 3DR Modem
        if (portInfo.vendorIdentifier() == 1027) {
            SerialConfiguration* pSerial = _findSerialConfiguration(portInfo.systemLocation());
            if (pSerial) {
                //-- If this port is configured make sure it has the preferred flag set, unless someone else already has it set.
                if(!pSerial->isPreferred() && !saveList) {
                    pSerial->setPreferred(true);
                    saveList = true;
                }
            } else {
                // Lets create a new Serial configuration automatically (an assumption at best)
                pSerial = new SerialConfiguration(QString("3DR Radio on %1").arg(portInfo.portName().trimmed()));
                pSerial->setDynamic(true);
                pSerial->setPreferred(true);
                pSerial->setBaud(57600);
                pSerial->setPortName(portInfo.systemLocation());
                addLinkConfiguration(pSerial);
                saveList = true;
            }
        }
    }
    // Now we go through the current configuration list and make sure any dynamic config has gone away
    QList<LinkConfiguration*>  _confToDelete;
    foreach (LinkConfiguration* pLink, _linkConfigurations) {
        Q_ASSERT(pLink != NULL);
        // We only care about dynamic links
        if(pLink->isDynamic()) {
            if(pLink->type() == LinkConfiguration::TypeSerial) {
                SerialConfiguration* pSerial = dynamic_cast<SerialConfiguration*>(pLink);
                if(!currentPorts.contains(pSerial->portName())) {
                    _confToDelete.append(pSerial);
                }
            }
        }
    }
    // Now remove all links that are gone
    foreach (LinkConfiguration* pDelete, _confToDelete) {
        removeLinkConfiguration(pDelete);
        saveList = true;
    }
    // Save configuration list, which will also trigger a signal for the UI
    if(saveList) {
        saveLinkConfigurationList();
    }
}

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
        if (sharedLink.data()->isConnected()) {
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
    // This should never happen
    Q_ASSERT(false);
    return _nullSharedLink;
}
