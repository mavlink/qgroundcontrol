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
 *   @brief QGC Main Tool Bar
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include <QQmlContext>
#include <QQmlEngine>

#include "MainToolBarController.h"
#include "ScreenToolsController.h"
#include "MainWindow.h"
#include "UASMessageView.h"
#include "UASMessageHandler.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include "UAS.h"

MainToolBarController::MainToolBarController(QObject* parent)
    : QObject(parent)
    , _vehicle(NULL)
    , _mav(NULL)
    , _connectionCount(0)
    , _progressBarValue(0.0f)
    , _remoteRSSI(0)
    , _remoteRSSIstore(100.0)
    , _telemetryRRSSI(0)
    , _telemetryLRSSI(0)
    , _rollDownMessages(0)
    , _toolbarMessageVisible(false)
{
    emit configListChanged();
    emit connectionCountChanged(_connectionCount);
    _activeVehicleChanged(MultiVehicleManager::instance()->activeVehicle());
    
    // Link signals
    connect(LinkManager::instance(),     &LinkManager::linkConfigurationChanged, this, &MainToolBarController::_updateConfigurations);
    connect(LinkManager::instance(),     &LinkManager::linkConnected,            this, &MainToolBarController::_linkConnected);
    connect(LinkManager::instance(),     &LinkManager::linkDisconnected,         this, &MainToolBarController::_linkDisconnected);
    
    // RSSI (didn't like standard connection)
    connect(MAVLinkProtocol::instance(),
        SIGNAL(radioStatusChanged(LinkInterface*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned)), this,
        SLOT(_telemetryChanged(LinkInterface*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned)));
    
    connect(MultiVehicleManager::instance(), &MultiVehicleManager::activeVehicleChanged, this, &MainToolBarController::_activeVehicleChanged);
}

MainToolBarController::~MainToolBarController()
{

}

void MainToolBarController::onSetupView()
{
    MainWindow::instance()->showSetupView();
}

void MainToolBarController::onPlanView()
{
    MainWindow::instance()->showPlanView();
}

void MainToolBarController::onFlyView()
{
    MainWindow::instance()->showFlyView();
}

void MainToolBarController::onDisconnect(QString conf)
{
    if(conf.isEmpty()) {
        // Disconnect Only Connected Link
        int connectedCount = 0;
        LinkInterface* connectedLink = NULL;
        QList<LinkInterface*> links = LinkManager::instance()->getLinks();
        foreach(LinkInterface* link, links) {
            if (link->isConnected()) {
                connectedCount++;
                connectedLink = link;
            }
        }
        Q_ASSERT(connectedCount   == 1);
        Q_ASSERT(_connectionCount == 1);
        Q_ASSERT(connectedLink);
        LinkManager::instance()->disconnectLink(connectedLink);
    } else {
        // Disconnect Named Connected Link
        QList<LinkInterface*> links = LinkManager::instance()->getLinks();
        foreach(LinkInterface* link, links) {
            if (link->isConnected()) {
                if(link->getLinkConfiguration() && link->getLinkConfiguration()->name() == conf) {
                    LinkManager::instance()->disconnectLink(link);
                }
            }
        }
    }
}

void MainToolBarController::onConnect(QString conf)
{
    // Connect Link
    if(conf.isEmpty()) {
        MainWindow::instance()->manageLinks();
    } else {
        // We don't want the list updating under our feet
        LinkManager::instance()->suspendConfigurationUpdates(true);
        // Create a link
        LinkInterface* link = LinkManager::instance()->createConnectedLink(conf);
        if(link) {
            // Save last used connection
            MainWindow::instance()->saveLastUsedConnection(conf);
        }
        LinkManager::instance()->suspendConfigurationUpdates(false);
    }
}

void MainToolBarController::onEnterMessageArea(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);

    // If not already there and messages are actually present
    if(!_rollDownMessages && UASMessageHandler::instance()->messages().count()) {
        if (MultiVehicleManager::instance()->activeVehicle()) {
            MultiVehicleManager::instance()->activeVehicle()->resetMessages();
        }

        // FIXME: Position of the message dropdown is hacked right now to speed up Qml conversion
        // Show messages
        int dialogWidth = 400;
#if 0
        x = x - (dialogWidth >> 1);
        if(x < 0) x = 0;
        y = height() / 3;
#endif

        // Put dialog on top of the message alert icon
        _rollDownMessages = new UASMessageViewRollDown(MainWindow::instance());
        _rollDownMessages->setAttribute(Qt::WA_DeleteOnClose);
        _rollDownMessages->move(QPoint(100, 100));
        _rollDownMessages->setMinimumSize(dialogWidth,200);
        connect(_rollDownMessages, &UASMessageViewRollDown::closeWindow, this, &MainToolBarController::_leaveMessageView);
        _rollDownMessages->show();
    }
}

void MainToolBarController::_leaveMessageView()
{
    // Mouse has left the message window area (and it has closed itself)
    _rollDownMessages = NULL;
}

