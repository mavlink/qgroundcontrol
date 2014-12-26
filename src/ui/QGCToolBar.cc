/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2011 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

#include <QToolButton>
#include <QLabel>
#include <QSpacerItem>
#include <QSerialPortInfo>

#include "SerialLink.h"
#include "UDPLink.h"
#include "QGCToolBar.h"
#include "UASManager.h"
#include "MainWindow.h"
#include "QGCApplication.h"
#include "CommConfigurationWindow.h"

QGCToolBar::QGCToolBar(QWidget *parent) :
    QToolBar(parent),
    mav(NULL),
    changed(true),
    batteryPercent(0),
    batteryVoltage(0),
    wpId(0),
    wpDistance(0),
    altitudeRel(0),
    systemArmed(false),
    currentLink(NULL),
    firstAction(NULL),
    _linkMgr(LinkManager::instance()),
    _linkCombo(NULL),
    _linkComboAction(NULL),
    _linkSelectedOnce(false),
    _baudCombo(NULL),
    _baudComboAction(NULL),
    _linksConnected(false)
{
    setObjectName("QGCToolBar");
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    connect(LinkManager::instance(), &LinkManager::linkConnected, this, &QGCToolBar::_linkConnected);
    connect(LinkManager::instance(), &LinkManager::linkDisconnected, this, &QGCToolBar::_linkDisconnected);
}

void QGCToolBar::heartbeatTimeout(bool timeout, unsigned int ms)
{
    // set timeout label visible
    if (timeout)
    {
        // Alternate colors to increase visibility
        if ((ms / 1000) % 2 == 0)
        {
            toolBarTimeoutLabel->setStyleSheet(QString("QLabel {color: #000; background-color: #FF0037;}"));
        }
        else
        {
            toolBarTimeoutLabel->setStyleSheet(QString("QLabel {color: #FFF; background-color: #6B0017;}"));
        }
        toolBarTimeoutLabel->setText(tr("CONNECTION LOST: %1 s").arg((ms / 1000.0f), 2, 'f', 1, ' '));
        toolBarTimeoutAction->setVisible(true);
        toolBarMessageAction->setVisible(false);
        toolBarBatteryBarAction->setVisible(false);
    }
    else
    {
        // Check if loss text is present, reset once
        if (toolBarTimeoutAction->isVisible())
        {
            toolBarTimeoutAction->setVisible(false);
            toolBarMessageAction->setVisible(true);
            toolBarBatteryBarAction->setVisible(true);
        }
    }
}

void QGCToolBar::createUI()
{
    // CREATE TOOLBAR ITEMS
    // Add internal actions
    // Add MAV widget
    symbolLabel = new QLabel(this);
    addWidget(symbolLabel);

    toolBarNameLabel = new QLabel(this);
    toolBarNameLabel->setToolTip(tr("Currently controlled vehicle"));
    toolBarNameLabel->setAlignment(Qt::AlignCenter);
    toolBarNameLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    addWidget(toolBarNameLabel);

    toolBarTimeoutLabel = new QLabel(this);
    toolBarTimeoutLabel->setToolTip(tr("System timed out, interval since last message"));
    toolBarTimeoutLabel->setAlignment(Qt::AlignCenter);
    toolBarTimeoutLabel->setObjectName("toolBarTimeoutLabel");
    toolBarTimeoutLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    toolBarTimeoutAction = addWidget(toolBarTimeoutLabel);

    toolBarSafetyLabel = new QLabel(this);
    toolBarSafetyLabel->setToolTip(tr("Vehicle safety state"));
    toolBarSafetyLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarSafetyLabel);

    toolBarModeLabel = new QLabel(this);
    toolBarModeLabel->setToolTip(tr("Vehicle mode"));
    toolBarModeLabel->setObjectName("toolBarModeLabel");
    toolBarModeLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarModeLabel);

