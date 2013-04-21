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

#ifndef QGCTOOLBAR_H
#define QGCTOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "UASInterface.h"
#include "QGCMAVLinkLogPlayer.h"

class QGCToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit QGCToolBar(QWidget* parent = 0);
    void addPerspectiveChangeAction(QAction* action);
    ~QGCToolBar();

public slots:
    /** @brief Set the system that is currently displayed by this widget */
    void setActiveUAS(UASInterface* active);
    /** @brief Set the link which is currently handled with connecting / disconnecting */
    void addLink(LinkInterface* link);
    /** @brief Remove link which is currently handled */
    void removeLink(LinkInterface* link);
    /** @brief Update the link state */
    void updateLinkState(bool connected);
    /** @brief Set the system state */
    void updateState(UASInterface* system, QString name, QString description);
    /** @brief Set the system mode */
    void updateMode(int system, QString name, QString description);
    /** @brief Update the system name */
    void updateName(const QString& name);
    /** @brief Set the MAV system type */
    void setSystemType(UASInterface* uas, unsigned int systemType);
    /** @brief Received system text message */
    void receiveTextMessage(int uasid, int componentid, int severity, QString text);
    /** @brief Start / stop logging */
    void logging(bool checked);
    /** @brief Start playing logfile */
    void playLogFile(bool checked);
    /** @brief Set log playing component */
    void setLogPlayer(QGCMAVLinkLogPlayer* player);
    /** @brief Update battery charge state */
    void updateBatteryRemaining(UASInterface* uas, double voltage, double percent, int seconds);
    /** @brief Update current waypoint */
    void updateCurrentWaypoint(quint16 id);
    /** @brief Update distance to current waypoint */
    void updateWaypointDistance(double distance);
    /** @brief Update arming state */
    void updateArmingState(bool armed);
    /** @brief Repaint widgets */
    void updateView();
    /** @brief Update connection timeout time */
    void heartbeatTimeout(bool timeout, unsigned int ms);
    /** @brief Create or connect link */
    void connectLink(bool connect);
    /** @brief Clear status string */
    void clearStatusString();

protected:
    void createCustomWidgets();
    void storeSettings();
    void loadSettings();

    QAction* toggleLoggingAction;
    QAction* logReplayAction;
    UASInterface* mav;
    QToolButton* symbolButton;
    QLabel* toolBarNameLabel;
    QLabel* toolBarTimeoutLabel;
    QLabel* toolBarSafetyLabel;
    QLabel* toolBarModeLabel;
    QLabel* toolBarStateLabel;
    QLabel* toolBarWpLabel;
    QLabel* toolBarDistLabel;
    QLabel* toolBarMessageLabel;
    QPushButton* connectButton;
    QProgressBar* toolBarBatteryBar;
    QLabel* toolBarBatteryVoltageLabel;
    QGCMAVLinkLogPlayer* player;
    bool changed;
    float batteryPercent;
    float batteryVoltage;
    int wpId;
    double wpDistance;
    QString state;
    QString mode;
    QString systemName;
    QString lastSystemMessage;
    quint64 lastSystemMessageTimeMs;
    QTimer updateViewTimer;
    bool systemArmed;
    QString lastLogDirectory;
    LinkInterface* currentLink;
};

#endif // QGCTOOLBAR_H
