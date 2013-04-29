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
#include "QGCToolBar.h"
#include "UASManager.h"
#include "MainWindow.h"

QGCToolBar::QGCToolBar(QWidget *parent) :
    QToolBar(parent),
    mav(NULL),
    player(NULL),
    changed(true),
    batteryPercent(0),
    batteryVoltage(0),
    wpId(0),
    wpDistance(0),
    systemArmed(false),
    currentLink(NULL),
    firstAction(NULL)
{
    setObjectName("QGC_TOOLBAR");

    // Do not load UI, wait for actions
}

void QGCToolBar::heartbeatTimeout(bool timeout, unsigned int ms)
{
    // set timeout label visible
    if (timeout)
    {
        // Alternate colors to increase visibility
        if ((ms / 1000) % 2 == 0)
        {
            toolBarTimeoutLabel->setStyleSheet(QString("QLabel { margin: 3px 2px; font: 14px; color: %1; background-color: %2; border-radius: 4px;}").arg(QGC::colorDarkWhite.name()).arg(QGC::colorMagenta.name()));
        }
        else
        {
            toolBarTimeoutLabel->setStyleSheet(QString("QLabel { margin: 3px 2px; font: 14px; color: %1; background-color: %2; border-radius: 4px;}").arg(QGC::colorDarkWhite.name()).arg(QGC::colorMagenta.dark(250).name()));
        }
        toolBarTimeoutLabel->setText(tr("CONNECTION LOST: %1 s").arg((ms / 1000.0f), 2, 'f', 1, ' '));
    }
    else
    {
        // Check if loss text is present, reset once
        if (toolBarTimeoutLabel->text() != "")
        {
            toolBarTimeoutLabel->setText("");
            toolBarTimeoutLabel->setStyleSheet(QString(""));
        }
    }
}

