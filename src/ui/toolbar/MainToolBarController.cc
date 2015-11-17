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
    , _progressBarValue(0.0f)
    , _remoteRSSI(0)
    , _remoteRSSIstore(100.0)
    , _telemetryRRSSI(0)
    , _telemetryLRSSI(0)
    , _rollDownMessages(0)
    , _toolbarMessageVisible(false)
{
    _activeVehicleChanged(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle());
    
    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged, this, &MainToolBarController::_activeVehicleChanged);
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

void MainToolBarController::onEnterMessageArea(int x, int y)
{
    Q_UNUSED(x);
    Q_UNUSED(y);

    // If not already there and messages are actually present
    if(!_rollDownMessages && qgcApp()->toolbox()->uasMessageHandler()->messages().count()) {
        if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
            qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->resetMessages();
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
        _rollDownMessages = new UASMessageViewRollDown(qgcApp()->toolbox()->uasMessageHandler(), MainWindow::instance());
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
        disconnect(_vehicle->autopilotPlugin(), &AutoPilotPlugin::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
        _mav = NULL;
        _vehicle = NULL;
    }
    
    // Connect new system
    if (vehicle)
    {
        _vehicle = vehicle;
        _mav = vehicle->uas();
        connect(_vehicle->autopilotPlugin(), &AutoPilotPlugin::parameterListProgress, this, &MainToolBarController::_setProgressBarValue);
    }
}

#if 0
// FIXME: Huh?
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
#endif

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

void MainToolBarController::showSettings(void)
{
    MainWindow::instance()->showSettings();
}

void MainToolBarController::manageLinks(void)
{
    MainWindow::instance()->manageLinks();
}
