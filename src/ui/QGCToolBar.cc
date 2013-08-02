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
    changed(true),
    batteryPercent(0),
    batteryVoltage(0),
    wpId(0),
    wpDistance(0),
    altitudeMSL(0),
    altitudeRel(0),
    systemArmed(false),
    currentLink(NULL),
    firstAction(NULL)
{
    setObjectName("QGCToolBar");
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    // Do not load UI, wait for actions
}

void QGCToolBar::globalPositionChanged(UASInterface* uas, double lat, double lon, double alt, quint64 usec)
{
    Q_UNUSED(uas);
    Q_UNUSED(lat);
    Q_UNUSED(lon);
    Q_UNUSED(usec);
    altitudeMSL = alt;
    changed = true;
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
    }
    else
    {
        // Check if loss text is present, reset once
        if (toolBarTimeoutAction->isVisible())
        {
            toolBarTimeoutAction->setVisible(false);
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

    toolBarStateLabel = new QLabel(this);
    toolBarStateLabel->setToolTip(tr("Vehicle state"));
    toolBarStateLabel->setObjectName("toolBarStateLabel");
    toolBarStateLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarStateLabel);

    toolBarBatteryBar = new QProgressBar(this);
    toolBarBatteryBar->setMinimum(0);
    toolBarBatteryBar->setMaximum(100);
    toolBarBatteryBar->setMinimumWidth(20);
    toolBarBatteryBar->setMaximumWidth(100);
    toolBarBatteryBar->setToolTip(tr("Battery charge level"));
    toolBarBatteryBar->setObjectName("toolBarBatteryBar");
    toolBarBatteryBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
    addWidget(toolBarBatteryBar);

    toolBarBatteryVoltageLabel = new QLabel(this);
    toolBarBatteryVoltageLabel->setToolTip(tr("Battery voltage"));
    toolBarBatteryVoltageLabel->setObjectName("toolBarBatteryVoltageLabel");
    toolBarBatteryVoltageLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarBatteryVoltageLabel);

    toolBarWpLabel = new QLabel(this);
    toolBarWpLabel->setToolTip(tr("Current waypoint"));
    toolBarWpLabel->setObjectName("toolBarWpLabel");
    toolBarWpLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarWpLabel);

    toolBarDistLabel = new QLabel(this);
    toolBarDistLabel->setToolTip(tr("Distance to current waypoint"));
    toolBarDistLabel->setAlignment(Qt::AlignCenter);
    addWidget(toolBarDistLabel);

    toolBarMessageLabel = new QLabel(this);
    toolBarMessageLabel->setToolTip(tr("Most recent system message"));
    toolBarMessageLabel->setObjectName("toolBarMessageLabel");
    addWidget(toolBarMessageLabel);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    addWidget(spacer);

    connectButton = new QPushButton(tr("Connect"), this);
    connectButton->setObjectName("connectButton");
    connectButton->setToolTip(tr("Connect wireless link to MAV"));
    connectButton->setCheckable(true);
    addWidget(connectButton);
    connect(connectButton, SIGNAL(clicked(bool)), this, SLOT(connectLink(bool)));

    resetToolbarUI();

    // DONE INITIALIZING BUTTONS

    // Set the toolbar to be updated every 2s
    connect(&updateViewTimer, SIGNAL(timeout()), this, SLOT(updateView()));

    // Configure the toolbar for the current default UAS
    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    if (LinkManager::instance()->getLinks().count() > 2)
        addLink(LinkManager::instance()->getLinks().last());
    // XXX implies that connect button is always active for the last used link
    connect(LinkManager::instance(), SIGNAL(newLink(LinkInterface*)), this, SLOT(addLink(LinkInterface*)));
    connect(LinkManager::instance(), SIGNAL(linkRemoved(LinkInterface*)), this, SLOT(removeLink(LinkInterface*)));

    // Update label if required
    if (LinkManager::instance()->getLinks().count() < 3) {
        connectButton->setText(tr("New Link"));
    }

    loadSettings();

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
    toolBarStateLabel->setText("------");
    toolBarBatteryBar->setValue(0);
    toolBarBatteryBar->setDisabled(true);
    toolBarBatteryVoltageLabel->setText("xx.x V");
    toolBarWpLabel->setText("WP--");
    toolBarDistLabel->setText("--- ---- m");
    toolBarMessageLabel->clear();
    lastSystemMessage = "";
    lastSystemMessageTimeMs = 0;
    symbolLabel->setStyleSheet("");
    symbolLabel->clear();
}

