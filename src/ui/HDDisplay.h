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
 *   @brief Definition of Head Down Display (HDD)
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#ifndef HDDISPLAY_H
#define HDDISPLAY_H

#include <QtGui/QGraphicsView>
#include <QColor>
#include <QTimer>
#include <QFontDatabase>
#include <QMap>
#include <QContextMenuEvent>
#include <QPair>
#include <cmath>

#include "UASInterface.h"

namespace Ui
{
class HDDisplay;
}

/**
 * @brief Head Down Display Widget
 *
 * This widget is used for any head down display as base widget. It handles the basic widget setup
 * each head down instrument has a virtual screen size in millimeters as base coordinate system
 * this virtual screen size is then scaled to pixels on the screen.
 * When the pixel per millimeter ratio is known, a 1:1 representation is possible on the screen
 */
class HDDisplay : public QGraphicsView
{
    Q_OBJECT
public:
    HDDisplay(QStringList* plotList, QString title="", QWidget *parent = 0);
    ~HDDisplay();

public slots:
    /** @brief Update a HDD double value */
    void updateValue(const int uasId, const QString& name, const QString& unit, const double value, const quint64 msec);
    /** @brief Update a HDD integer value */
    void updateValue(const int uasId, const QString& name, const QString& unit, const int value, const quint64 msec);
    /** @brief Update a HDD integer value */
    void updateValue(const int uasId, const QString& name, const QString& unit, const unsigned int value, const quint64 msec);
    /** @brief Update a HDD integer value */
    void updateValue(const int uasId, const QString& name, const QString& unit, const qint64 value, const quint64 msec);
    /** @brief Update a HDD integer value */
    void updateValue(const int uasId, const QString& name, const QString& unit, const quint64 value, const quint64 msec);
    virtual void setActiveUAS(UASInterface* uas);
    void addSource(QObject* obj);

    /** @brief Removes a plot item by the action data */
    void removeItemByAction();
    /** @brief Bring up the menu to add a gauge */
    void addGauge();
    /** @brief Add a gauge using this spec string */
    void addGauge(const QString& gauge);
    /** @brief Set the title of this widget and any existing parent dock widget */
    void setTitle();
    /** @brief Set the number of colums via popup */
    void setColumns();
    /** @brief Set the number of colums */
    void setColumns(int cols);
    /** @brief Save the current layout and state to disk */
    void saveState();
    /** @brief Restore the last layout and state from disk */
    void restoreState();

protected slots:
    void enableGLRendering(bool enable);
    //void render(QPainter* painter, const QRectF& target = QRectF(), const QRect& source = QRect(), Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);
    void renderOverlay();
    void triggerUpdate();
    /** @brief Adjust the size hint for the current gauge layout */
    void adjustGaugeAspectRatio();

protected:
    QSize sizeHint() const;
    void changeEvent(QEvent* e);
    void paintEvent(QPaintEvent* event);
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);
    void contextMenuEvent(QContextMenuEvent* event);
    QList<QAction*> getItemRemoveActions();
    void createActions();
    float refLineWidthToPen(float line);
    float refToScreenX(float x);
    float refToScreenY(float y);
    float screenToRefX(float x);
    float screenToRefY(float y);
    void rotatePolygonClockWiseRad(QPolygonF& p, float angle, QPointF origin);
    void drawPolygon(QPolygonF refPolygon, QPainter* painter);
    void drawLine(float refX1, float refY1, float refX2, float refY2, float width, const QColor& color, QPainter* painter);
    void drawEllipse(float refX, float refY, float radiusX, float radiusY, float lineWidth, const QColor& color, QPainter* painter);
    void drawCircle(float refX, float refY, float radius, float lineWidth, const QColor& color, QPainter* painter);

    void drawChangeRateStrip(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter);
    void drawChangeIndicatorGauge(float xRef, float yRef, float radius, float expectedMaxChange, float value, const QColor& color, QPainter* painter, bool solid=true);
    void drawGauge(float xRef, float yRef, float radius, float min, float max, const QString name, float value, const QColor& color, QPainter* painter, bool symmetric, QPair<float, float> goodRange, QPair<float, float> criticalRange, bool solid=true);
    void drawSystemIndicator(float xRef, float yRef, int maxNum, float maxWidth, float maxHeight, QPainter* painter);
    void paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter);