//    toolBarStateLabel = new QLabel(this);
//    toolBarStateLabel->setToolTip(tr("Vehicle state"));
//    toolBarStateLabel->setObjectName("toolBarStateLabel");
//    toolBarStateLabel->setAlignment(Qt::AlignCenter);
//    addWidget(toolBarStateLabel);

    toolBarBatteryBar = new QProgressBar(this);
    toolBarBatteryBar->setMinimum(0);
    toolBarBatteryBar->setMaximum(100);
    toolBarBatteryBar->setMinimumWidth(20);
    toolBarBatteryBar->setMaximumWidth(100);
    toolBarBatteryBar->setToolTip(tr("Battery charge level"));
    toolBarBatteryBar->setObjectName("toolBarBatteryBar");
    toolBarBatteryBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    toolBarBatteryBarAction = addWidget(toolBarBatteryBar);

    toolBarBatteryVoltageLabel = new QLabel(this);
    toolBarBatteryVoltageLabel->setToolTip(tr("Battery voltage"));
    toolBarBatteryVoltageLabel->setObjectName("toolBarBatteryVoltageLabel");
    toolBarBatteryVoltageLabel->setAlignment(Qt::AlignCenter);
    toolBarBatteryVoltageAction = addWidget(toolBarBatteryVoltageLabel);

    toolBarWpLabel = new QLabel(this);
    toolBarWpLabel->setToolTip(tr("Current waypoint"));
    toolBarWpLabel->setObjectName("toolBarWpLabel");
    toolBarWpLabel->setAlignment(Qt::AlignCenter);
    toolBarWpAction = addWidget(toolBarWpLabel);

    toolBarMessageLabel = new QLabel(this);
    toolBarMessageLabel->setToolTip(tr("Most recent system message"));
    toolBarMessageLabel->setObjectName("toolBarMessageLabel");
    toolBarMessageAction = addWidget(toolBarMessageLabel);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    addWidget(spacer);

    _linkCombo = new QComboBox(this);
    _linkCombo->addItem("WiFi");
    connect(_linkCombo, SIGNAL(activated(int)), SLOT(_linkComboActivated(int)));
    
    _linkCombo->setToolTip(tr("Choose the link to use"));
    _linkCombo->setEnabled(true);
    _linkCombo->setMinimumWidth(100);
    
    _linkComboAction = addWidget(_linkCombo);

    _baudCombo = new QComboBox(this);
    _baudCombo->setToolTip(tr("Choose what baud rate to use"));
    _baudCombo->setEnabled(true);
    _baudCombo->setMinimumWidth(40);
    _baudCombo->addItem("9600", 9600);
    _baudCombo->addItem("14400", 14400);
    _baudCombo->addItem("19200", 19200);
    _baudCombo->addItem("38400", 38400);
    _baudCombo->addItem("57600", 57600);
    _baudCombo->addItem("115200", 115200);
    _baudCombo->addItem("230400", 230400);
    _baudCombo->addItem("460800", 460800);
    _baudCombo->addItem("921600", 921600);
    _baudCombo->setCurrentIndex(_baudCombo->findData(57600));
    _baudComboAction = addWidget(_baudCombo);

    _connectButton = new QPushButton(tr("Connect"), this);
    _connectButton->setObjectName("connectButton");
    addWidget(_connectButton);
    connect(_connectButton, &QPushButton::clicked, this, &QGCToolBar::_connectButtonClicked);

    resetToolbarUI();

    // DONE INITIALIZING BUTTONS

    // Set the toolbar to be updated every 2s
    connect(&updateViewTimer, SIGNAL(timeout()), this, SLOT(updateView()));

    // Configure the toolbar for the current default UAS
    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    connect(&_portListTimer, &QTimer::timeout, this, &QGCToolBar::_updatePortList);
    _portListTimer.start(500);

    toolBarMessageAction->setVisible(false);
    toolBarBatteryBarAction->setVisible(false);

    changed = false;
}

/**
 * Reset all the labels and stuff for the toolbar to a pristine state. Done at startup after
 * all UI has been created and also when the last UAS has been deleted.
 **/