void QGCToolBar::setPerspectiveChangeActions(const QList<QAction*> &actions)
{
    if (actions.count() > 1)
    {
        group = new QButtonGroup(this);
        group->setExclusive(true);

        // Add the first button.
        QToolButton *first = new QToolButton(this);
        first->setIcon(actions.first()->icon());
        first->setText(actions.first()->text());
        first->setToolTip(actions.first()->toolTip());
        first->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        first->setCheckable(true);
        connect(first, SIGNAL(clicked(bool)), actions.first(), SIGNAL(triggered(bool)));
        connect(actions.first(),SIGNAL(triggered(bool)),first,SLOT(setChecked(bool)));
        first->setObjectName("firstAction");
        addWidget(first);
        group->addButton(first);

        // Add all the middle buttons.
        for (int i = 1; i < actions.count(); i++)
        {
            QToolButton *btn = new QToolButton(this);
            btn->setIcon(actions.at(i)->icon());
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
        advancedButton->setText(tr("Pro"));
        advancedButton->setToolTip(tr("Options for advanced users"));
        advancedButton->setCheckable(true);
        advancedButton->setObjectName("advancedButton");
        advancedButton->setPopupMode(QToolButton::InstantPopup);
        advancedButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
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
        disconnect(active, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(globalPositionChanged(UASInterface*,double,double,double,quint64)));
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
        connect(mav, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(globalPositionChanged(UASInterface*,double,double,double,quint64)));
        if (mav->getWaypointManager())
        {
            connect(mav->getWaypointManager(), SIGNAL(currentWaypointChanged(quint16)), this, SLOT(updateCurrentWaypoint(quint16)));
            connect(mav->getWaypointManager(), SIGNAL(waypointDistanceChanged(double)), this, SLOT(updateWaypointDistance(double)));
        }

        // Update all values once
        systemName = mav->getUASName();
        systemArmed = mav->isArmed();
        toolBarNameLabel->setText(mav->getUASName());
        toolBarNameLabel->setStyleSheet(QString("QLabel {color: %1;}").arg(mav->getColor().name()));
        symbolLabel->setStyleSheet(QString("QWidget {background-color: %1;}").arg(mav->getColor().name()));
        toolBarModeLabel->setText(mav->getShortMode());
        toolBarStateLabel->setText(mav->getShortState());
        toolBarTimeoutAction->setVisible(false);
        toolBarMessageLabel->clear();
        lastSystemMessageTimeMs = 0;
        toolBarDistLabel->clear();
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
    //toolBarDistLabel->setText(tr("%1 m").arg(wpDistance, 6, 'f', 2, '0'));
    // XXX add also rel altitude
    toolBarDistLabel->setText(QString("%1 m MSL").arg(altitudeMSL, 6, 'f', 2, '0'));
    toolBarWpLabel->setText(tr("WP%1").arg(wpId));
    toolBarBatteryBar->setValue(batteryPercent);
    if (batteryPercent < 30 && toolBarBatteryBar->value() >= 30) {
        if (MainWindow::instance()->getStyle() == MainWindow::QGC_MAINWINDOW_STYLE_LIGHT)
        {
            toolBarBatteryBar->setStyleSheet("QProgressBar {color: #FFF} QProgressBar::chunk { background-color: #008000}");
        }
        else
        {
            toolBarBatteryBar->setStyleSheet("QProgressBar {color: #000} QProgressBar QProgressBar::chunk { background-color: #0F0}");
        }
    } else if (batteryPercent >= 30 && toolBarBatteryBar->value() < 30){
        if (MainWindow::instance()->getStyle() == MainWindow::QGC_MAINWINDOW_STYLE_LIGHT)
        {
            toolBarBatteryBar->setStyleSheet("QProgressBar {color: #FFF} QProgressBar::chunk { background-color: #808000}");
        }
        else
        {
            toolBarBatteryBar->setStyleSheet("QProgressBar {color: #000} QProgressBar QProgressBar::chunk { background-color: #FF0}");
        }
    }

    toolBarBatteryVoltageLabel->setText(tr("%1 V").arg(batteryVoltage, 4, 'f', 1, ' '));
    toolBarStateLabel->setText(QString("%1").arg(state));
    toolBarModeLabel->setText(QString("%1").arg(mode));
    toolBarNameLabel->setText(systemName);
    // expire after 15 seconds
    if (QGC::groundTimeMilliseconds() - lastSystemMessageTimeMs < 15000) {
        toolBarMessageLabel->setText(QString("%1").arg(lastSystemMessage));
    } else {
        toolBarMessageLabel->clear();
    }

    // Display the system armed state with a red-on-yellow background if armed or green text if safe.
    if (systemArmed)
    {
        toolBarSafetyLabel->setStyleSheet(QString("QLabel {color: %1; background-color: %2; font-size: 15pt;}").arg(QGC::colorRed.name()).arg(QGC::colorYellow.name()));
        toolBarSafetyLabel->setText(tr("ARMED"));
    }
    else
    {
        if (MainWindow::instance()->getStyle() == MainWindow::QGC_MAINWINDOW_STYLE_LIGHT)
        {
            toolBarSafetyLabel->setStyleSheet("QLabel {color: #0D820D; font-size: 15pt;}");
        }
        else
        {
            toolBarSafetyLabel->setStyleSheet("QLabel {color: #14C814; font-size: 15pt;}");
        }
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