//    //Holds the current centerpoint for the view, used for panning and zooming
//     QPointF currentCenterPoint;
//
//     //From panning the view
//     QPoint lastPanPoint;
//
//     //Set the current centerpoint in the
//     void setCenter(const QPointF& centerPoint);
//     QPointF getCenter() { return currentCenterPoint; }
//
//     //Take over the interaction
//     virtual void mousePressEvent(QMouseEvent* event);
//     virtual void mouseReleaseEvent(QMouseEvent* event);
//     virtual void mouseMoveEvent(QMouseEvent* event);
//     virtual void wheelEvent(QWheelEvent* event);
//     virtual void resizeEvent(QResizeEvent* event);

    UASInterface* uas;                 ///< The uas currently monitored
    QMap<QString, double> values;       ///< The variables this HUD displays
    QMap<QString, QString> units;      ///< The units
    QMap<QString, float> valuesDot;    ///< First derivative of the variable
    QMap<QString, float> valuesMean;   ///< Mean since system startup for this variable
    QMap<QString, int> valuesCount;    ///< Number of values received so far
    QMap<QString, quint64> lastUpdate; ///< The last update time for this variable
    QMap<QString, float> minValues;    ///< The minimum value this variable is assumed to have
    QMap<QString, float> maxValues;    ///< The maximum value this variable is assumed to have
    QMap<QString, bool> symmetric;     ///< Draw the gauge / dial symmetric bool = yes
    QMap<QString, bool> intValues;     ///< Is the gauge value an integer?
    QMap<QString, QString> customNames; ///< Custom names for the data names
    QMap<QString, QPair<float, float> > goodRanges; ///< The range of good values
    QMap<QString, QPair<float, float> > critRanges; ///< The range of critical values
    double scalingFactor;      ///< Factor used to scale all absolute values to screen coordinates
    float xCenterOffset, yCenterOffset; ///< Offset from center of window in mm coordinates
    float vwidth;              ///< Virtual width of this window, 200 mm per default. This allows to hardcode positions and aspect ratios. This virtual image plane is then scaled to the window size.
    float vheight;             ///< Virtual height of this window, 150 mm per default

    int xCenter;               ///< Center of the HUD instrument in pixel coordinates. Allows to off-center the whole instrument in its OpenGL window, e.g. to fit another instrument
    int yCenter;               ///< Center of the HUD instrument in pixel coordinates. Allows to off-center the whole instrument in its OpenGL window, e.g. to fit another instrument

    // HUD colors
    QColor backgroundColor;    ///< Background color
    QColor defaultColor;       ///< Color for most HUD elements, e.g. pitch lines, center cross, change rate gauges
    QColor setPointColor;      ///< Color for the current control set point, e.g. yaw desired
    QColor warningColor;       ///< Color for warning messages
    QColor criticalColor;      ///< Color for caution messages
    QColor infoColor;          ///< Color for normal/default messages
    QColor fuelColor;          ///< Current color for the fuel message, can be info, warning or critical color

    // Blink rates
    int warningBlinkRate;      ///< Blink rate of warning messages, will be rounded to the refresh rate

    QTimer* refreshTimer;      ///< The main timer, controls the update rate
    static const int updateInterval = 300; ///< Update interval in milliseconds
    QPainter* hudPainter;
    QFont font;                ///< The HUD font, per default the free Bitstream Vera SANS, which is very close to actual HUD fonts
    QFontDatabase fontDatabase;///< Font database, only used to load the TrueType font file (the HUD font is directly loaded from file rather than from the system)
    bool hardwareAcceleration; ///< Enable hardware acceleration

    float strongStrokeWidth;   ///< Strong line stroke width, used throughout the HUD
    float normalStrokeWidth;   ///< Normal line stroke width, used throughout the HUD
    float fineStrokeWidth;     ///< Fine line stroke width, used throughout the HUD

    QStringList* acceptList;       ///< Variable names to plot
    QStringList* acceptUnitList;   ///< Unit names to plot

    quint64 lastPaintTime;     ///< Last time this widget was refreshed
    int columns;               ///< Number of instrument columns

    QAction* addGaugeAction;   ///< Action adding a gauge
    QAction* setTitleAction;   ///< Action setting the title
    QAction* setColumnsAction; ///< Action setting the number of columns
    bool valuesChanged;

private:
    Ui::HDDisplay *m_ui;
};

#endif // HDDISPLAY_H