void QGCToolBar::resetToolbarUI()
{
    toolBarNameLabel->setText("------");
    toolBarNameLabel->setStyleSheet("");
    toolBarTimeoutLabel->setText(tr("UNCONNECTED"));
    //toolBarTimeoutLabel->show();
    toolBarSafetyLabel->setText("----");
    toolBarModeLabel->setText("------");
//    toolBarStateLabel->setText("------");
    toolBarBatteryBar->setValue(0);
    toolBarBatteryBar->setDisabled(true);
    toolBarBatteryVoltageLabel->setText("xx.x V");
    toolBarWpLabel->setText("WP--");
    toolBarMessageLabel->clear();
    lastSystemMessage = "";
    lastSystemMessageTimeMs = 0;
    symbolLabel->setStyleSheet("");
    symbolLabel->clear();
    toolBarMessageAction->setVisible(false);
    toolBarBatteryBarAction->setVisible(false);
}

void QGCToolBar::setPerspectiveChangeActions(const QList<QAction*> &actions)
{
    if (actions.count() > 1)
    {
        group = new QButtonGroup(this);
        group->setExclusive(true);

        // Add the first button.
        QToolButton *first = new QToolButton(this);
        //first->setIcon(actions.first()->icon());
        first->setText(actions.first()->text());
        first->setToolTip(actions.first()->toolTip());
        first->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        first->setCheckable(true);

        connect(first, SIGNAL(clicked(bool)), actions.first(), SIGNAL(triggered(bool)));
        connect(actions.first(),SIGNAL(triggered(bool)),first,SLOT(setChecked(bool)));

        first->setObjectName("firstAction");

        //first->setStyleSheet("QToolButton { min-height: 24px; max-height: 24px; min-width: 60px; color: #222222; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #A2A3A4, stop: 1 #B6B7B8); margin-left: 8px; margin-right: 0px; padding-left: 4px; padding-right: 8px; border-radius: 0px; border : 0px solid blue; border-bottom-left-radius: 6px; border-top-left-radius: 6px; border-left: 1px solid #484848; border-top: 1px solid #484848; border-bottom: 1px solid #484848; } QToolButton:checked { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #555555, stop: 1 #787878); color: #DDDDDD; }");
        addWidget(first);
        group->addButton(first);

        // Add all the middle buttons.
        for (int i = 1; i < actions.count(); i++)
        {
            QToolButton *btn = new QToolButton(this);
            //btn->setIcon(actions.at(i)->icon());
            btn->setText(actions.at(i)->text());
            btn->setToolTip(actions.at(i)->toolTip());
            btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            btn->setCheckable(true);
            connect(btn, SIGNAL(clicked(bool)), actions.at(i), SIGNAL(triggered(bool)));
            connect(actions.at(i),SIGNAL(triggered(bool)),btn,SLOT(setChecked(bool)));
            addWidget(btn);
            group->addButton(btn);
        }

        // Add last button
        advancedButton = new QToolButton(this);
        advancedButton->setIcon(QIcon(":/files/images/apps/utilities-system-monitor.svg"));
        advancedButton->setText(tr("More"));
        advancedButton->setToolTip(tr("Options for advanced users"));
        advancedButton->setCheckable(true);
        advancedButton->setObjectName("advancedButton");
        advancedButton->setPopupMode(QToolButton::InstantPopup);
        advancedButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
        addWidget(advancedButton);
        group->addButton(advancedButton);
    } else {
        qDebug() << __FILE__ << __LINE__ << "Not enough perspective change actions provided";
    }

    // Add the "rest"
    createUI();
}

void QGCToolBar::setPerspectiveChangeAdvancedActions(const QList<QAction*> &actions)
{
    if (actions.count() > 1)
    {
        QMenu *menu = new QMenu(advancedButton);

        for (int i = 0; i < actions.count(); i++)
        {

            menu->addAction(actions.at(i));
        }

        advancedButton->setMenu(menu);
        connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(advancedActivityTriggered(QAction*)));

    } else {
        qDebug() << __FILE__ << __LINE__ << "Not enough perspective change actions provided";
    }
}

