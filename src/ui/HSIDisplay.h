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
 *   @brief Definition of of Horizontal Situation Indicator class
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef HSIDISPLAY_H
#define HSIDISPLAY_H

#include <QtGui/QWidget>
#include <QColor>
#include <QTimer>
#include <QMap>
#include <QPair>
#include <QMouseEvent>
#include <cmath>

#include "HDDisplay.h"
#include "MG.h"

class HSIDisplay : public HDDisplay
{
    Q_OBJECT
public:
    HSIDisplay(QWidget *parent = 0);
    ~HSIDisplay();

public slots:
    void setActiveUAS(UASInterface* uas);
    /** @brief Set the width in meters this widget shows from top */
    void setMetricWidth(double width);
    void updateSatellite(int uasid, int satid, float azimuth, float direction, float snr, bool used);
    void updateAttitudeSetpoints(UASInterface*, double rollDesired, double pitchDesired, double yawDesired, double thrustDesired, quint64 usec);
    void updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 time);
    void updateUserPositionSetpoints(int uasid, float xDesired, float yDesired, float zDesired, float yawDesired);
    void updatePositionSetpoints(int uasid, float xDesired, float yDesired, float zDesired, float yawDesired, quint64 usec);
    void updateLocalPosition(UASInterface*, double x, double y, double z, quint64 usec);
    void updateGlobalPosition(UASInterface*, double lat, double lon, double alt, quint64 usec);
    void updateSpeed(UASInterface* uas, double vx, double vy, double vz, quint64 time);
    void updatePositionLock(UASInterface* uas, bool lock);
    void updateAttitudeControllerEnabled(bool enabled);
    void updatePositionXYControllerEnabled(bool enabled);
    void updatePositionZControllerEnabled(bool enabled);
    void updateObjectPosition(unsigned int time, int id, int type, const QString& name, int quality, float bearing, float distance);
    /** @brief Heading control enabled/disabled */
    void updatePositionYawControllerEnabled(bool enabled);

    /** @brief Localization quality changed */
    void updateLocalization(UASInterface* uas, int localization);
    /** @brief GPS localization quality changed */
    void updateGpsLocalization(UASInterface* uas, int localization);
    /** @brief Vision localization quality changed */
    void updateVisionLocalization(UASInterface* uas, int localization);

    /** @brief Ultrasound/Infrared localization changed */
    void updateInfraredUltrasoundLocalization(UASInterface* uas, int fix);

    /** @brief Repaint the widget */
    void paintEvent(QPaintEvent * event);
    /** @brief Update state from joystick */
    void updateJoystick(double roll, double pitch, double yaw, double thrust, int xHat, int yHat);
    void pressKey(int key);
    /** @brief Reset the state of the view */
    void resetMAVState();
    /** @brief Clear the status message */
    void clearStatusMessage()
    {
        statusMessage = "";
        if (actionPending) statusMessage = "TIMED OUT, NO ACTION";
        statusClearTimer.start();
        userSetPointSet = false;
        actionPending = false;
    }

signals:
    void metricWidthChanged(double width);

protected slots:
    void renderOverlay();
    void drawGPS(QPainter &painter);
    void drawObjects(QPainter &painter);
    void drawPositionDirection(float xRef, float yRef, float radius, const QColor& color, QPainter* painter);
    void drawAttitudeDirection(float xRef, float yRef, float radius, const QColor& color, QPainter* painter);
    void drawAltitudeSetpoint(float xRef, float yRef, float radius, const QColor& color, QPainter* painter);
    /** @brief Draw a status flag indicator */
    void drawStatusFlag(float x, float y, QString label, bool status, bool known, QPainter& painter);
    /** @brief Draw a position lock indicator */
    void drawPositionLock(float x, float y, QString label, int status, bool known, QPainter& painter);
    void setBodySetpointCoordinateXY(double x, double y);
    void setBodySetpointCoordinateYaw(double yaw);
    void setBodySetpointCoordinateZ(double z);
    /** @brief Send the current ui setpoint coordinates as new setpoint to the MAV */
    void sendBodySetPointCoordinates();
    /** @brief Draw one setpoint */
    void drawSetpointXYZYaw(float x, float y, float z, float yaw, const QColor &color, QPainter &painter);
    /** @brief Draw waypoints of this system */
    void drawWaypoints(QPainter& painter);
    /** @brief Draw the limiting safety area */
    void drawSafetyArea(const QPointF &topLeft, const QPointF &bottomRight,  const QColor &color, QPainter &painter);
    /** @brief Receive mouse clicks */
    void mouseDoubleClickEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    /** @brief Receive mouse wheel events */
    void wheelEvent(QWheelEvent* event);
    /** @brief Read out send keys */
    void keyPressEvent(QKeyEvent* event);
    /** @brief Ignore context menu event */
    void contextMenuEvent (QContextMenuEvent* event);
    /** @brief Set status message on screen */
    void setStatusMessage(const QString& message)
    {
        statusMessage = message;
        statusClearTimer.start();
    }

