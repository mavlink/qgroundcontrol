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
 *   @brief Definition of Head up display
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef HUD_H
#define HUD_H

#include <QImage>
#include <QGLWidget>
#include <QPainter>
#include <QFontDatabase>
#include <QTimer>
#include "UASInterface.h"

/**
 * @brief Displays a Head Up Display (HUD)
 *
 * This class represents a head up display (HUD) and draws this HUD in an OpenGL widget (QGLWidget).
 * It can superimpose the HUD over the current live image stream (any arriving image stream will be auto-
 * matically used as background), or it draws the classic blue-brown background known from instruments.
 */
class HUD : public QGLWidget
{
    Q_OBJECT
public:
    HUD(int width = 640, int height = 480, QWidget* parent = NULL);
    ~HUD();

    void setImageSize(int width, int height, int depth, int channels);
    void resizeGL(int w, int h);

public slots:
    void initializeGL();
    //void paintGL();

    /** @brief Set the currently monitored UAS */
    void setActiveUAS(UASInterface* uas);

    void updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp);
//    void updateAttitudeThrustSetPoint(UASInterface*, double rollDesired, double pitchDesired, double yawDesired, double thrustDesired, quint64 usec);
    void updateBattery(UASInterface*, double, double, int);
    void receiveHeartbeat(UASInterface*);
    void updateThrust(UASInterface*, double);
    void updateLocalPosition(UASInterface*,double,double,double,quint64);
    void updateGlobalPosition(UASInterface*,double,double,double,quint64);
    void updateSpeed(UASInterface*,double,double,double,quint64);
    void updateState(UASInterface*,QString);
    void updateMode(int id,QString mode, QString description);
    void updateLoad(UASInterface*, double);
    void selectWaypoint(int uasId, int id);

    void startImage(quint64 timestamp);
    void startImage(int imgid, int width, int height, int depth, int channels);
    void setPixels(int imgid, const unsigned char* imageData, int length, int startIndex);
    void finishImage();
    void saveImage();
    void saveImage(QString fileName);
    /** @brief Select directory where to load the offline files from */
    void selectOfflineDirectory();
    /** @brief Enable the HUD instruments */
    void enableHUDInstruments(bool enabled);
    /** @brief Enable Video */
    void enableVideo(bool enabled);
    /** @brief Copy an image from the current active UAS */
    void copyImage();


protected slots:
    void paintCenterBackground(float roll, float pitch, float yaw);
    void paintRollPitchStrips();
    void paintPitchLines(float pitch, QPainter* painter);
    /** @brief Paint text on top of the image and OpenGL drawings */
    void paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter);
    /** @brief Setup the OpenGL view for drawing a sub-component of the HUD */
    void setupGLView(float referencePositionX, float referencePositionY, float referenceWidth, float referenceHeight);
    void paintHUD();
    void paintPitchLinePos(QString text, float refPosX, float refPosY, QPainter* painter);
    void paintPitchLineNeg(QString text, float refPosX, float refPosY, QPainter* painter);

    void drawLine(float refX1, float refY1, float refX2, float refY2, float width, const QColor& color, QPainter* painter);
    void drawEllipse(float refX, float refY, float radiusX, float radiusY, float startDeg, float endDeg, float lineWidth, const QColor& color, QPainter* painter);
    void drawCircle(float refX, float refY, float radius, float startDeg, float endDeg, float lineWidth, const QColor& color, QPainter* painter);

    void drawChangeRateStrip(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter);
    void drawChangeIndicatorGauge(float xRef, float yRef, float radius, float expectedMaxChange, float value, const QColor& color, QPainter* painter, bool solid=true);

    void drawPolygon(QPolygonF refPolygon, QPainter* painter);