void QGCToolBar::advancedActivityTriggered(QAction* action)
{
    if (action->isChecked())
        advancedButton->setChecked(true);
}

void QGCToolBar::setActiveUAS(UASInterface* active)
{
    // Do nothing if system is the same
    if (mav == active || active == NULL)
        return;

    // If switching UASes, disconnect the only one.
    if (mav)
    {
        disconnect(mav, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString,QString)));
        disconnect(mav, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        disconnect(mav, SIGNAL(nameChanged(QString)), this, SLOT(updateName(QString)));
        disconnect(mav, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));
        disconnect(mav, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(receiveTextMessage(int,int,int,QString)));
        disconnect(mav, SIGNAL(batteryChanged(UASInterface*, double, double, double,int)), this, SLOT(updateBatteryRemaining(UASInterface*, double, double, double, int)));
        disconnect(mav, SIGNAL(armingChanged(bool)), this, SLOT(updateArmingState(bool)));
        disconnect(mav, SIGNAL(heartbeatTimeout(bool, unsigned int)), this, SLOT(heartbeatTimeout(bool,unsigned int)));
        if (mav->getWaypointManager())
        {
            disconnect(mav->getWaypointManager(), SIGNAL(currentWaypointChanged(quint16)), this, SLOT(updateCurrentWaypoint(quint16)));
            disconnect(mav->getWaypointManager(), SIGNAL(waypointDistanceChanged(double)), this, SLOT(updateWaypointDistance(double)));
        }
    }
    else
    {
        // Only update the UI once a UAS has been selected.
        updateViewTimer.start(2000);
    }

    // Connect new system
    mav = active;
    if (mav)
    {
        connect(mav, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*, QString,QString)));
        connect(mav, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        connect(mav, SIGNAL(nameChanged(QString)), this, SLOT(updateName(QString)));
        connect(mav, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));
        connect(mav, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(receiveTextMessage(int,int,int,QString)));
        connect(mav, SIGNAL(batteryChanged(UASInterface*,double,double,double,int)), this, SLOT(updateBatteryRemaining(UASInterface*,double,double,double,int)));
        connect(mav, SIGNAL(armingChanged(bool)), this, SLOT(updateArmingState(bool)));
        connect(mav, SIGNAL(heartbeatTimeout(bool, unsigned int)), this, SLOT(heartbeatTimeout(bool,unsigned int)));
        if (mav->getWaypointManager())
        {
            connect(mav->getWaypointManager(), SIGNAL(currentWaypointChanged(quint16)), this, SLOT(updateCurrentWaypoint(quint16)));
            connect(mav->getWaypointManager(), SIGNAL(waypointDistanceChanged(double)), this, SLOT(updateWaypointDistance(double)));
        }

        // Update all values once
        systemName = mav->getUASName();
        systemArmed = mav->isArmed();
        toolBarNameLabel->setText(mav->getUASName().replace("MAV", ""));
        toolBarNameLabel->setStyleSheet(QString("QLabel {color: %1;}").arg(mav->getColor().name()));
        symbolLabel->setStyleSheet(QString("QWidget {background-color: %1;}").arg(mav->getColor().name()));
        QString shortMode = mav->getShortMode();
        shortMode = shortMode.replace("D|", "");
        shortMode = shortMode.replace("A|", "");
        toolBarModeLabel->setText(shortMode);
//        toolBarStateLabel->setText(mav->getShortState());
        toolBarTimeoutAction->setVisible(false);
        toolBarMessageLabel->clear();
        lastSystemMessageTimeMs = 0;
        toolBarBatteryBar->setEnabled(true);
        setSystemType(mav, mav->getSystemType());
    }
    else
    {
        updateViewTimer.stop();
        resetToolbarUI();
    }
}

void QGCToolBar::updateArmingState(bool armed)
{
    systemArmed = armed;
    changed = true;
    /* important, immediately update */
    updateView();
}