void MainToolBarController::_activeVehicleChanged(Vehicle* vehicle)
{
    // Disconnect the previous one (if any)
    if (_vehicle) {
        disconnect(_mav, &UASInterface::remoteControlRSSIChanged, this, &MainToolBarController::_remoteControlRSSIChanged);
        disconnect(_vehicle->autopilotPlugin(), &AutoPilotPlugin::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
        _mav = NULL;
        _vehicle = NULL;
    }
    
    // Connect new system
    if (vehicle)
    {
        _vehicle = vehicle;
        _mav = vehicle->uas();
        connect(_mav, &UASInterface::remoteControlRSSIChanged, this, &MainToolBarController::_remoteControlRSSIChanged);
        connect(_vehicle->autopilotPlugin(), &AutoPilotPlugin::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
    }
}

void MainToolBarController::_updateConfigurations()
{
    QStringList tmpList;
    QList<LinkConfiguration*> configs = LinkManager::instance()->getLinkConfigurationList();
    foreach(LinkConfiguration* conf, configs) {
        if(conf) {
            if(conf->isPreferred()) {
                tmpList.insert(0,conf->name());
            } else {
                tmpList << conf->name();
            }
        }
    }
    // Any changes?
    if(tmpList != _linkConfigurations) {
        _linkConfigurations = tmpList;
        emit configListChanged();
    }
}

void MainToolBarController::_telemetryChanged(LinkInterface*, unsigned, unsigned, unsigned rssi, unsigned remrssi, unsigned, unsigned, unsigned)
{
    // We only care if we haveone single connection
    if(_connectionCount == 1) {
        if((unsigned)_telemetryLRSSI != rssi) {
            // According to the Silabs data sheet, the RSSI value is 0.5db per bit
            _telemetryLRSSI = rssi >> 1;
            emit telemetryLRSSIChanged(_telemetryLRSSI);
        }
        if((unsigned)_telemetryRRSSI != remrssi) {
            // According to the Silabs data sheet, the RSSI value is 0.5db per bit
            _telemetryRRSSI = remrssi >> 1;
            emit telemetryRRSSIChanged(_telemetryRRSSI);
        }
    }
}

void MainToolBarController::_remoteControlRSSIChanged(uint8_t rssi)
{
    // We only care if we have one single connection
    if(_connectionCount == 1) {
        // Low pass to git rid of jitter
        _remoteRSSIstore = (_remoteRSSIstore * 0.9f) + ((float)rssi * 0.1);
        uint8_t filteredRSSI = (uint8_t)ceil(_remoteRSSIstore);
        if(_remoteRSSIstore < 0.1) {
            filteredRSSI = 0;
        }
        if(_remoteRSSI != filteredRSSI) {
            _remoteRSSI = filteredRSSI;
            emit remoteRSSIChanged(_remoteRSSI);
        }
    }
}

void MainToolBarController::_linkConnected(LinkInterface*)
{
    _updateConnection();
}

void MainToolBarController::_linkDisconnected(LinkInterface* link)
{
    _updateConnection(link);
}

void MainToolBarController::_updateConnection(LinkInterface *disconnectedLink)
{
    QStringList connList;
    int oldCount = _connectionCount;
    // If there are multiple connected links add/update the connect button menu
    _connectionCount = 0;
    QList<LinkInterface*> links = LinkManager::instance()->getLinks();
    foreach(LinkInterface* link, links) {
        if (disconnectedLink != link && link->isConnected()) {
            _connectionCount++;
            if(link->getLinkConfiguration()) {
                connList << link->getLinkConfiguration()->name();
            }
        }
    }
    if(oldCount != _connectionCount) {
        emit connectionCountChanged(_connectionCount);
    }
    if(connList != _connectedList) {
        _connectedList = connList;
        emit connectedListChanged(_connectedList);
    }
    // Update telemetry RSSI display
    if(_connectionCount != 1 && _telemetryRRSSI > 0) {
        _telemetryRRSSI = 0;
        emit telemetryRRSSIChanged(_telemetryRRSSI);
    }
    if(_connectionCount != 1 && _telemetryLRSSI > 0) {
        _telemetryLRSSI = 0;
        emit telemetryLRSSIChanged(_telemetryLRSSI);
    }
    if(_connectionCount != 1 && _remoteRSSI > 0) {
        _remoteRSSI = 0;
        emit remoteRSSIChanged(_remoteRSSI);
    }
}

void MainToolBarController::_setProgressBarValue(float value)
{
    _progressBarValue = value;
    emit progressBarValueChanged(value);
}

void MainToolBarController::showToolBarMessage(const QString& message)
{
    _toolbarMessageQueueMutex.lock();
    
    if (_toolbarMessageQueue.count() == 0 && !_toolbarMessageVisible) {
        QTimer::singleShot(500, this, &MainToolBarController::_delayedShowToolBarMessage);
    }
    
    _toolbarMessageQueue += message;
    
    _toolbarMessageQueueMutex.unlock();
}

void MainToolBarController::_delayedShowToolBarMessage(void)
{
    QString messages;
    
    if (!_toolbarMessageVisible) {
        _toolbarMessageQueueMutex.lock();
        
        foreach (QString message, _toolbarMessageQueue) {
            messages += message + "\n";
        }
        _toolbarMessageQueue.clear();
        
        _toolbarMessageQueueMutex.unlock();
        
        if (!messages.isEmpty()) {
            _toolbarMessageVisible = true;
            emit showMessage(messages);
        }
    }
}

void MainToolBarController::onToolBarMessageClosed(void)
{
    _toolbarMessageVisible = false;
    _delayedShowToolBarMessage();
}
