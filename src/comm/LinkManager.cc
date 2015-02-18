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
#include <QSerialPortInfo>

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

LinkInterface* LinkManager::createLink(LinkConfiguration* config)
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
#ifdef UNITTEST_BUILD
        case LinkConfiguration::TypeMock:
            pLink = new MockLink(dynamic_cast<MockConfiguration*>(config));
            break;
#endif
    }
    if(pLink) {
        addLink(pLink);
    }
    return pLink;
}

LinkInterface* LinkManager::createLink(const QString& name)
{
    Q_ASSERT(name.isEmpty() == false);
    for(int i = 0; i < _linkConfigurations.count(); i++) {
        LinkConfiguration* conf = _linkConfigurations.at(i);
        if(conf && conf->name() == name)
            return createLink(conf);
    }
    return NULL;
}

void LinkManager::addLink(LinkInterface* link)
{
    Q_ASSERT(link);

    // Take ownership for delete
    link->_ownedByLinkManager = true;

    _linkListMutex.lock();

    if (!_links.contains(link)) {
        _links.append(link);
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

    _linkListMutex.lock();
    foreach (LinkInterface* link, _links) {
        Q_ASSERT(link);
        if (!link->_connect()) {
            allConnected = false;
        }
    }
    _linkListMutex.unlock();

    return allConnected;
}

bool LinkManager::disconnectAll()
{
    bool allDisconnected = true;

    _linkListMutex.lock();
    foreach (LinkInterface* link, _links)
    {
        Q_ASSERT(link);
        if (!link->_disconnect()) {
            allDisconnected = false;
        }
    }
    _linkListMutex.unlock();

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
        // TODO There is no point in disconnecting it if will stay around.
        // This should be turned into a delete link instead. Deleting a link
        // is not yet possible as LinkManager is broken.
        // Disconnect this link from its configuration
        LinkConfiguration* config = link->getLinkConfiguration();
        if(config) {
            config->setLink(NULL);
        }
        return true;
    } else {
        return false;
    }
}

void LinkManager::deleteLink(LinkInterface* link)
{
    Q_ASSERT(link);

    _linkListMutex.lock();

    Q_ASSERT(_links.contains(link));
    _links.removeOne(link);
    Q_ASSERT(!_links.contains(link));

    _linkListMutex.unlock();

    // Emit removal of link
    emit linkDeleted(link);

    Q_ASSERT(link->_ownedByLinkManager);
    link->_deletedByLinkManager = true;   // Signal that this is a valid delete
    delete link;
}

/**
 *
 */
const QList<LinkInterface*> LinkManager::getLinks()
{
    _linkListMutex.lock();
    QList<LinkInterface*> ret(_links);
    _linkListMutex.unlock();
    return ret;
}

const QList<SerialLink *> LinkManager::getSerialLinks()
{
    _linkListMutex.lock();
    QList<SerialLink*> s;

    foreach (LinkInterface* link, _links)
    {
        Q_ASSERT(link);

        SerialLink* serialLink = qobject_cast<SerialLink*>(link);

        if (serialLink)
            s.append(serialLink);
    }
    _linkListMutex.unlock();

    return s;
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
    QList<LinkInterface*> links = _links;
    foreach(LinkInterface* link, links) {
        disconnectLink(link);
        deleteLink(link);
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
    QString root(LinkConfiguration::settingsRoot());
    settings.setValue(root + "/count", _linkConfigurations.count());
    int index = 0;
    foreach (LinkConfiguration* pLink, _linkConfigurations) {
        Q_ASSERT(pLink != NULL);
        root = LinkConfiguration::settingsRoot();
        root += QString("/Link%1").arg(index++);
        settings.setValue(root + "/name", pLink->name());
        settings.setValue(root + "/type", pLink->type());
        settings.setValue(root + "/preferred", pLink->isPreferred());
        // Have the instance save its own values
        pLink->saveSettings(settings, root);
    }
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
#ifdef UNITTEST_BUILD
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
        // Is this a PX4?
        if (portInfo.vendorIdentifier() == 9900) {
            SerialConfiguration* pSerial = _findSerialConfiguration(portInfo.portName());
            if (pSerial) {
                //-- If this port is configured make sure it has the preferred flag set
                if(!pSerial->isPreferred()) {
                    pSerial->setPreferred(true);
                    saveList = true;
                }
            } else {
                // Lets create a new Serial configuration automatically
                pSerial = new SerialConfiguration(QString("Pixhawk on %1").arg(portInfo.portName().trimmed()));
                pSerial->setPreferred(true);
                pSerial->setBaud(115200);
                pSerial->setPortName(portInfo.portName());
                addLinkConfiguration(pSerial);
                saveList = true;
            }
        }
    }
    // Save configuration list, which will also trigger a signal for the UI
    if(saveList) {
        saveLinkConfigurationList();
    }
}