void QGCToolBar::updateView()
{
    if (!changed) return;
    if (toolBarWpAction->isVisible())
        toolBarWpLabel->setText(tr("WP%1").arg(wpId));

    if (toolBarBatteryBarAction->isVisible()) {
        toolBarBatteryBar->setValue(batteryPercent);

        if (batteryPercent < 30 && toolBarBatteryBar->value() >= 30) {
            toolBarBatteryBar->setStyleSheet(qgcApp()->styleIsDark() ?
                                             "QProgressBar {color: #000} QProgressBar QProgressBar::chunk { background-color: #0F0}" :
                                             "QProgressBar {color: #FFF} QProgressBar::chunk { background-color: #008000}");
        } else if (batteryPercent >= 30 && toolBarBatteryBar->value() < 30){
            toolBarBatteryBar->setStyleSheet(qgcApp()->styleIsDark() ?
                                             "QProgressBar {color: #000} QProgressBar QProgressBar::chunk { background-color: #FF0}" :
                                             "QProgressBar {color: #FFF} QProgressBar::chunk { background-color: #808000}");
        }

    }
    if (toolBarBatteryVoltageLabel->isVisible()) {
	toolBarBatteryVoltageLabel->setText(tr("%1 V").arg(batteryVoltage, 4, 'f', 1, ' '));
    }


//    toolBarStateLabel->setText(QString("%1").arg(state));
    if (mode.size() > 0) {
        toolBarModeLabel->setText(QString("%1").arg(mode));
    }
    toolBarNameLabel->setText(systemName);
    // expire after 15 seconds

    if (toolBarMessageAction->isVisible()) {
        if (QGC::groundTimeMilliseconds() - lastSystemMessageTimeMs < 15000) {
            toolBarMessageLabel->setText(QString("%1").arg(lastSystemMessage));
        } else {
            toolBarMessageLabel->clear();
        }
    }

    // Display the system armed state with a red-on-yellow background if armed or green text if safe.
    if (systemArmed)
    {
        toolBarSafetyLabel->setStyleSheet(QString("QLabel {color: %1; background-color: %2; font-size: 15pt;}").arg(QGC::colorRed.name()).arg(QGC::colorYellow.name()));
        toolBarSafetyLabel->setText(tr("ARMED"));
    }
    else
    {
        toolBarSafetyLabel->setStyleSheet(qgcApp()->styleIsDark() ?
                                          "QLabel {color: #14C814; font-size: 15pt;}" :
                                          "QLabel {color: #0D820D; font-size: 15pt;}");
        toolBarSafetyLabel->setText(tr("DISARMED"));
    }

    changed = false;
}

void QGCToolBar::updateWaypointDistance(double distance)
{
    if (wpDistance != distance) changed = true;
    wpDistance = distance;
}

void QGCToolBar::updateCurrentWaypoint(quint16 id)
{
    if (wpId != id) changed = true;
    wpId = id;
}

void QGCToolBar::updateBatteryRemaining(UASInterface* uas, double voltage, double current, double percent, int seconds)
{
    Q_UNUSED(uas);
    Q_UNUSED(seconds);
    Q_UNUSED(current);

    if (batteryPercent != percent || batteryVoltage != voltage) changed = true;
    batteryPercent = percent;
    batteryVoltage = voltage;
}

void QGCToolBar::updateState(UASInterface* system, QString name, QString description)
{
    Q_UNUSED(system);
    Q_UNUSED(description);
    if (state != name) changed = true;
    state = name;
    /* important, immediately update */
    updateView();
}

void QGCToolBar::updateMode(int system, QString name, QString description)
{
    Q_UNUSED(system);
    Q_UNUSED(description);
    if (name.size() == 0) {
        qDebug() << "EMPTY MODE, RETURN";
    }

    QString shortMode = name;
    shortMode = shortMode.replace("D|", "");
    shortMode = shortMode.replace("A|", "");

    if (mode != shortMode) changed = true;
    mode = shortMode;
    /* important, immediately update */
    updateView();
}