protected:

    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    /** @brief Get color from GPS signal-to-noise colormap */
    static QColor getColorForSNR(float snr);
    /** @brief Metric world coordinates to metric body coordinates */
    QPointF metricWorldToBody(QPointF world);
    /** @brief Metric body coordinates to metric world coordinates */
    QPointF metricBodyToWorld(QPointF body);
    /** @brief Screen coordinates of widget to metric coordinates in body frame */
    QPointF screenToMetricBody(QPointF ref);
    /** @brief Reference coordinates to metric coordinates */
    QPointF refToMetricBody(QPointF &ref);
    /** @brief Metric coordinates to reference coordinates */
    QPointF metricBodyToRef(QPointF &metric);
    /** @brief Metric length to reference coordinates */
    double metricToRef(double metric);
    /** @bried Reference coordinates to metric length */
    double refToMetric(double ref);
    /** @brief Metric body coordinates to screen coordinates */
    QPointF metricBodyToScreen(QPointF metric);
    QMap<int, QString> objectNames;
    QMap<int, int> objectTypes;
    QMap<int, float> objectQualities;
    QMap<int, float> objectBearings;
    QMap<int, float> objectDistances;
    bool dragStarted;
    bool leftDragStarted;
    bool mouseHasMoved;
    float startX;
    float startY;
    QTimer statusClearTimer;
    QString statusMessage;
    bool actionPending;

    /**
     * @brief Private data container class to be used within the HSI widget
     */
    class GPSSatellite
    {
    public:
        GPSSatellite(int id, float elevation, float azimuth, float snr, bool used) :
            id(id),
            elevation(elevation),
            azimuth(azimuth),
            snr(snr),
            used(used),
            lastUpdate(MG::TIME::getGroundTimeNowUsecs()) {

        }

        void update(int id, float elevation, float azimuth, float snr, bool used) {
            this->id = id;
            this->elevation = elevation;
            this->azimuth = azimuth;
            this->snr = snr;
            this->used = used;
            this->lastUpdate = MG::TIME::getGroundTimeNowUsecs();
        }

        int id;
        float elevation;
        float azimuth;
        float snr;
        bool used;
        quint64 lastUpdate;

        friend class HSIDisplay;
    };

    QMap<int, GPSSatellite*> gpsSatellites;
    unsigned int satellitesUsed;

    // Current controller values
    float attXSet;
    float attYSet;
    float attYawSet;
    float altitudeSet;

    float posXSet;
    float posYSet;
    float posZSet;

    // Controller saturation values
    float attXSaturation;
    float attYSaturation;
    float attYawSaturation;

    float posXSaturation;
    float posYSaturation;
    float altitudeSaturation;

    // Position
    float lat;
    float lon;
    float alt;
    quint64 globalAvailable;   ///< Last global position update time
    float x;
    float y;
    float z;
    float vx;
    float vy;
    float vz;
    float speed;
    quint64 localAvailable;    ///< Last local position update time
    float roll;
    float pitch;
    float yaw;
    float bodyXSetCoordinate;  ///< X Setpoint coordinate active on the MAV
    float bodyYSetCoordinate;  ///< Y Setpoint coordinate active on the MAV
    float bodyZSetCoordinate;  ///< Z Setpoint coordinate active on the MAV
    float bodyYawSet;          ///< Yaw setpoint coordinate active on the MAV
    float uiXSetCoordinate;    ///< X Setpoint coordinate wanted by the UI
    float uiYSetCoordinate;    ///< Y Setpoint coordinate wanted by the UI
    float uiZSetCoordinate;    ///< Z Setpoint coordinate wanted by the UI
    float uiYawSet;            ///< Yaw Setpoint wanted by the UI
    double metricWidth;        ///< Width the instrument represents in meters (the width of the ground shown by the widget)

    //
    float xCenterPos;         ///< X center of instrument in virtual coordinates
    float yCenterPos;         ///< Y center of instrument in virtual coordinates

    bool positionLock;
    bool attControlEnabled;   ///< Attitude control enabled
    bool xyControlEnabled;    ///< Horizontal control enabled
    bool zControlEnabled;     ///< Vertical control enabled
    bool yawControlEnabled;   ///< Yaw angle position control enabled
    int positionFix;          ///< Total dimensions the MAV is localizaed in
    int gpsFix;               ///< Localization dimensions based on GPS
    int visionFix;            ///< Localizaiton dimensions based on computer vision
    int laserFix;             ///< Localization dimensions based on laser
    int iruFix;               ///< Localization dimensions based on ultrasound
    bool mavInitialized;      ///< The MAV is initialized once the setpoint has been received
    float bottomMargin;       ///< Margin on the bottom of the page, in virtual coordinates
    float topMargin;          ///< Margin on top of the page, in virtual coordinates

    bool attControlKnown;     ///< Attitude control status known flag
    bool xyControlKnown;      ///< XY control status known flag
    bool zControlKnown;       ///< Z control status known flag
    bool yawControlKnown;     ///< Yaw control status known flag

    // Position lock indicators
    bool positionFixKnown;    ///< Position fix status known flag
    bool visionFixKnown;      ///< Vision fix status known flag
    bool gpsFixKnown;         ///< GPS fix status known flag
    bool iruFixKnown;         ///< Infrared/Ultrasound fix status known flag

    // Data indicators
    bool setPointKnown;       ///< Controller setpoint known status flag
    bool positionSetPointKnown; ///< Position setpoint known status flag
    bool userSetPointSet;     ///< User set X, Y and Z
    bool userXYSetPointSet;   ///< User set the X/Y position already

private:
};

#endif // HSIDISPLAY_H
