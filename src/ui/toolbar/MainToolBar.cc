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

#include "MainWindow.h"
#include "MainToolBar.h"
#include "UASMessageHandler.h"
#include "UASMessageView.h"

MainToolBar::MainToolBar(QWidget* parent)
    : QGCQmlWidgetHolder(parent)
    , _mav(NULL)
    , _toolBar(NULL)
    , _currentView(ViewNone)
    , _batteryVoltage(0.0)
    , _batteryPercent(0.0)
    , _linkSelected(false)
    , _connectionCount(0)
    , _systemArmed(false)
    , _currentHeartbeatTimeout(0)
    , _waypointDistance(0.0)
    , _currentWaypoint(0)
    , _currentMessageCount(0)
    , _messageCount(0)
    , _currentErrorCount(0)
    , _currentWarningCount(0)
    , _currentNormalCount(0)
    , _currentMessageType(MessageNone)
    , _satelliteCount(-1)
    , _dotsPerInch(96.0)    // Default to Windows as it's more likely not to report below
    , _satelliteLock(0)
    , _showGPS(true)
    , _showMav(true)
    , _showMessages(true)
    , _showBattery(true)
    , _rollDownMessages(0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setObjectName("MainToolBar");
    setMinimumHeight(40);
    setMaximumHeight(40);
    setMinimumWidth(MainWindow::instance()->minimumWidth());
    // Get rid of layout default margins
    QLayout* pl = layout();
    if(pl) {
        pl->setContentsMargins(0,0,0,0);
    }
    // Get screen DPI to manage font sizes on different platforms
    QScreen *srn = QGuiApplication::screens().at(0); // TODO: Find current monitor as opposed to picking first one
    if(srn && srn->logicalDotsPerInch() > 50.0) {
        _dotsPerInch = (qreal)srn->logicalDotsPerInch(); // Font point sizes are based on Mac 72dpi
    } else {
        qWarning() << "System not reporting logical DPI, which is used to compute the appropriate font size. The default being used is 96dpi. If the text within buttons and UI elements are too big or too small, that's the reason.";
    }

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
    // Configure the toolbar for the current default UAS (which should be none as we just booted)
    _setActiveUAS(UASManager::instance()->getActiveUAS());
    emit configListChanged();
    emit heartbeatTimeoutChanged(_currentHeartbeatTimeout);
    emit connectionCountChanged(_connectionCount);
    // Link signals
    connect(UASManager::instance(),  &UASManager::activeUASSet,              this, &MainToolBar::_setActiveUAS);
    connect(LinkManager::instance(), &LinkManager::linkConfigurationChanged, this, &MainToolBar::_updateConfigurations);
    connect(LinkManager::instance(), &LinkManager::linkConnected,            this, &MainToolBar::_linkConnected);
    connect(LinkManager::instance(), &LinkManager::linkDisconnected,         this, &MainToolBar::_linkDisconnected);
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
    }
}

void MainToolBar::viewStateChanged(const QString &key, bool value)
{
    _setToolBarState(key, value);
}

void MainToolBar::onSetupView()
{
    setCurrentView(ViewSetup);
    MainWindow::instance()->loadSetupView();
}

void MainToolBar::onPlanView()
{
    setCurrentView(ViewPlan);
    MainWindow::instance()->loadOperatorView();
}

void MainToolBar::onFlyView()
{
    setCurrentView(ViewFly);
    MainWindow::instance()->loadPilotView();
}

void MainToolBar::onAnalyzeView()
{
    setCurrentView(ViewAnalyze);
    MainWindow::instance()->loadEngineerView();
}