void QGCToolBar::updateName(const QString& name)
{
    if (systemName != name)
    {
        changed = true;
    }
    systemName = name;
}

/**
 * The current system type is represented through the system icon.
 *
 * @param uas Source system, has to be the same as this->uas
 * @param systemType type ID, following the MAVLink system type conventions
 * @see http://pixhawk.ethz.ch/software/mavlink
 */
void QGCToolBar::setSystemType(UASInterface* uas, unsigned int systemType)
{
    Q_UNUSED(uas);
    QPixmap newPixmap;
    switch (systemType) {
    case MAV_TYPE_GENERIC:
        newPixmap = QPixmap(":/files/images/mavs/generic.svg");
        break;
    case MAV_TYPE_FIXED_WING:
        newPixmap = QPixmap(":/files/images/mavs/fixed-wing.svg");
        break;
    case MAV_TYPE_QUADROTOR:
        newPixmap = QPixmap(":/files/images/mavs/quadrotor.svg");
        break;
    case MAV_TYPE_COAXIAL:
        newPixmap = QPixmap(":/files/images/mavs/coaxial.svg");
        break;
    case MAV_TYPE_HELICOPTER:
        newPixmap = QPixmap(":/files/images/mavs/helicopter.svg");
        break;
    case MAV_TYPE_ANTENNA_TRACKER:
        newPixmap = QPixmap(":/files/images/mavs/antenna-tracker.svg");
        break;
    case MAV_TYPE_GCS:
        newPixmap = QPixmap(":files/images/mavs/groundstation.svg");
        break;
    case MAV_TYPE_AIRSHIP:
        newPixmap = QPixmap(":files/images/mavs/airship.svg");
        break;
    case MAV_TYPE_FREE_BALLOON:
        newPixmap = QPixmap(":files/images/mavs/free-balloon.svg");
        break;
    case MAV_TYPE_ROCKET:
        newPixmap = QPixmap(":files/images/mavs/rocket.svg");
        break;
    case MAV_TYPE_GROUND_ROVER:
        newPixmap = QPixmap(":files/images/mavs/ground-rover.svg");
        break;
    case MAV_TYPE_SURFACE_BOAT:
        newPixmap = QPixmap(":files/images/mavs/surface-boat.svg");
        break;
    case MAV_TYPE_SUBMARINE:
        newPixmap = QPixmap(":files/images/mavs/submarine.svg");
        break;
    case MAV_TYPE_HEXAROTOR:
        newPixmap = QPixmap(":files/images/mavs/hexarotor.svg");
        break;
    case MAV_TYPE_OCTOROTOR:
        newPixmap = QPixmap(":files/images/mavs/octorotor.svg");
        break;
    case MAV_TYPE_TRICOPTER:
        newPixmap = QPixmap(":files/images/mavs/tricopter.svg");
        break;
    case MAV_TYPE_FLAPPING_WING:
        newPixmap = QPixmap(":files/images/mavs/flapping-wing.svg");
        break;
    case MAV_TYPE_KITE:
        newPixmap = QPixmap(":files/images/mavs/kite.svg");
        break;
    default:
        newPixmap = QPixmap(":/files/images/mavs/unknown.svg");
        break;
    }
    symbolLabel->setPixmap(newPixmap.scaledToHeight(24));
}

void QGCToolBar::receiveTextMessage(int uasid, int componentid, int severity, QString text)
{
    Q_UNUSED(uasid);
    Q_UNUSED(componentid);
    Q_UNUSED(severity);
    if (lastSystemMessage != text) changed = true;
    lastSystemMessage = text;
    lastSystemMessageTimeMs = QGC::groundTimeMilliseconds();
}