void QGCToolBar::createUI()
{
    setStyleSheet("QToolBar {margin: 0px; border-bottom: 1px solid #484848; border-top: 1px solid #969696; background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #8B8B8B, stop:0.3 #808080, stop:0.34 #747474, stop:1 #484848);}");

    // CREATE TOOLBAR ITEMS
    // Add internal actions
    // Add MAV widget
    symbolButton = new QToolButton(this);
    symbolButton->setStyleSheet("QWidget { margin-left: 10px; background-color: #050508; color: #DDDDDF; background-clip: border; }");
    addWidget(symbolButton);

    toolBarNameLabel = new QLabel("------", this);
    toolBarNameLabel->setToolTip(tr("Currently controlled vehicle"));
    toolBarNameLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarNameLabel);

    toolBarTimeoutLabel = new QLabel("UNCONNECTED", this);
    toolBarTimeoutLabel->setToolTip(tr("System timed out, interval since last message"));
    toolBarTimeoutLabel->setStyleSheet(QString("QLabel { margin: 3px 2px; font: 14px; color: %1; background-color: %2; border-radius: 4px;}").arg(QGC::colorDarkWhite.name()).arg(QGC::colorMagenta.name()));
    toolBarTimeoutLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarTimeoutLabel);

    toolBarSafetyLabel = new QLabel("SAFE", this);
    toolBarSafetyLabel->setStyleSheet("QLabel { margin: 3px 2px; font: 14px; color: #14C814; }");
    toolBarSafetyLabel->setToolTip(tr("Vehicle safety state"));
    toolBarSafetyLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarSafetyLabel);

    toolBarModeLabel = new QLabel("------", this);
    toolBarModeLabel->setStyleSheet("QLabel { margin: 3px 2px; font: 14px; color: #ACEBFE; }");
    toolBarModeLabel->setToolTip(tr("Vehicle mode"));
    toolBarModeLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarModeLabel);

    toolBarStateLabel = new QLabel("------", this);
    toolBarStateLabel->setStyleSheet("QLabel { margin: 3px 2px; font: 14px; color: #FEC654; }");
    toolBarStateLabel->setToolTip(tr("Vehicle state"));
    toolBarStateLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarStateLabel);

    toolBarBatteryBar = new QProgressBar(this);
    toolBarBatteryBar->setStyleSheet("QProgressBar:horizontal { margin: 0px 4px 0px 0px; border: 1px solid #4A4A4F; border-radius: 4px; text-align: center; padding: 2px; color: #111111; background-color: #111118; height: 14px; } QProgressBar:horizontal QLabel { font-size: 9px; color: #111111; } QProgressBar::chunk { background-color: green; }");
    toolBarBatteryBar->setMinimum(0);
    toolBarBatteryBar->setMaximum(100);
    toolBarBatteryBar->setMinimumWidth(20);
    toolBarBatteryBar->setMaximumWidth(100);
    toolBarBatteryBar->setValue(0);
    toolBarBatteryBar->setToolTip(tr("Battery charge level"));
    addWidget(toolBarBatteryBar);

    toolBarBatteryVoltageLabel = new QLabel("xx.x V");
    toolBarBatteryVoltageLabel->setStyleSheet(QString("QLabel { margin: 0px 0px 0px 4px; font: 14px; color: %1; }").arg(QColor(Qt::green).name()));
    toolBarBatteryVoltageLabel->setToolTip(tr("Battery voltage"));
    toolBarBatteryVoltageLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarBatteryVoltageLabel);

    toolBarWpLabel = new QLabel("WP--", this);
    toolBarWpLabel->setStyleSheet("QLabel { margin: 3px 2px; font: 18px; color: #ACEBFE; }");
    toolBarWpLabel->setToolTip(tr("Current waypoint"));
    toolBarWpLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarWpLabel);

    toolBarDistLabel = new QLabel("--- ---- m", this);
    toolBarDistLabel->setToolTip(tr("Distance to current waypoint"));
    toolBarDistLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarDistLabel);

    toolBarMessageLabel = new QLabel("", this);
    toolBarMessageLabel->setStyleSheet("QLabel { margin: 3px 2px; font: 14px; color: #ACEBFE; }");
    toolBarMessageLabel->setToolTip(tr("Most recent system message"));
    addWidget(toolBarMessageLabel);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer->setStyleSheet("* { margin: 0px; background-color: transparent; min-height: 28px}");
    addWidget(spacer);

    connectButton = new QPushButton(tr("Connect"), this);
    connectButton->setToolTip(tr("Connect wireless link to MAV"));
    connectButton->setCheckable(true);
    connectButton->setStyleSheet("QPushButton {min-height: 20px; color: #222222; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #5AAA49, stop: 1 #106B38);  margin-left: 4px; margin-right: 4px; border-radius: 4px; border: 1px solid #085B35; } QPushButton:checked { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #FF9000, stop: 1 #FFD450); color: #222222; border-color: #D1892A}");
    addWidget(connectButton);
    connect(connectButton, SIGNAL(clicked(bool)), this, SLOT(connectLink(bool)));

    // DONE INITIALIZING BUTTONS

    // Configure the toolbar for the current default UAS
    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    if (LinkManager::instance()->getLinks().count() > 2)
        addLink(LinkManager::instance()->getLinks().last());
    // XXX implies that connect button is always active for the last used link
    connect(LinkManager::instance(), SIGNAL(newLink(LinkInterface*)), this, SLOT(addLink(LinkInterface*)));

    // Update label if required
    if (LinkManager::instance()->getLinks().count() < 3) {
        connectButton->setText(tr("New Link"));
    }

    // Set the toolbar to be updated every 2s
    connect(&updateViewTimer, SIGNAL(timeout()), this, SLOT(updateView()));
    updateViewTimer.start(2000);

    loadSettings();

    changed = false;
}