protected:
    void commitRawDataToGL();
    /** @brief Convert reference coordinates to screen coordinates */
    float refToScreenX(float x);
    /** @brief Convert reference coordinates to screen coordinates */
    float refToScreenY(float y);
    /** @brief Convert mm line widths to QPen line widths */
    float refLineWidthToPen(float line);
    /** @brief Rotate a polygon around a point clockwise */
    void rotatePolygonClockWiseRad(QPolygonF& p, float angle, QPointF origin);
    /** @brief Preferred Size */
    QSize sizeHint() const;
    /** @brief Start updating widget */
    void showEvent(QShowEvent* event);
    /** @brief Stop updating widget */
    void hideEvent(QHideEvent* event);
    void contextMenuEvent (QContextMenuEvent* event);
    void createActions();

    static const int updateInterval = 40;

    QImage* image; ///< Double buffer image
    QImage glImage; ///< The background / camera image
    UASInterface* uas; ///< The uas currently monitored
    float yawInt; ///< The yaw integral. Used to damp the yaw indication.
    QString mode; ///< The current vehicle mode
    QString state; ///< The current vehicle state
    QString fuelStatus; ///< Current fuel level / battery voltage
    double scalingFactor; ///< Factor used to scale all absolute values to screen coordinates
    float xCenterOffset, yCenterOffset; ///< Offset from center of window in mm coordinates
    float vwidth; ///< Virtual width of this window, 200 mm per default. This allows to hardcode positions and aspect ratios. This virtual image plane is then scaled to the window size.
    float vheight; ///< Virtual height of this window, 150 mm per default
    float vGaugeSpacing; ///< Virtual spacing of the gauges from the center, 50 mm per default
    float vPitchPerDeg; ///< Virtual pitch to mm conversion. Currently one degree is 3 mm up/down in the pitch markings

    int xCenter; ///< Center of the HUD instrument in pixel coordinates. Allows to off-center the whole instrument in its OpenGL window, e.g. to fit another instrument
    int yCenter; ///< Center of the HUD instrument in pixel coordinates. Allows to off-center the whole instrument in its OpenGL window, e.g. to fit another instrument

    // Image buffers
    unsigned char* rawBuffer1; ///< Double buffer 1 for the image
    unsigned char* rawBuffer2; ///< Double buffer 2 for the image
    unsigned char* rawImage;   ///< Pointer to current complete image
    int rawLastIndex;          ///< The last byte index received of the image
    int rawExpectedBytes;      ///< Number of raw image bytes expected. Calculated by: image depth * channels * widht * height / 8
    int bytesPerLine;          ///< Bytes per image line. Is calculated as: image depth * channels * width / 8
    bool imageStarted;         ///< If an image is currently in transmission
    int receivedDepth;         ///< Image depth in bit for the current image
    int receivedChannels;      ///< Number of color channels
    int receivedWidth;         ///< Width in pixels of the current image
    int receivedHeight;        ///< Height in pixels of the current image

    // HUD colors
    QColor defaultColor;       ///< Color for most HUD elements, e.g. pitch lines, center cross, change rate gauges
    QColor setPointColor;      ///< Color for the current control set point, e.g. yaw desired
    QColor warningColor;       ///< Color for warning messages
    QColor criticalColor;      ///< Color for caution messages
    QColor infoColor;          ///< Color for normal/default messages
    QColor fuelColor;          ///< Current color for the fuel message, can be info, warning or critical color

    // Blink rates
    int warningBlinkRate;      ///< Blink rate of warning messages, will be rounded to the refresh rate

    QTimer* refreshTimer;      ///< The main timer, controls the update rate
    QPainter* hudPainter;
    QFont font;                ///< The HUD font, per default the free Bitstream Vera SANS, which is very close to actual HUD fonts
    QFontDatabase fontDatabase;///< Font database, only used to load the TrueType font file (the HUD font is directly loaded from file rather than from the system)
    bool noCamera;             ///< No camera images available, draw the ground/sky box to indicate the horizon
    bool hardwareAcceleration; ///< Enable hardware acceleration

    float strongStrokeWidth;   ///< Strong line stroke width, used throughout the HUD
    float normalStrokeWidth;   ///< Normal line stroke width, used throughout the HUD
    float fineStrokeWidth;     ///< Fine line stroke width, used throughout the HUD

    QString waypointName;      ///< Waypoint name displayed in HUD
    float roll;
    float pitch;
    float yaw;
    float rollLP;
    float pitchLP;
    float yawLP;
    double yawDiff;
    double xPos;
    double yPos;
    double zPos;
    double xSpeed;
    double ySpeed;
    double zSpeed;
    quint64 lastSpeedUpdate;
    double totalSpeed;
    double totalAcc;
    double lat;
    double lon;
    double alt;
    float load;
    QString offlineDirectory;
    QString nextOfflineImage;
    bool hudInstrumentsEnabled;
    bool videoEnabled;
    float xImageFactor;
    float yImageFactor;
    QAction* enableHUDAction;
    QAction* enableVideoAction;
    QAction* selectOfflineDirectoryAction;
    QAction* selectVideoChannelAction;
    void paintEvent(QPaintEvent *event);
    bool imageRequested;
    UAS* u;

};

#endif // HUD_H