void QGCToolBar::_updatePortList(void)
{
    if (!_linkCombo->isVisible()) {
        return;
    }

    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    
    foreach (QSerialPortInfo portInfo, portList) {
        if (_linkCombo->findText(portInfo.portName()) == -1) {
            _linkCombo->addItem(portInfo.portName());
            if (!_linkSelectedOnce && portInfo.vendorIdentifier() == 9900) {
                // Pre-Select 3DR connection
                _linkSelectedOnce = true;
                _linkCombo->setCurrentIndex(_linkCombo->findText(portInfo.portName()));
            }
        }
    }
}

void QGCToolBar::_linkConnected(LinkInterface* link)
{
    Q_UNUSED(link);
    _updateConnectButton();
}

void QGCToolBar::_linkDisconnected(LinkInterface* link)
{
    Q_UNUSED(link);
    _updateConnectButton();
}

void QGCToolBar::_updateConnectButton(void)
{
    QMenu* menu = new QMenu(this);
    
    // If there are multiple connected links add/update the connect button menu
    
    int connectedCount = 0;
    QList<LinkInterface*> links = _linkMgr->getLinks();
    foreach(LinkInterface* link, links) {
        if (link->isConnected()) {
            connectedCount++;
            QAction* action = menu->addAction(link->getName());
            action->setData(QVariant::fromValue((void*)link));
            connect(action, &QAction::triggered, this, &QGCToolBar::_disconnectFromMenu);
        }
    }
    
    // Remove old menu
    QMenu* oldMenu = _connectButton->menu();
    _connectButton->setMenu(NULL);
    if (oldMenu) {
        oldMenu->deleteLater();
    }
    
    // Add new menu if needed
    if (connectedCount > 1) {
        _connectButton->setMenu(menu);
    } else {
        delete menu;
    }
    
    _linksConnected = connectedCount != 0;
    
    _connectButton->setText(_linksConnected ? tr("Disconnect") : tr("Connect"));

    _linkComboAction->setVisible(!_linksConnected);
    _baudComboAction->setVisible(!_linksConnected);
    toolBarMessageAction->setVisible(_linksConnected);
    toolBarWpAction->setVisible(_linksConnected);
}

void QGCToolBar::_connectButtonClicked(bool checked)
{
    Q_UNUSED(checked);
    
    if (_linksConnected) {
        // Disconnect
        
        // Should be just one connected link, disconnect it
        
        int connectedCount = 0;
        LinkInterface* connectedLink = NULL;
        QList<LinkInterface*> links = _linkMgr->getLinks();
        foreach(LinkInterface* link, links) {
            if (link->isConnected()) {
                connectedCount++;
                connectedLink = link;
            }
        }
        Q_ASSERT(connectedCount == 1);
        Q_ASSERT(connectedLink);
        
        _linkMgr->disconnectLink(connectedLink);
    } else {
        // Connect
        
        QString linkName = _linkCombo->currentText();
        
        if (linkName == "WiFi") {
            UDPLink* link = new UDPLink;
            Q_CHECK_PTR(link);
            
            _linkMgr->addLink(link);
            CommConfigurationWindow* commDialog = new CommConfigurationWindow(link, this);
            commDialog->exec();
        } else {
            // Must be a serial port
            SerialLink* link = new SerialLink(linkName, _baudCombo->currentText().toInt());
            Q_CHECK_PTR(link);
            
            _linkMgr->addLink(link);
            _linkMgr->connectLink(link);
        }
    }
}

void QGCToolBar::_disconnectFromMenu(bool checked)
{
    Q_UNUSED(checked);
    
    QAction* action = qobject_cast<QAction*>(sender());
    Q_ASSERT(action);
    
    LinkInterface* link = (LinkInterface*)(action->data().value<void *>());
    Q_ASSERT(link);
    
    _linkMgr->disconnectLink(link);
}

void QGCToolBar::clearStatusString()
{
    if (toolBarMessageLabel->text().length() > 0)
    {
        lastSystemMessage = "";
        changed = true;
    }
}

void QGCToolBar::_linkComboActivated(int index)
{
    Q_UNUSED(index);
    
    _linkSelectedOnce = true;
}
