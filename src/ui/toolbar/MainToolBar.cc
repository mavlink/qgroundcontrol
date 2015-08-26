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

#include "MainToolBar.h"
#include "ScreenToolsController.h"
#include "MainWindow.h"
#include "UASMessageView.h"
#include "UASMessageHandler.h"
#include "FlightDisplay.h"
#include "QGCApplication.h"
#include "MavManager.h"
#include "AutoPilotPluginManager.h"

MainToolBar::MainToolBar(QWidget* parent)
    : QGCQmlWidgetHolder(parent)
    , _mav(NULL)
    , _toolBar(NULL)
    , _currentView(ViewNone)
    , _connectionCount(0)
    , _showGPS(true)
    , _showMav(true)
    , _showMessages(true)
    , _showRSSI(true)
    , _showBattery(true)
    , _progressBarValue(0.0f)
    , _remoteRSSI(0)
    , _remoteRSSIstore(100.0)
    , _telemetryRRSSI(0)
    , _telemetryLRSSI(0)
    , _rollDownMessages(0)
    , _toolbarMessageVisible(false)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setObjectName("MainToolBar");
    setMinimumWidth(MainWindow::instance()->minimumWidth());
    // Get rid of layout default margins
    QLayout* pl = layout();
    if(pl) {
        pl->setContentsMargins(0,0,0,0);
    }
    setMinimumHeight(ScreenToolsController::defaultFontPixelSize_s() * 3);
    setMaximumHeight(ScreenToolsController::defaultFontPixelSize_s() * 3);
    // Tool Bar Preferences
    QSettings settings;
    settings.beginGroup(TOOL_BAR_SETTINGS_GROUP);
    _showBattery  = settings.value(TOOL_BAR_SHOW_BATTERY,  true).toBool();
    _showGPS      = settings.value(TOOL_BAR_SHOW_GPS,      true).toBool();
    _showMav      = settings.value(TOOL_BAR_SHOW_MAV,      true).toBool();
    _showMessages = settings.value(TOOL_BAR_SHOW_MESSAGES, true).toBool();
    settings.endGroup();

    setContextPropertyObject("mainToolBar", this);
    setSource(QUrl::fromUserInput("qrc:/qml/MainToolBar.qml"));
    setVisible(true);
    emit configListChanged();
    emit connectionCountChanged(_connectionCount);
    _setActiveUAS(UASManager::instance()->getActiveUAS());
    // Link signals
    connect(LinkManager::instance(),     &LinkManager::linkConfigurationChanged, this, &MainToolBar::_updateConfigurations);
    connect(LinkManager::instance(),     &LinkManager::linkConnected,            this, &MainToolBar::_linkConnected);
    connect(LinkManager::instance(),     &LinkManager::linkDisconnected,         this, &MainToolBar::_linkDisconnected);
    // RSSI (didn't like standard connection)
    connect(MAVLinkProtocol::instance(),
        SIGNAL(radioStatusChanged(LinkInterface*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned)), this,
        SLOT(_telemetryChanged(LinkInterface*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned)));
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(_setActiveUAS(UASInterface*)));
    connect(UASManager::instance(), SIGNAL(UASDeleted(UASInterface*)),   this, SLOT(_forgetUAS(UASInterface*)));
    
    connect(this, &MainToolBar::heightChanged, this, &MainToolBar::_heightChanged);
}

MainToolBar::~MainToolBar()
{

}

void MainToolBar::_setToolBarState(const QString& key, bool value)
{
    QSettings settings;
    settings.beginGroup(TOOL_BAR_SETTINGS_GROUP);
    settings.setValue(key, value);
    settings.endGroup();
    if(key == TOOL_BAR_SHOW_GPS) {
        _showGPS = value;
        emit showGPSChanged(value);
    } else if(key == TOOL_BAR_SHOW_MAV) {
        _showMav = value;
        emit showMavChanged(value);
    }else if(key == TOOL_BAR_SHOW_BATTERY) {
        _showBattery = value;
        emit showBatteryChanged(value);
    } else if(key == TOOL_BAR_SHOW_MESSAGES) {
        _showMessages = value;
        emit showMessagesChanged(value);
    } else if(key == TOOL_BAR_SHOW_RSSI) {
        _showRSSI = value;
        emit showRSSIChanged(value);
    }
}

void MainToolBar::viewStateChanged(const QString &key, bool value)
{
    _setToolBarState(key, value);
}

void MainToolBar::onSetupView()
{
    setCurrentView(MainWindow::VIEW_SETUP);
    MainWindow::instance()->loadSetupView();
}

void MainToolBar::onPlanView()
{
    setCurrentView(MainWindow::VIEW_PLAN);
    MainWindow::instance()->loadPlanView();
}

void MainToolBar::onFlyView()
{
    setCurrentView(MainWindow::VIEW_FLIGHT);
    MainWindow::instance()->loadFlightView();
}

