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
#include "QGCToolBar.h"
#include "UASManager.h"
#include "MainWindow.h"

QGCToolBar::QGCToolBar(QWidget *parent) :
    QToolBar(parent),
    toggleLoggingAction(NULL),
    logReplayAction(NULL),
    mav(NULL),
    player(NULL),
    changed(true),
    batteryPercent(0),
    batteryVoltage(0),
    wpId(0),
    wpDistance(0),
    systemArmed(false)
{
    setObjectName("QGC_TOOLBAR");

    toggleLoggingAction = new QAction(QIcon(":"), "Logging", this);
    toggleLoggingAction->setCheckable(true);
    logReplayAction = new QAction(QIcon(":"), "Replay", this);
    logReplayAction->setCheckable(false);

    addAction(toggleLoggingAction);
    addAction(logReplayAction);

    // CREATE TOOLBAR ITEMS
    // Add internal actions
    // Add MAV widget
    symbolButton = new QToolButton(this);
    symbolButton->setStyleSheet("QWidget { background-color: #050508; color: #DDDDDF; background-clip: border; } QToolButton { font-weight: bold; font-size: 12px; border: 0px solid #999999; border-radius: 5px; min-width:22px; max-width: 22px; min-height: 22px; max-height: 22px; padding: 0px; margin: 0px 0px 0px 20px; background-color: none; }");
	addWidget(symbolButton);

    toolBarNameLabel = new QLabel("------", this);
	toolBarNameLabel->setToolTip(tr("Currently controlled vehicle"));
    addWidget(toolBarNameLabel);

    toolBarTimeoutLabel = new QLabel("UNCONNECTED", this);
    toolBarTimeoutLabel->setToolTip(tr("System timed out, interval since last message"));
    toolBarTimeoutLabel->setStyleSheet(QString("QLabel { margin: 0px 2px; font: 14px; color: %1; background-color: %2; }").arg(QGC::colorDarkWhite.name()).arg(QGC::colorMagenta.name()));
    addWidget(toolBarTimeoutLabel);

    toolBarSafetyLabel = new QLabel("SAFE", this);
    toolBarSafetyLabel->setStyleSheet("QLabel { margin: 0px 2px; font: 14px; color: #14C814; }");
	toolBarSafetyLabel->setToolTip(tr("Vehicle safety state"));
    addWidget(toolBarSafetyLabel);

    toolBarModeLabel = new QLabel("------", this);
    toolBarModeLabel->setStyleSheet("QLabel { margin: 0px 2px; font: 14px; color: #3C7B9E; }");
	toolBarModeLabel->setToolTip(tr("Vehicle mode"));
    addWidget(toolBarModeLabel);

    toolBarStateLabel = new QLabel("------", this);
    toolBarStateLabel->setStyleSheet("QLabel { margin: 0px 2px; font: 14px; color: #FEC654; }");
	toolBarStateLabel->setToolTip(tr("Vehicle state"));
    addWidget(toolBarStateLabel);

    toolBarBatteryBar = new QProgressBar(this);
    toolBarBatteryBar->setStyleSheet("QProgressBar:horizontal { margin: 0px 4px 0px 0px; border: 1px solid #4A4A4F; border-radius: 4px; text-align: center; padding: 2px; color: #111111; background-color: #111118; height: 10px; } QProgressBar:horizontal QLabel { font-size: 9px; color: #111111; } QProgressBar::chunk { background-color: green; }");
    toolBarBatteryBar->setMinimum(0);
    toolBarBatteryBar->setMaximum(100);
    toolBarBatteryBar->setMinimumWidth(20);
    toolBarBatteryBar->setMaximumWidth(100);
    toolBarBatteryBar->setToolTip(tr("Battery charge level"));
    addWidget(toolBarBatteryBar);

    toolBarBatteryVoltageLabel = new QLabel("xx.x V");
    toolBarBatteryVoltageLabel->setStyleSheet(QString("QLabel { margin: 0px 0px 0px 4px; font: 14px; color: %1; }").arg(QColor(Qt::green).name()));
	toolBarBatteryVoltageLabel->setToolTip(tr("Battery voltage"));
    addWidget(toolBarBatteryVoltageLabel);

    toolBarWpLabel = new QLabel("WP--", this);
    toolBarWpLabel->setStyleSheet("QLabel { margin: 0px 2px; font: 18px; color: #3C7B9E; }");
	toolBarWpLabel->setToolTip(tr("Current mission"));
    addWidget(toolBarWpLabel);

    toolBarDistLabel = new QLabel("--- ---- m", this);
	toolBarDistLabel->setToolTip(tr("Distance to current mission"));
    addWidget(toolBarDistLabel);

    toolBarMessageLabel = new QLabel("No system messages.", this);
    toolBarMessageLabel->setStyleSheet("QLabel { margin: 0px 4px; font: 12px; font-style: italic; color: #3C7B9E; }");
	toolBarMessageLabel->setToolTip(tr("Most recent system message"));
    addWidget(toolBarMessageLabel);

    // DONE INITIALIZING BUTTONS

	// Configure the toolbar for the current default UAS
    setActiveUAS(UASManager::instance()->getActiveUAS());
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

    // Set the toolbar to be updated every 2s
    connect(&updateViewTimer, SIGNAL(timeout()), this, SLOT(updateView()));
    updateViewTimer.start(2000);
}