void QGCToolBar::setPerspectiveChangeActions(const QList<QAction*> &actions)
{
    if (actions.count() > 1)
    {
        QButtonGroup* group = new QButtonGroup(this);
        group->setExclusive(true);

        QToolButton *first = new QToolButton(this);
        // Add first button
        first->setIcon(actions.first()->icon());
        first->setText(actions.first()->text());
        first->setToolTip(actions.first()->toolTip());
        first->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        first->setCheckable(true);
        connect(first, SIGNAL(clicked(bool)), actions.first(), SIGNAL(triggered(bool)));
        first->setStyleSheet("QToolButton { min-width: 70px; color: #222222; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #A2A3A4, stop: 1 #B6B7B8); margin-left: 8px; margin-right: 0px; border-radius: 0px; border : 0px solid blue; border-bottom-left-radius: 6px; border-top-left-radius: 6px; border-left: 1px solid #484848; border-top: 1px solid #484848; border-bottom: 1px solid #484848; } QToolButton:checked { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #555555, stop: 1 #787878); color: #DDDDDD; }");
        addWidget(first);
        group->addButton(first);

        for (int i = 1; i < actions.count() - 1; i++)
        {
            // Add last button
            QToolButton *btn = new QToolButton(this);
            // Add first button
            btn->setIcon(actions.at(i)->icon());
            btn->setText(actions.at(i)->text());
            btn->setToolTip(actions.at(i)->toolTip());
            btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            btn->setCheckable(true);
            connect(btn, SIGNAL(clicked(bool)), actions.at(i), SIGNAL(triggered(bool)));
            btn->setStyleSheet("QToolButton { min-width: 70px; color: #222222; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #A2A3A4, stop: 1 #B6B7B8);  margin-left: -2px; margin-right: -2px; padding-left: 0px; padding-right: 0px; border-radius: 0px; border-top: 1px solid #484848; border-bottom: 1px solid #484848; } QToolButton:checked { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #555555, stop: 1 #787878); color: #DDDDDD; }");
            addWidget(btn);
            group->addButton(btn);
        }

        // Add last button
        QToolButton *last = new QToolButton(this);
        // Add first button
        last->setIcon(actions.last()->icon());
        last->setText(actions.last()->text());
        last->setToolTip(actions.last()->toolTip());
        last->setCheckable(true);
        connect(last, SIGNAL(clicked(bool)), actions.last(), SIGNAL(triggered(bool)));
        last->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        last->setStyleSheet("QToolButton { min-width: 70px; color: #222222; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #A2A3A4, stop: 1 #B6B7B8);  margin-left: 0px; margin-right: 8px; padding-left: 0px; padding-right: 0px; border-radius: 0px; border : 0px solid blue; border-bottom-right-radius: 6px; border-top-right-radius: 6px; border-right: 1px solid #484848; border-top: 1px solid #484848; border-bottom: 1px solid #484848; } QToolButton:checked { background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #555555, stop: 1 #787878); color: #DDDDDD; }");
        addWidget(last);
        group->addButton(last);
    } else {
        qDebug() << __FILE__ << __LINE__ << "Not enough perspective change actions provided";
    }

    // Add the "rest"
    createUI();
}

void QGCToolBar::setActiveUAS(UASInterface* active)
{
    // Do nothing if system is the same or NULL
    if ((active == NULL) || mav == active) return;

    if (mav)
    {
        // Disconnect old system
        disconnect(mav, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*,QString,QString)));
        disconnect(mav, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
        disconnect(mav, SIGNAL(nameChanged(QString)), this, SLOT(updateName(QString)));
        disconnect(mav, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));
        disconnect(mav, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(receiveTextMessage(int,int,int,QString)));
        disconnect(mav, SIGNAL(batteryChanged(UASInterface*,double,double,int)), this, SLOT(updateBatteryRemaining(UASInterface*,double,double,int)));
        disconnect(mav, SIGNAL(armingChanged(bool)), this, SLOT(updateArmingState(bool)));
        disconnect(mav, SIGNAL(heartbeatTimeout(bool, unsigned int)), this, SLOT(heartbeatTimeout(bool,unsigned int)));
        if (mav->getWaypointManager())
        {
            disconnect(mav->getWaypointManager(), SIGNAL(currentWaypointChanged(quint16)), this, SLOT(updateCurrentWaypoint(quint16)));
            disconnect(mav->getWaypointManager(), SIGNAL(waypointDistanceChanged(double)), this, SLOT(updateWaypointDistance(double)));
        }
    }

    // Connect new system
    mav = active;
    connect(active, SIGNAL(statusChanged(UASInterface*,QString,QString)), this, SLOT(updateState(UASInterface*, QString,QString)));
    connect(active, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int,QString,QString)));
    connect(active, SIGNAL(nameChanged(QString)), this, SLOT(updateName(QString)));
    connect(active, SIGNAL(systemTypeSet(UASInterface*,uint)), this, SLOT(setSystemType(UASInterface*,uint)));
    connect(active, SIGNAL(textMessageReceived(int,int,int,QString)), this, SLOT(receiveTextMessage(int,int,int,QString)));
    connect(active, SIGNAL(batteryChanged(UASInterface*,double,double,int)), this, SLOT(updateBatteryRemaining(UASInterface*,double,double,int)));
    connect(active, SIGNAL(armingChanged(bool)), this, SLOT(updateArmingState(bool)));
    connect(active, SIGNAL(heartbeatTimeout(bool, unsigned int)), this, SLOT(heartbeatTimeout(bool,unsigned int)));
    if (active->getWaypointManager())
    {
        connect(active->getWaypointManager(), SIGNAL(currentWaypointChanged(quint16)), this, SLOT(updateCurrentWaypoint(quint16)));
        connect(active->getWaypointManager(), SIGNAL(waypointDistanceChanged(double)), this, SLOT(updateWaypointDistance(double)));
    }

    // Update all values once
    systemName = mav->getUASName();
    systemArmed = mav->isArmed();
    toolBarNameLabel->setText(mav->getUASName());
    toolBarNameLabel->setStyleSheet(QString("QLabel { font: bold 16px; color: %1; }").arg(mav->getColor().name()));
    symbolButton->setStyleSheet(QString("QWidget { background-color: %1; color: #DDDDDF; background-clip: border; } QToolButton { font-weight: bold; font-size: 14px; border: 0px solid #484848; border-radius: 5px; min-width:22px; max-width: 22px; min-height: 22px; max-height: 22px; padding: 0px; margin: 0px 4px 0px 20px; background-color: none; }").arg(mav->getColor().name()));
    toolBarModeLabel->setText(mav->getShortMode());
    toolBarStateLabel->setText(mav->getShortState());
    toolBarTimeoutLabel->setStyleSheet(QString(""));
    toolBarTimeoutLabel->setText("");
    setSystemType(mav, mav->getSystemType());
}