void MainToolBar::onFlyViewMenu()
{
    FlightDisplay* fdsp = MainWindow::instance()->getFlightDisplay();
    if(fdsp) {
        fdsp->showOptionsMenu();
    }
}

void MainToolBar::onAnalyzeView()
{
    setCurrentView(MainWindow::VIEW_ANALYZE);
    MainWindow::instance()->loadAnalyzeView();
}

void MainToolBar::onDisconnect(QString conf)
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

void MainToolBar::onConnect(QString conf)
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

void MainToolBar::onEnterMessageArea(int x, int y)
{
    // If not already there and messages are actually present
    if(!_rollDownMessages && UASMessageHandler::instance()->messages().count())
    {
        if(qgcApp()->getMavManager())
            qgcApp()->getMavManager()->resetMessages();
        // Show messages
        int dialogWidth = 400;
        x = x - (dialogWidth >> 1);
        if(x < 0) x = 0;
        y = height() / 3;
        // Put dialog on top of the message alert icon
        QPoint p = mapToGlobal(QPoint(x,y));
        _rollDownMessages = new UASMessageViewRollDown(MainWindow::instance());
        _rollDownMessages->setAttribute(Qt::WA_DeleteOnClose);
        _rollDownMessages->move(mapFromGlobal(p));
        _rollDownMessages->setMinimumSize(dialogWidth,200);
        connect(_rollDownMessages, &UASMessageViewRollDown::closeWindow, this, &MainToolBar::_leaveMessageView);
        _rollDownMessages->show();
    }
}

void MainToolBar::_leaveMessageView()
{
    // Mouse has left the message window area (and it has closed itself)
    _rollDownMessages = NULL;
}

void MainToolBar::setCurrentView(int currentView)
{
    ViewType_t view = ViewNone;
    switch((MainWindow::VIEW_SECTIONS)currentView) {
        case MainWindow::VIEW_ANALYZE:
            view = ViewAnalyze;
            break;
        case MainWindow::VIEW_PLAN:
            view = ViewPlan;
            break;
           case MainWindow::VIEW_FLIGHT:
            view = ViewFly;
            break;
        case MainWindow::VIEW_SETUP:
            view = ViewSetup;
            break;
        default:
            view = ViewNone;
            break;
    }
    if(view != _currentView) {
        _currentView = view;
        emit currentViewChanged();
    }
}

void MainToolBar::_forgetUAS(UASInterface* uas)
{
    if (_mav != NULL && _mav == uas) {
        disconnect(_mav, &UASInterface::remoteControlRSSIChanged, this, &MainToolBar::_remoteControlRSSIChanged);
        disconnect(AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_mav).data(), &AutoPilotPlugin::parameterListProgress, this, &MainToolBar::_setProgressBarValue);
        _mav = NULL;
    }
}
void MainToolBar::_setActiveUAS(UASInterface* active)
{
    // Do nothing if system is the same
    if (_mav == active) {
        return;
    }
    // Disconnect the previous one (if any)
    if(_mav) {
        _forgetUAS(_mav);
    }
    // Connect new system
    _mav = active;
    if (_mav)
    {
        connect(_mav, &UASInterface::remoteControlRSSIChanged, this, &MainToolBar::_remoteControlRSSIChanged);
        connect(AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(_mav).data(), &AutoPilotPlugin::parameterListProgress, this, &MainToolBar::_setProgressBarValue);
    }
}

void MainToolBar::_updateConfigurations()
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

void MainToolBar::_telemetryChanged(LinkInterface*, unsigned, unsigned, unsigned rssi, unsigned remrssi, unsigned, unsigned, unsigned)
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

void MainToolBar::_remoteControlRSSIChanged(uint8_t rssi)
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

void MainToolBar::_linkConnected(LinkInterface*)
{
    _updateConnection();
}

void MainToolBar::_linkDisconnected(LinkInterface* link)
{
    _updateConnection(link);
}

void MainToolBar::_updateConnection(LinkInterface *disconnectedLink)
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

void MainToolBar::_setProgressBarValue(float value)
{
    _progressBarValue = value;
    emit progressBarValueChanged(value);
}

void MainToolBar::_heightChanged(double height)
{
    setMinimumHeight(height);
    setMaximumHeight(height);
}

void MainToolBar::showToolBarMessage(const QString& message)
{
    _toolbarMessageQueueMutex.lock();
    
    if (_toolbarMessageQueue.count() == 0 && !_toolbarMessageVisible) {
        QTimer::singleShot(500, this, &MainToolBar::_delayedShowToolBarMessage);
    }
    
    _toolbarMessageQueue += message;
    
    _toolbarMessageQueueMutex.unlock();
}

void MainToolBar::_delayedShowToolBarMessage(void)
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

void MainToolBar::onToolBarMessageClosed(void)
{
    _toolbarMessageVisible = false;
    _delayedShowToolBarMessage();
}