void QGCToolBar::heartbeatTimeout(bool timeout, unsigned int ms)
{
    // set timeout label visible
    if (timeout)
    {
        // Alternate colors to increase visibility
        if ((ms / 1000) % 2 == 0)
        {
            toolBarTimeoutLabel->setStyleSheet(QString("QLabel { margin: 0px 2px; font: 14px; color: %1; background-color: %2; }").arg(QGC::colorDarkWhite.name()).arg(QGC::colorMagenta.name()));
        }
        else
        {
            toolBarTimeoutLabel->setStyleSheet(QString("QLabel { margin: 0px 2px; font: 14px; color: %1; background-color: %2; }").arg(QGC::colorDarkWhite.name()).arg(QGC::colorMagenta.dark(250).name()));
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

void QGCToolBar::setLogPlayer(QGCMAVLinkLogPlayer* player)
{
    this->player = player;
    connect(toggleLoggingAction, SIGNAL(triggered(bool)), this, SLOT(logging(bool)));
    connect(logReplayAction, SIGNAL(triggered(bool)), this, SLOT(playLogFile(bool)));
}

void QGCToolBar::playLogFile(bool checked)
{
    // Check if player exists
    if (player)
    {
        // If a logfile is already replayed, stop the replay
        // and select a new logfile
        if (player->isPlayingLogFile())
        {
            player->playPause(false);
            if (checked)
            {
                if (!player->selectLogFile()) return;
            }
        }
        // If no replaying happens already, start it
        else
        {
            if (!player->selectLogFile()) return;
        }
        player->playPause(checked);
    }
}

void QGCToolBar::logging(bool checked)
{
    // Stop logging in any case
    MainWindow::instance()->getMAVLink()->enableLogging(false);

	// If the user is enabling logging
    if (checked)
    {
		// Prompt the user for a filename/location to save to
        QString fileName = QFileDialog::getSaveFileName(this, tr("Specify MAVLink log file to save to"), QDesktopServices::storageLocation(QDesktopServices::DesktopLocation), tr("MAVLink Logfile (*.mavlink *.log *.bin);;"));

		// Check that they didn't cancel out
		if (fileName.isNull())
		{
			toggleLoggingAction->setChecked(false);
			return;
		}

		// Make sure the file's named properly
        if (!fileName.endsWith(".mavlink"))
        {
            fileName.append(".mavlink");
        }

		// Check that we can save the logfile
        QFileInfo file(fileName);
        if ((file.exists() && !file.isWritable()))
        {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("The selected logfile is not writable"));
            msgBox.setInformativeText(tr("Please make sure that the file %1 is writable or select a different file").arg(fileName));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        }
		// Otherwise we're off and logging
        else
        {
            MainWindow::instance()->getMAVLink()->setLogfileName(fileName);
            MainWindow::instance()->getMAVLink()->enableLogging(true);
        }
    }
}

void QGCToolBar::addPerspectiveChangeAction(QAction* action)
{
    insertAction(toggleLoggingAction, action);

    // Set tab style
    QWidget* widget = widgetForAction(action);
    widget->setStyleSheet("\
                          * { font-weight: bold; min-height: 16px; min-width: 24px; \
                          border-top: 1px solid #BBBBBB; \
                          border-bottom: 0px; \
                          border-left: 1px solid #BBBBBB; \
                          border-right: 1px solid #BBBBBB; \
                          border-top-left-radius: 5px; \
                          border-top-right-radius: 5px; \
                          border-bottom-left-radius: 0px; \
                          border-bottom-right-radius: 0px; \
                          max-height: 22px; \
                          margin-top: 4px; \
                          margin-left: 2px; \
                          margin-bottom: 0px; \
                          margin-right: 2px; \
                          background-color: #222222; \
                  } \
                  *:checked { \
                          background-color: #000000; \
                          border-top: 2px solid #379AC3; \
                          border-bottom: 0px; \
                          border-left: 2px solid #379AC3; \
                          border-right: 2px solid #379AC3; \
                  } \
                  *:pressed { \
                          background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #bbbbbb, stop: 1 #b0b0b0); \
                          border-top: 2px solid #379AC3; \
                          border-bottom: 0px; \
                          border-left: 2px solid #379AC3; \
                          border-right: 2px solid #379AC3; }");
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
    symbolButton->setStyleSheet(QString("QWidget { background-color: %1; color: #DDDDDF; background-clip: border; } QToolButton { font-weight: bold; font-size: 12px; border: 0px solid #999999; border-radius: 5px; min-width:22px; max-width: 22px; min-height: 22px; max-height: 22px; padding: 0px; margin: 0px 4px 0px 20px; background-color: none; }").arg(mav->getColor().name()));
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
}

void QGCToolBar::updateView()
{
    if (!changed) return;
    toolBarDistLabel->setText(tr("%1 m").arg(wpDistance, 6, 'f', 2, '0'));
    toolBarWpLabel->setText(tr("WP%1").arg(wpId));
    toolBarBatteryBar->setValue(batteryPercent);
    toolBarBatteryVoltageLabel->setText(tr("%1 V").arg(batteryVoltage, 4, 'f', 1, ' '));
    toolBarStateLabel->setText(tr("%1").arg(state));
    toolBarModeLabel->setText(tr("%1").arg(mode));
    toolBarNameLabel->setText(systemName);
    toolBarMessageLabel->setText(lastSystemMessage);

    if (systemArmed)
    {
        toolBarSafetyLabel->setStyleSheet(QString("QLabel { margin: 0px 2px; font: 14px; color: %1; background-color: %2; }").arg(QGC::colorRed.name()).arg(QGC::colorYellow.name()));
        toolBarSafetyLabel->setText(tr("ARMED"));
    }
    else
    {
        toolBarSafetyLabel->setStyleSheet("QLabel { margin: 0px 2px; font: 14px; color: #14C814; }");
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
}

void QGCToolBar::updateMode(int system, QString name, QString description)
{
    Q_UNUSED(system);
    Q_UNUSED(description);
    if (mode != name) changed = true;
    mode = name;
}

void QGCToolBar::updateName(const QString& name)
{
    if (systemName != name) changed = true;
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
            symbolButton->setIcon(QIcon(":/files/images/mavs/antenn-tracker.svg"));
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
}

QGCToolBar::~QGCToolBar()
{
    if (toggleLoggingAction) toggleLoggingAction->deleteLater();
    if (logReplayAction) logReplayAction->deleteLater();
}