void MainToolBar::onConnect(QString conf)
{
    // If no connection, the role is "Connect"
    if(_connectionCount == 0) {
        // Connect Link
        if(_currentConfig.isEmpty()) {
            MainWindow::instance()->manageLinks();
        } else {
            // We don't want the combo box updating under our feet
            LinkManager::instance()->suspendConfigurationUpdates(true);
            // Create a link
            LinkInterface* link = LinkManager::instance()->createLink(_currentConfig);
            if(link) {
                // Connect it
                LinkManager::instance()->connectLink(link);
                // Save last used connection
                MainWindow::instance()->saveLastUsedConnection(_currentConfig);
            }
            LinkManager::instance()->suspendConfigurationUpdates(false);
        }
    } else {
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
}

void MainToolBar::onLinkConfigurationChanged(const QString& config)
{
    // User selected a link configuration from the combobox
    if(_currentConfig != config) {
        _currentConfig = config;
        _linkSelected = true;
    }
}

void MainToolBar::onEnterMessageArea(int x, int y)
{
    // If not already there and messages are actually present
    if(!_rollDownMessages && UASMessageHandler::instance()->messages().count())
    {
        // Reset Counts
        int count = _currentMessageCount;
        MessageType_t type = _currentMessageType;
        _currentErrorCount   = 0;
        _currentWarningCount = 0;
        _currentNormalCount  = 0;
        _currentMessageCount = 0;
        _currentMessageType = MessageNone;
        if(count != _currentMessageCount) {
            emit newMessageCountChanged(0);
        }
        if(type != _currentMessageType) {
            emit messageTypeChanged(MessageNone);
        }
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

QString MainToolBar::getMavIconColor()
{
    // TODO: Not using because not only the colors are ghastly, it doesn't respect dark/light palette
    if(_mav)
        return _mav->getColor().name();
    else
        return QString("black");
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
        case MainWindow::VIEW_ENGINEER:
            view = ViewAnalyze;
            break;
        case MainWindow::VIEW_MISSION:
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

void MainToolBar::_setActiveUAS(UASInterface* active)
{
    // Do nothing if system is the same
    if (_mav == active) {
        return;
    }
    // If switching the UAS, disconnect the existing one.
    if (_mav)
    {
        disconnect(UASMessageHandler::instance(), &UASMessageHandler::textMessageCountChanged,  this, &MainToolBar::_handleTextMessage);
        disconnect(_mav, &UASInterface::heartbeatTimeout,                                       this, &MainToolBar::_heartbeatTimeout);
        disconnect(_mav, &UASInterface::batteryChanged,                                         this, &MainToolBar::_updateBatteryRemaining);
        disconnect(_mav, &UASInterface::modeChanged,                                            this, &MainToolBar::_updateMode);
        disconnect(_mav, &UASInterface::nameChanged,                                            this, &MainToolBar::_updateName);
        disconnect(_mav, &UASInterface::systemTypeSet,                                          this, &MainToolBar::_setSystemType);
        disconnect(_mav, &UASInterface::localizationChanged,                                    this, &MainToolBar::_setSatLoc);
        disconnect(_mav, SIGNAL(statusChanged(UASInterface*,QString,QString)),                  this, SLOT(_updateState(UASInterface*,QString,QString)));
        disconnect(_mav, SIGNAL(armingChanged(bool)),                                           this, SLOT(_updateArmingState(bool)));
        if (_mav->getWaypointManager())
        {
            disconnect(_mav->getWaypointManager(), &UASWaypointManager::currentWaypointChanged,  this, &MainToolBar::_updateCurrentWaypoint);
            disconnect(_mav->getWaypointManager(), &UASWaypointManager::waypointDistanceChanged, this, &MainToolBar::_updateWaypointDistance);
        }
        UAS* pUas = dynamic_cast<UAS*>(_mav);
        if(pUas) {
            disconnect(pUas, &UAS::satelliteCountChanged, this, &MainToolBar::_setSatelliteCount);
        }
    }
    // Connect new system
    _mav = active;
    if (_mav)
    {
        _setSystemType(_mav, _mav->getSystemType());
        _updateArmingState(_mav->isArmed());
        connect(UASMessageHandler::instance(), &UASMessageHandler::textMessageCountChanged, this, &MainToolBar::_handleTextMessage);
        connect(_mav, &UASInterface::heartbeatTimeout,                                      this, &MainToolBar::_heartbeatTimeout);
        connect(_mav, &UASInterface::batteryChanged,                                        this, &MainToolBar::_updateBatteryRemaining);
        connect(_mav, &UASInterface::modeChanged,                                           this, &MainToolBar::_updateMode);
        connect(_mav, &UASInterface::nameChanged,                                           this, &MainToolBar::_updateName);
        connect(_mav, &UASInterface::systemTypeSet,                                         this, &MainToolBar::_setSystemType);
        connect(_mav, &UASInterface::localizationChanged,                                   this, &MainToolBar::_setSatLoc);
        connect(_mav, SIGNAL(statusChanged(UASInterface*,QString,QString)),                 this, SLOT(_updateState(UASInterface*, QString,QString)));
        connect(_mav, SIGNAL(armingChanged(bool)),                                          this, SLOT(_updateArmingState(bool)));
        if (_mav->getWaypointManager())
        {
            connect(_mav->getWaypointManager(), &UASWaypointManager::currentWaypointChanged,  this, &MainToolBar::_updateCurrentWaypoint);
            connect(_mav->getWaypointManager(), &UASWaypointManager::waypointDistanceChanged, this, &MainToolBar::_updateWaypointDistance);
        }
        UAS* pUas = dynamic_cast<UAS*>(_mav);
        if(pUas) {
            _setSatelliteCount(pUas->getSatelliteCount(), QString(""));
            connect(pUas, &UAS::satelliteCountChanged, this, &MainToolBar::_setSatelliteCount);
        }
    }
    // Let toolbar know about it
    emit mavPresentChanged(_mav != NULL);
}

void MainToolBar::_updateArmingState(bool armed)
{
    if(_systemArmed != armed) {
        _systemArmed = armed;
        emit systemArmedChanged(armed);
    }
}

void MainToolBar::_updateBatteryRemaining(UASInterface*, double voltage, double, double percent, int)
{

    if(percent < 0.0) {
        percent = 0.0;
    }
    if(voltage < 0.0) {
        voltage = 0.0;
    }
    if (_batteryVoltage != voltage) {
        _batteryVoltage = voltage;
        emit batteryVoltageChanged(voltage);
    }
    if (_batteryPercent != percent) {
        _batteryPercent = percent;
        emit batteryPercentChanged(voltage);
    }
}

void MainToolBar::_updateConfigurations()
{
    bool resetSelected = false;
    QString selected = _currentConfig;
    QStringList tmpList;
    QList<LinkConfiguration*> configs = LinkManager::instance()->getLinkConfigurationList();
    foreach(LinkConfiguration* conf, configs) {
        if(conf) {
            tmpList << conf->name();
            if((!_linkSelected && conf->isPreferred()) || selected.isEmpty()) {
                selected = conf->name();
                resetSelected = true;
            }
        }
    }
    // Any changes?
    if(tmpList != _linkConfigurations) {
        _linkConfigurations = tmpList;
        emit configListChanged();
    }
    // Selection change?
    if((selected != _currentConfig && _linkConfigurations.contains(selected)) ||
       (selected.isEmpty())) {
        _currentConfig = selected;
        emit currentConfigChanged(_currentConfig);
    }
    if(resetSelected) {
        _linkSelected = false;
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
}

void MainToolBar::_updateState(UASInterface*, QString name, QString)
{
    if (_currentState != name) {
        _currentState = name;
        emit currentStateChanged(_currentState);
    }
}

void MainToolBar::_updateMode(int, QString name, QString)
{
    if (name.size()) {
        QString shortMode = name;
        shortMode = shortMode.replace("D|", "");
        shortMode = shortMode.replace("A|", "");
        if (_currentMode != shortMode) {
            _currentMode = shortMode;
            emit currentModeChanged();
        }
    }
}

void MainToolBar::_updateName(const QString& name)
{
    if (_systemName != name) {
        _systemName = name;
        // TODO: emit signal and use it
    }
}

/**
 * The current system type is represented through the system icon.
 *
 * @param uas Source system, has to be the same as this->uas
 * @param systemType type ID, following the MAVLink system type conventions
 * @see http://pixhawk.ethz.ch/software/mavlink
 */
void MainToolBar::_setSystemType(UASInterface*, unsigned int systemType)
{
    _systemPixmap = "qrc:/files/images/mavs/";
    switch (systemType) {
        case MAV_TYPE_GENERIC:
            _systemPixmap += "generic.svg";
            break;
        case MAV_TYPE_FIXED_WING:
            _systemPixmap += "fixed-wing.svg";
            break;
        case MAV_TYPE_QUADROTOR:
            _systemPixmap += "quadrotor.svg";
            break;
        case MAV_TYPE_COAXIAL:
            _systemPixmap += "coaxial.svg";
            break;
        case MAV_TYPE_HELICOPTER:
            _systemPixmap += "helicopter.svg";
            break;
        case MAV_TYPE_ANTENNA_TRACKER:
            _systemPixmap += "antenna-tracker.svg";
            break;
        case MAV_TYPE_GCS:
            _systemPixmap += "groundstation.svg";
            break;
        case MAV_TYPE_AIRSHIP:
            _systemPixmap += "airship.svg";
            break;
        case MAV_TYPE_FREE_BALLOON:
            _systemPixmap += "free-balloon.svg";
            break;
        case MAV_TYPE_ROCKET:
            _systemPixmap += "rocket.svg";
            break;
        case MAV_TYPE_GROUND_ROVER:
            _systemPixmap += "ground-rover.svg";
            break;
        case MAV_TYPE_SURFACE_BOAT:
            _systemPixmap += "surface-boat.svg";
            break;
        case MAV_TYPE_SUBMARINE:
            _systemPixmap += "submarine.svg";
            break;
        case MAV_TYPE_HEXAROTOR:
            _systemPixmap += "hexarotor.svg";
            break;
        case MAV_TYPE_OCTOROTOR:
            _systemPixmap += "octorotor.svg";
            break;
        case MAV_TYPE_TRICOPTER:
            _systemPixmap += "tricopter.svg";
            break;
        case MAV_TYPE_FLAPPING_WING:
            _systemPixmap += "flapping-wing.svg";
            break;
        case MAV_TYPE_KITE:
            _systemPixmap += "kite.svg";
            break;
        default:
            _systemPixmap += "unknown.svg";
            break;
    }
    emit systemPixmapChanged(_systemPixmap);
}

void MainToolBar::_heartbeatTimeout(bool timeout, unsigned int ms)
{
    unsigned int elapsed = ms;
    if (!timeout)
    {
        elapsed = 0;
    }
    if(elapsed != _currentHeartbeatTimeout) {
        _currentHeartbeatTimeout = elapsed;
        emit heartbeatTimeoutChanged(_currentHeartbeatTimeout);
    }
}

void MainToolBar::_handleTextMessage(int newCount)
{
    // Reset?
    if(!newCount) {
        _currentMessageCount = 0;
        _currentNormalCount  = 0;
        _currentWarningCount = 0;
        _currentErrorCount   = 0;
        _messageCount        = 0;
        _currentMessageType  = MessageNone;
        emit newMessageCountChanged(0);
        emit messageTypeChanged(MessageNone);
        emit messageCountChanged(0);
        return;
    }

    UASMessageHandler* pMh = UASMessageHandler::instance();
    Q_ASSERT(pMh);
    MessageType_t type = newCount ? _currentMessageType : MessageNone;
    int errorCount     = _currentErrorCount;
    int warnCount      = _currentWarningCount;
    int normalCount    = _currentNormalCount;
    //-- Add current message counts
    errorCount  += pMh->getErrorCount();
    warnCount   += pMh->getWarningCount();
    normalCount += pMh->getNormalCount();
    //-- See if we have a higher level
    if(errorCount != _currentErrorCount) {
        _currentErrorCount = errorCount;
        type = MessageError;
    }
    if(warnCount != _currentWarningCount) {
        _currentWarningCount = warnCount;
        if(_currentMessageType != MessageError) {
            type = MessageWarning;
        }
    }
    if(normalCount != _currentNormalCount) {
        _currentNormalCount = normalCount;
        if(_currentMessageType != MessageError && _currentMessageType != MessageWarning) {
            type = MessageNormal;
        }
    }
    int count = _currentErrorCount + _currentWarningCount + _currentNormalCount;
    if(count != _currentMessageCount) {
        _currentMessageCount = count;
        // Display current total new messages count
        emit newMessageCountChanged(count);
    }
    if(type != _currentMessageType) {
        _currentMessageType = type;
        // Update message level
        emit messageTypeChanged(type);
    }
    // Update message count (all messages)
    if(newCount != _messageCount) {
        _messageCount = newCount;
        emit messageCountChanged(_messageCount);
    }
}

void MainToolBar::_updateWaypointDistance(double distance)
{
    if (_waypointDistance != distance) {
        _waypointDistance = distance;
        // TODO: emit signal and use it
    }
}

void MainToolBar::_updateCurrentWaypoint(quint16 id)
{
    if (_currentWaypoint != id) {
        _currentWaypoint = id;
        // TODO: emit signal and use it
    }
}

void MainToolBar::_setSatelliteCount(double val, QString)
{
    // I'm assuming that a negative value or over 99 means there is no GPS
    if(val < 0.0)  val = -1.0;
    if(val > 99.0) val = -1.0;
    if(_satelliteCount != (int)val) {
        _satelliteCount = (int)val;
        emit satelliteCountChanged(_satelliteCount);
    }
}

void MainToolBar::_setSatLoc(UASInterface*, int fix)
{
    // fix 0: lost, 1: at least one satellite, but no GPS fix, 2: 2D lock, 3: 3D lock
    if(_satelliteLock != fix) {
        _satelliteLock = fix;
        emit satelliteLockChanged(_satelliteLock);
    }
}

void MainToolBar::updateCanvas()
{
    emit repaintRequestedChanged();
}