void QGCToolBar::createCustomWidgets()
{

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
    toolBarDistLabel->setText(tr("%1 m").arg(wpDistance, 6, 'f', 2, '0'));
    toolBarWpLabel->setText(tr("WP%1").arg(wpId));
    toolBarBatteryBar->setValue(batteryPercent);
    if (batteryPercent < 30 && toolBarBatteryBar->value() >= 30) {
        toolBarBatteryBar->setStyleSheet("QProgressBar:horizontal { margin: 0px 4px 0px 0px; border: 1px solid #4A4A4F; border-radius: 4px; text-align: center; padding: 2px; color: #111111; background-color: #111118; height: 14px; } QProgressBar:horizontal QLabel { font-size: 9px; color: #111111; } QProgressBar::chunk { background-color: yellow; }");
    } else if (batteryPercent >= 30 && toolBarBatteryBar->value() < 30){
        toolBarBatteryBar->setStyleSheet("QProgressBar:horizontal { margin: 0px 4px 0px 0px; border: 1px solid #4A4A4F; border-radius: 4px; text-align: center; padding: 2px; color: #111111; background-color: #111118; height: 14px; } QProgressBar:horizontal QLabel { font-size: 9px; color: #111111; } QProgressBar::chunk { background-color: green; }");
    }

    toolBarBatteryVoltageLabel->setText(tr("%1 V").arg(batteryVoltage, 4, 'f', 1, ' '));
    toolBarStateLabel->setText(tr("%1").arg(state));
    toolBarModeLabel->setText(tr("%1").arg(mode));
    toolBarNameLabel->setText(systemName);
    // expire after 15 seconds
    if (QGC::groundTimeMilliseconds() - lastSystemMessageTimeMs < 15000) {
        toolBarMessageLabel->setText(tr("%1").arg(lastSystemMessage));
    } else {
        toolBarMessageLabel->setText(tr("%1").arg(""));
    }

    if (systemArmed)
    {
        toolBarSafetyLabel->setStyleSheet(QString("QLabel { margin: 3px 2px; font: 14px; color: %1; background-color: %2; border-radius: 4px;}").arg(QGC::colorRed.name()).arg(QGC::colorYellow.name()));
        toolBarSafetyLabel->setText(tr("ARMED"));
    }
    else
    {
        toolBarSafetyLabel->setStyleSheet("QLabel { margin: 3px 2px; font: 14px; color: #14C814; }");
        toolBarSafetyLabel->setText(tr("SAFE"));
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

void QGCToolBar::updateBatteryRemaining(UASInterface* uas, double voltage, double percent, int seconds)
{
    Q_UNUSED(uas);
    Q_UNUSED(seconds);
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
    if (mode != name) changed = true;
    mode = name;
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
        // Set matching icon
        switch (systemType) {
        case MAV_TYPE_GENERIC:
            symbolButton->setIcon(QIcon(":/files/images/mavs/generic.svg"));
            break;
        case MAV_TYPE_FIXED_WING:
            symbolButton->setIcon(QIcon(":/files/images/mavs/fixed-wing.svg"));
            break;
        case MAV_TYPE_QUADROTOR:
            symbolButton->setIcon(QIcon(":/files/images/mavs/quadrotor.svg"));
            break;
        case MAV_TYPE_COAXIAL:
            symbolButton->setIcon(QIcon(":/files/images/mavs/coaxial.svg"));
            break;
        case MAV_TYPE_HELICOPTER:
            symbolButton->setIcon(QIcon(":/files/images/mavs/helicopter.svg"));
            break;
        case MAV_TYPE_ANTENNA_TRACKER:
            symbolButton->setIcon(QIcon(":/files/images/mavs/antenna-tracker.svg"));
            break;
        case MAV_TYPE_GCS:
            symbolButton->setIcon(QIcon(":files/images/mavs/groundstation.svg"));
            break;
        case MAV_TYPE_AIRSHIP:
            symbolButton->setIcon(QIcon(":files/images/mavs/airship.svg"));
            break;
        case MAV_TYPE_FREE_BALLOON:
            symbolButton->setIcon(QIcon(":files/images/mavs/free-balloon.svg"));
            break;
        case MAV_TYPE_ROCKET:
            symbolButton->setIcon(QIcon(":files/images/mavs/rocket.svg"));
            break;
        case MAV_TYPE_GROUND_ROVER:
            symbolButton->setIcon(QIcon(":files/images/mavs/ground-rover.svg"));
            break;
        case MAV_TYPE_SURFACE_BOAT:
            symbolButton->setIcon(QIcon(":files/images/mavs/surface-boat.svg"));
            break;
        case MAV_TYPE_SUBMARINE:
            symbolButton->setIcon(QIcon(":files/images/mavs/submarine.svg"));
            break;
        case MAV_TYPE_HEXAROTOR:
            symbolButton->setIcon(QIcon(":files/images/mavs/hexarotor.svg"));
            break;
        case MAV_TYPE_OCTOROTOR:
            symbolButton->setIcon(QIcon(":files/images/mavs/octorotor.svg"));
            break;
        case MAV_TYPE_TRICOPTER:
            symbolButton->setIcon(QIcon(":files/images/mavs/tricopter.svg"));
            break;
        case MAV_TYPE_FLAPPING_WING:
            symbolButton->setIcon(QIcon(":files/images/mavs/flapping-wing.svg"));
            break;
        case MAV_TYPE_KITE:
            symbolButton->setIcon(QIcon(":files/images/mavs/kite.svg"));
            break;
        default:
            symbolButton->setIcon(QIcon(":/files/images/mavs/unknown.svg"));
            break;
        }
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

void QGCToolBar::addLink(LinkInterface* link)
{
    // XXX magic number
    if (LinkManager::instance()->getLinks().count() > 2) {
        currentLink = link;
        connect(currentLink, SIGNAL(connected(bool)), this, SLOT(updateLinkState(bool)));
        updateLinkState(link->isConnected());
    }
}

void QGCToolBar::removeLink(LinkInterface* link)
{
    if (link == currentLink) {
        currentLink = NULL;
        // XXX magic number
        if (LinkManager::instance()->getLinks().count() > 2) {
            currentLink = LinkManager::instance()->getLinks().last();
            updateLinkState(currentLink->isConnected());
        } else {
            connectButton->setText(tr("New Link"));
        }
    }
}

void QGCToolBar::updateLinkState(bool connected)
{
    Q_UNUSED(connected);
    if (currentLink && currentLink->isConnected())
    {
        connectButton->setText(tr("Disconnect"));
        connectButton->blockSignals(true);
        connectButton->setChecked(true);
        connectButton->blockSignals(false);
    }
    else
    {
        connectButton->setText(tr("Connect"));
        connectButton->blockSignals(true);
        connectButton->setChecked(false);
        connectButton->blockSignals(false);
    }
}

void QGCToolBar::connectLink(bool connect)
{
    // No serial port yet present
    // XXX magic number
    if (connect && LinkManager::instance()->getLinks().count() < 3)
    {
        MainWindow::instance()->addLink();
    } else if (connect) {
        LinkManager::instance()->getLinks().last()->connect();
    } else if (!connect && LinkManager::instance()->getLinks().count() > 2) {
        LinkManager::instance()->getLinks().last()->disconnect();
    }
}


void QGCToolBar::loadSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_TOOLBAR");
    settings.endGroup();
}

void QGCToolBar::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_TOOLBAR");
    settings.endGroup();
    settings.sync();
}

void QGCToolBar::clearStatusString()
{
    if (toolBarMessageLabel->text().length() > 0)
    {
        lastSystemMessage = "";
        changed = true;
    }
}

QGCToolBar::~QGCToolBar()
{
    storeSettings();
}
