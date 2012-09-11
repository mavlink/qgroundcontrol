/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
 *   @brief Definition of class UASView
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef UASVIEW_H
#define UASVIEW_H

#include <QtGui/QWidget>
#include <QString>
#include <QTimer>
#include <QMouseEvent>
#include <UASInterface.h>

namespace Ui
{
class UASView;
}

class UASView : public QWidget
{
    Q_OBJECT
public:
    UASView(UASInterface* uas, QWidget *parent = 0);
    ~UASView();

public slots:
    /** @brief Update the name of the system */
    void updateName(const QString& name);
    void receiveHeartbeat(UASInterface* uas);
    void updateThrust(UASInterface* uas, double thrust);
    void updateBattery(UASInterface* uas, double voltage, double percent, int seconds);
    void updateLocalPosition(UASInterface*, double x, double y, double z, quint64 usec);
    void updateGlobalPosition(UASInterface*, double lon, double lat, double alt, quint64 usec);
    void updateSpeed(UASInterface*, double x, double y, double z, quint64 usec);
    void updateState(UASInterface*, QString uasState, QString stateDescription);
    /** @brief Update the MAV mode */
    void updateMode(int sysId, QString status, QString description);
    void updateLoad(UASInterface* uas, double load);
    //void receiveValue(int uasid, QString id, double value, quint64 time);
    void refresh();
    /** @brief Receive new waypoint information */
    void setWaypoint(int uasId, int id, double x, double y, double z, double yaw, bool autocontinue, bool current);
    /** @brief Update the current active waypoint */
    void currentWaypointUpdated(quint16 waypoint);
    /** @brief Set waypoint as current target */
    void selectWaypoint(int uasId, int id);
    /** @brief Set the current system type */
    void setSystemType(UASInterface* uas, unsigned int systemType);
    /** @brief Set the current UAS as the globally active system */
    void setUASasActive(bool);
    /** @brief Update the view if an UAS has been set to active */
    void updateActiveUAS(UASInterface* uas, bool active);
    /** @brief Set the widget into critical mode */
    void heartbeatTimeout(bool timeout, unsigned int ms);
    /** @brief Set the background color for the widget */
    void setBackgroundColor();
    /** @brief Bring up the dialog to rename the system */
    void rename();
    /** @brief Select airframe for this vehicle */
    void selectAirframe();
    /** @brief Select the battery type */
    void setBatterySpecs();
    /** @brief Show a status text message */
    void showStatusText(int uasid, int componentid, int severity, QString text);
    /** @brief Update the navigation mode state */
    void updateNavMode(int uasid, int mode, const QString& text);

protected:
    void changeEvent(QEvent *e);
    QTimer* refreshTimer;
    QColor heartbeatColor;
    quint64 startTime;
    bool timeout;
    bool iconIsRed;
    int timeRemaining;
    float chargeLevel;
    UASInterface* uas;
    float load;
    QString state;
    QString stateDesc;
    QString mode;
    double thrust; ///< Current vehicle thrust: 0 - 1.0 for 100% thrust
    bool isActive; ///< Is this MAV selected by the user?
    float x;
    float y;
    float z;
    float totalSpeed;
    float lat;
    float lon;
    float alt;
    float groundDistance;
    bool localFrame;
    bool globalFrameKnown;
    QAction* removeAction;
    QAction* renameAction;
    QAction* selectAction;
    QAction* hilAction;
    QAction* hilXAction;
    QAction* selectAirframeAction;
    QAction* setBatterySpecsAction;
    static const int updateInterval = 800;
    static const int errorUpdateInterval = 200;
    bool lowPowerModeEnabled; ///< Low power mode reduces update rates
    unsigned int generalUpdateCount; ///< Skip counter for updates
    double filterTime; ///< Filter time estimate of battery


    void mouseDoubleClickEvent (QMouseEvent * event);
    /** @brief Mouse enters the widget */
    void enterEvent(QEvent* event);
    /** @brief Mouse leaves the widget */
    void leaveEvent(QEvent* event);
    /** @brief Start widget updating */
    void showEvent(QShowEvent* event);
    /** @brief Stop widget updating */
    void hideEvent(QHideEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);

private:
    Ui::UASView *m_ui;

signals:
    void uasInFocus(UASInterface* uas);
    void uasOutFocus(UASInterface* uas);
};

#endif // UASVIEW_H
