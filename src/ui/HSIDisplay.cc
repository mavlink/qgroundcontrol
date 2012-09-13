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
 *   @brief Implementation of Horizontal Situation Indicator class
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QFile>
#include <QStringList>
#include <QPainter>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include "UASManager.h"
#include "HSIDisplay.h"
#include "QGC.h"
#include "Waypoint.h"
#include "UASWaypointManager.h"
#include <qmath.h>
//#include "Waypoint2DIcon.h"
#include "MAV2DIcon.h"

#include <QDebug>


HSIDisplay::HSIDisplay(QWidget *parent) :
    HDDisplay(NULL, "HSI", parent),
    dragStarted(false),
    leftDragStarted(false),
    mouseHasMoved(false),
    startX(0.0f),
    startY(0.0f),
    actionPending(false),
    directSending(false),
    gpsSatellites(),
    satellitesUsed(0),
    attXSet(0.0f),
    attYSet(0.0f),
    attYawSet(0.0f),
    altitudeSet(1.0f),
    posXSet(0.0f),
    posYSet(0.0f),
    posZSet(0.0f),
    attXSaturation(0.2f),
    attYSaturation(0.2f),
    attYawSaturation(0.5f),
    posXSaturation(0.05f),
    posYSaturation(0.05f),
    altitudeSaturation(1.0f),
    lat(0.0f),
    lon(0.0f),
    alt(0.0f),
    globalAvailable(0),
    x(0.0f),
    y(0.0f),
    z(0.0f),
    vx(0.0f),
    vy(0.0f),
    vz(0.0f),
    speed(0.0f),
    localAvailable(0),
    roll(0.0f),
    pitch(0.0f),
    yaw(0.0f),
    bodyXSetCoordinate(0.0f),
    bodyYSetCoordinate(0.0f),
    bodyZSetCoordinate(0.0f),
    bodyYawSet(0.0f),
    uiXSetCoordinate(0.0f),
    uiYSetCoordinate(0.0f),
    uiZSetCoordinate(0.0f),
    uiYawSet(0.0f),
    metricWidth(4.0),
    xCenterPos(0),
    yCenterPos(0),
    positionLock(false),
    attControlEnabled(false),
    xyControlEnabled(false),
    zControlEnabled(false),
    yawControlEnabled(false),
    positionFix(0),
    gpsFix(0),
    visionFix(0),
    laserFix(0),
    iruFix(0),
    mavInitialized(false),
    topMargin(12.0f),
    bottomMargin(10.0f),
    attControlKnown(false),
    xyControlKnown(false),
    zControlKnown(false),
    yawControlKnown(false),
    positionFixKnown(false),
    visionFixKnown(false),
    gpsFixKnown(false),
    iruFixKnown(false),
    setPointKnown(false),
    positionSetPointKnown(false),
    userSetPointSet(false),
    userXYSetPointSet(false),
    userZSetPointSet(false),
    userYawSetPointSet(false)
{
    refreshTimer->setInterval(updateInterval);

    columns = 1;
    this->setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(backgroundRole(), QGC::colorBlack);
    setPalette(pal);

    vwidth = 80.0f;
    vheight = 80.0f;

    xCenterPos = vwidth/2.0f;
    yCenterPos = vheight/2.0f + topMargin - bottomMargin;

    uas = NULL;
    resetMAVState();

    // Do first update
    setMetricWidth(metricWidth);
    // Set tooltip
    setToolTip(tr("View from top in body frame. Scroll with mouse wheel to change the horizontal field of view of the widget."));
    setStatusTip(tr("View from top in body frame. Scroll with mouse wheel to change the horizontal field of view of the widget."));

    connect(&statusClearTimer, SIGNAL(timeout()), this, SLOT(clearStatusMessage()));
    statusClearTimer.start(3000);

    setFocusPolicy(Qt::StrongFocus);
}

HSIDisplay::~HSIDisplay()
{

}

void HSIDisplay::resetMAVState()
{
    mavInitialized = false;
    attControlKnown = false;
    attControlEnabled = false;
    xyControlKnown = false;
    xyControlEnabled = false;
    zControlKnown = false;
    zControlEnabled = false;
    yawControlKnown = false;
    yawControlEnabled = false;

    // Draw position lock indicators
    positionFixKnown = false;
    positionFix = 0;
    visionFixKnown = false;
    visionFix = 0;
    gpsFixKnown = false;
    gpsFix = 0;
    iruFixKnown = false;
    iruFix = 0;

    // Data
    setPointKnown = false;
    localAvailable = 0;
    globalAvailable = 0;

    // Setpoints
    positionSetPointKnown = false;
    setPointKnown = false;
}

void HSIDisplay::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event);
    //paintGL();
    //    static quint64 interval = 0;
    //    //qDebug() << "INTERVAL:" << MG::TIME::getGroundTimeNow() - interval << __FILE__ << __LINE__;
    //    interval = MG::TIME::getGroundTimeNow();
    renderOverlay();
}

void HSIDisplay::renderOverlay()
{
    if (!isVisible()) return;
#if (QGC_EVENTLOOP_DEBUG)
    qDebug() << "EVENTLOOP:" << __FILE__ << __LINE__;
#endif
    // Center location of the HSI gauge items

    //float bottomMargin = 3.0f;

    // Size of the ring instrument
    //const float margin = 0.1f;  // 10% margin of total width on each side
    float baseRadius = (vheight - topMargin - bottomMargin) / 2.0f - bottomMargin / 2.0f;

    // Draw instruments
    // TESTING THIS SHOULD BE MOVED INTO A QGRAPHICSVIEW
    // Update scaling factor
    // adjust scaling to fit both horizontally and vertically
    scalingFactor = this->width()/vwidth;
    double scalingFactorH = this->height()/vheight;
    if (scalingFactorH < scalingFactor) scalingFactor = scalingFactorH;

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

    // Draw base instrument
    // ----------------------
    painter.setBrush(Qt::NoBrush);
    const QColor ringColor = QColor(200, 250, 200);
    QPen pen;
    pen.setColor(ringColor);
    pen.setWidth(refLineWidthToPen(0.1f));
    painter.setPen(pen);
    const int ringCount = 2;
    for (int i = 0; i < ringCount; i++)
    {
        float radius = (vwidth - (topMargin + bottomMargin)*0.3f) / (1.35f * i+1) / 2.0f - bottomMargin / 2.0f;
        drawCircle(xCenterPos, yCenterPos, radius, 0.1f, ringColor, &painter);
        paintText(tr("%1 m").arg(refToMetric(radius), 5, 'f', 1, ' '), QGC::colorCyan, 1.6f, vwidth/2-4, vheight/2+radius+2.2, &painter);
    }

    // Draw orientation labels
    // Translate and rotate coordinate frame
    painter.translate((xCenterPos)*scalingFactor, (yCenterPos)*scalingFactor);
    const float yawDeg = ((yaw/M_PI)*180.0f);
    int yawRotate = static_cast<int>(yawDeg) % 360;
    painter.rotate(-yawRotate);
    paintText(tr("N"), ringColor, 3.5f, - 1.0f, - baseRadius - 5.5f, &painter);
    paintText(tr("S"), ringColor, 3.5f, - 1.0f, + baseRadius + 1.5f, &painter);
    paintText(tr("E"), ringColor, 3.5f, + baseRadius + 3.0f, - 1.25f, &painter);
    paintText(tr("W"), ringColor, 3.5f, - baseRadius - 5.5f, - 1.75f, &painter);
    painter.rotate(+yawRotate);
    painter.translate(-(xCenterPos)*scalingFactor, -(yCenterPos)*scalingFactor);

    // Draw trail
    //    QPointF m(bodyXSetCoordinate, bodyYSetCoordinate);
    //    // Transform from body to world coordinates
    //    m = metricWorldToBody(m);
    //    // Scale from metric body to screen reference units
    //    QPointF s = metricBodyToRef(m);
    //    drawLine(s.x(), s.y(), xCenterPos, yCenterPos, 1.5f, uas->getColor(), &painter);


    // Draw center indicator
    //    QPolygonF p(3);
    //    p.replace(0, QPointF(xCenterPos, yCenterPos-4.0f));
    //    p.replace(1, QPointF(xCenterPos-4.0f, yCenterPos+3.5f));
    //    p.replace(2, QPointF(xCenterPos+4.0f, yCenterPos+3.5f));
    //    drawPolygon(p, &painter);

    if (uas)
    {
        // Translate to center
        painter.translate((xCenterPos)*scalingFactor, (yCenterPos)*scalingFactor);
        QColor uasColor = uas->getColor();
        MAV2DIcon::drawAirframePolygon(uas->getAirframe(), painter, static_cast<int>((vwidth/4.0f)*scalingFactor*1.1f), uasColor, 0.0f);
        //MAV2DIcon::drawAirframePolygon(uas->getAirframe(), painter, static_cast<int>((vwidth/4.0f)*scalingFactor*1.1f), uasColor, 0.0f);
        // Translate back
        painter.translate(-(xCenterPos)*scalingFactor, -(yCenterPos)*scalingFactor);
    }

    // ----------------------

    // Draw satellites
    drawGPS(painter);

    // Draw state indicator

    // Draw position
    QColor positionColor(20, 20, 200);
    drawPositionDirection(xCenterPos, yCenterPos, baseRadius, positionColor, &painter);

    // Draw attitude
    QColor attitudeColor(200, 20, 20);
    drawAttitudeDirection(xCenterPos, yCenterPos, baseRadius, attitudeColor, &painter);


    // Draw position setpoints in body coordinates

    float xSpDiff = uiXSetCoordinate - bodyXSetCoordinate;
    float ySpDiff = uiYSetCoordinate - bodyYSetCoordinate;
    float zSpDiff = uiZSetCoordinate - bodyZSetCoordinate;

    float setPointDist = sqrt(xSpDiff*xSpDiff + ySpDiff*ySpDiff + zSpDiff*zSpDiff);

    float angleDiff = uiYawSet - bodyYawSet;

    float normAngleDiff = fabs(atan2(sin(angleDiff), cos(angleDiff)));

    //if (((userSetPointSet) || (normAngleDiff > 0.05f) || dragStarted) && !(setPointDist < 0.08f && mavInitialized))


    // Labels on outer part and bottom

    // Draw waypoints
    drawWaypoints(painter);

    // Draw status flags
    drawStatusFlag(2,  1, tr("ATT"), attControlEnabled, attControlKnown, painter);
    drawStatusFlag(22, 1, tr("PXY"), xyControlEnabled,  xyControlKnown,  painter);
    drawStatusFlag(44, 1, tr("PZ"),  zControlEnabled,   zControlKnown,   painter);
    drawStatusFlag(66, 1, tr("YAW"), yawControlEnabled, yawControlKnown, painter);

    // Draw position lock indicators
    drawPositionLock(2,  5, tr("POS"), positionFix, positionFixKnown, painter);
    drawPositionLock(22, 5, tr("VIS"), visionFix,   visionFixKnown,   painter);
    drawPositionLock(44, 5, tr("GPS"), gpsFix,      gpsFixKnown,      painter);
    drawPositionLock(66, 5, tr("IRU"), iruFix,      iruFixKnown,      painter);

    // Draw speed to top left
    paintText(tr("SPEED"), QGC::colorCyan, 2.2f, 2, 11, &painter);
    paintText(tr("%1 m/s").arg(speed, 5, 'f', 2, '0'), Qt::white, 2.2f, 12, 11, &painter);

    // Draw crosstrack error to top right
    float crossTrackError = 0;
    paintText(tr("XTRACK"), QGC::colorCyan, 2.2f, 54, 11, &painter);
    paintText(tr("%1 m").arg(crossTrackError, 5, 'f', 2, '0'), Qt::white, 2.2f, 67, 11, &painter);

    // Draw position to bottom left
    if (localAvailable > 0)
    {
        // Position
        QString str;
        float offset = (globalAvailable > 0) ? -3.0f : 0.0f;
        str.sprintf("%05.2f %05.2f %05.2f m", x, y, z);
        paintText(tr("POS"), QGC::colorCyan, 2.6f, 2, vheight - offset - 2.0f, &painter);
        paintText(str, Qt::white, 2.6f, 10, vheight - offset - 2.0f, &painter);
    }

    if (globalAvailable > 0)
    {
        // Position
        QString str;
        str.sprintf("lat: %05.2f lon: %06.2f alt: %06.2f", lat, lon, alt);
        paintText(tr("GPS"), QGC::colorCyan, 2.6f, 2, vheight- 2.0f, &painter);
        paintText(str, Qt::white, 2.6f, 10, vheight - 2.0f, &painter);
    }

    // Draw Safety
    double x1, y1, z1, x2, y2, z2;
    UASManager::instance()->getLocalNEDSafetyLimits(&x1, &y1, &z1, &x2, &y2, &z2);
    //    drawSafetyArea(QPointF(x1, y1), QPointF(x2, y2), QGC::colorYellow, painter);

    // Draw status message
    paintText(statusMessage, QGC::colorOrange, 2.8f, 8, 15, &painter);

    // Draw setpoint over waypoints
    if (positionSetPointKnown || setPointKnown)
    {
        // Draw setpoint
        drawSetpointXYZYaw(bodyXSetCoordinate, bodyYSetCoordinate, bodyZSetCoordinate, bodyYawSet, QGC::colorYellow, painter);
        // Draw travel direction line
        QPointF m(bodyXSetCoordinate, bodyYSetCoordinate);
        // Transform from body to world coordinates
        m = metricWorldToBody(m);
        // Scale from metric body to screen reference units
        QPointF s = metricBodyToRef(m);
        drawLine(s.x(), s.y(), xCenterPos, yCenterPos, 1.5f, QGC::colorYellow, &painter);
    }

    if ((userSetPointSet || dragStarted) && ((normAngleDiff > 0.05f) || !(setPointDist < 0.08f && mavInitialized)))
    {
        QColor spColor(150, 250, 150);
        drawSetpointXYZYaw(uiXSetCoordinate, uiYSetCoordinate, uiZSetCoordinate, uiYawSet, spColor, painter);
    }
}

void HSIDisplay::drawStatusFlag(float x, float y, QString label, bool status, bool known, QPainter& painter)
{
    paintText(label, QGC::colorCyan, 2.6f, x, y+0.8f, &painter);
    QColor statusColor(250, 250, 250);
    if(status) {
        painter.setBrush(QGC::colorGreen);
    } else {
        painter.setBrush(QGC::colorDarkYellow);
    }
    painter.setPen(Qt::NoPen);

    float indicatorWidth = refToScreenX(7.0f);
    float indicatorHeight = refToScreenY(4.0f);

    painter.drawRect(QRect(refToScreenX(x+7.3f), refToScreenY(y+0.05), indicatorWidth, indicatorHeight));
    paintText((status) ? tr("ON") : tr("OFF"), statusColor, 2.6f, x+7.9f, y+0.8f, &painter);
    // Cross out instrument if state unknown
    if (!known)
    {
        QPen pen(Qt::yellow);
        pen.setWidth(2);
        painter.setPen(pen);
        // Top left to bottom right
        QPointF p1, p2, p3, p4;
        p1.setX(refToScreenX(x));
        p1.setY(refToScreenX(y));
        p2.setX(p1.x()+indicatorWidth+refToScreenX(7.3f));
        p2.setY(p1.y()+indicatorHeight);
        painter.drawLine(p1, p2);
        // Bottom left to top right
        p3.setX(refToScreenX(x));
        p3.setY(refToScreenX(y)+indicatorHeight);
        p4.setX(p1.x()+indicatorWidth+refToScreenX(7.3f));
        p4.setY(p1.y());
        painter.drawLine(p3, p4);
    }
}

void HSIDisplay::drawPositionLock(float x, float y, QString label, int status, bool known, QPainter& painter)
{
    paintText(label, QGC::colorCyan, 2.6f, x, y+0.8f, &painter);
    QColor negStatusColor(200, 20, 20);
    QColor intermediateStatusColor (Qt::yellow);
    QColor posStatusColor(20, 200, 20);
    QColor statusColor(250, 250, 250);
    if (status == 3) {
        painter.setBrush(posStatusColor);
    } else if (status == 2) {
        painter.setBrush(intermediateStatusColor.dark(150));
    } else {
        painter.setBrush(negStatusColor);
    }

    // Lock text
    QString lockText;
    switch (status) {
    case 1:
        lockText = tr("LOC");
        break;
    case 2:
        lockText = tr("2D");
        break;
    case 3:
        lockText = tr("3D");
        break;
    default:
        lockText = tr("NO");
        break;
    }

    float indicatorWidth = refToScreenX(7.0f);
    float indicatorHeight = refToScreenY(4.0f);

    painter.setPen(Qt::NoPen);
    painter.drawRect(QRect(refToScreenX(x+7.3f), refToScreenY(y+0.05), refToScreenX(7.0f), refToScreenY(4.0f)));
    paintText(lockText, statusColor, 2.6f, x+7.9f, y+0.8f, &painter);
    // Cross out instrument if state unknown
    if (!known) {
        QPen pen(Qt::yellow);
        pen.setWidth(2);
        painter.setPen(pen);
        // Top left to bottom right
        QPointF p1, p2, p3, p4;
        p1.setX(refToScreenX(x));
        p1.setY(refToScreenX(y));
        p2.setX(p1.x()+indicatorWidth+refToScreenX(7.3f));
        p2.setY(p1.y()+indicatorHeight);
        painter.drawLine(p1, p2);
        // Bottom left to top right
        p3.setX(refToScreenX(x));
        p3.setY(refToScreenX(y)+indicatorHeight);
        p4.setX(p1.x()+indicatorWidth+refToScreenX(7.3f));
        p4.setY(p1.y());
        painter.drawLine(p3, p4);
    }
}

void HSIDisplay::updatePositionLock(UASInterface* uas, bool lock)
{
    Q_UNUSED(uas);
    bool changed = positionLock != lock;
    positionLock = lock;
    if (changed)
    {
        update();
    }
}

void HSIDisplay::updateAttitudeControllerEnabled(bool enabled)
{
    bool changed = attControlEnabled != enabled;
    attControlEnabled = enabled;
    attControlKnown = true;
    if (changed)
    {
        update();
    }
}

void HSIDisplay::updatePositionXYControllerEnabled(bool enabled)
{
    bool changed = xyControlEnabled != enabled;
    xyControlEnabled = enabled;
    xyControlKnown = true;
    if (changed)
    {
        update();
    }
}

void HSIDisplay::updatePositionZControllerEnabled(bool enabled)
{
    bool changed = zControlEnabled != enabled;
    zControlEnabled = enabled;
    zControlKnown = true;
    if (changed)
    {
        update();
    }
}

void HSIDisplay::updateObjectPosition(unsigned int time, int id, int type, const QString& name, int quality, float bearing, float distance)
{
    Q_UNUSED(quality);
    Q_UNUSED(name);
    Q_UNUSED(type);
    Q_UNUSED(id);
    Q_UNUSED(time);
    // FIXME add multi-object support
    QPainter painter(this);
    QColor color(Qt::yellow);
    float radius = vwidth / 20.0f;
    QPen pen(color);
    pen.setWidthF(refLineWidthToPen(0.4f));
    pen.setColor(color);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    QPointF in(cos(bearing)-sin(bearing)*distance, sin(bearing)+cos(bearing)*distance);
    // Scale from metric to screen reference coordinates
    QPointF p = metricBodyToRef(in);
    drawCircle(p.x(), p.y(), radius, 0.4f, color, &painter);
}

QPointF HSIDisplay::metricWorldToBody(QPointF world)
{
    // First translate to body-centered coordinates
    // Rotate around -yaw
    float angle = -yaw - M_PI;
    QPointF result(cos(angle) * (x - world.x()) - sin(angle) * (y - world.y()), sin(angle) * (x - world.x()) + cos(angle) * (y - world.y()));
    return result;
}

QPointF HSIDisplay::metricBodyToWorld(QPointF body)
{
    // First rotate into world coordinates
    // then translate to world position
    QPointF result((cos(-yaw) * body.x()) + (sin(-yaw) * body.y()) + x, (-sin(-yaw) * body.x()) + (cos(-yaw) * body.y()) + y);
    return result;
}

QPointF HSIDisplay::screenToMetricBody(QPointF ref)
{
    return QPointF(-((screenToRefY(ref.y()) - yCenterPos)/ vwidth) * metricWidth, ((screenToRefX(ref.x()) - xCenterPos) / vwidth) * metricWidth);
}

QPointF HSIDisplay::refToMetricBody(QPointF &ref)
{
    return QPointF(-((ref.y() - yCenterPos)/ vwidth) * metricWidth - x, ((ref.x() - xCenterPos) / vwidth) * metricWidth - y);
}

/**
 * @see refToScreenX()
 */
QPointF HSIDisplay::metricBodyToRef(QPointF &metric)
{
    return QPointF(((metric.y())/ metricWidth) * vwidth + xCenterPos, ((-metric.x()) / metricWidth) * vwidth + yCenterPos);
}

double HSIDisplay::metricToRef(double metric)
{
    return (metric / metricWidth) * vwidth;
}

double HSIDisplay::refToMetric(double ref)
{
    return (ref/vwidth) * metricWidth;
}

QPointF HSIDisplay::metricBodyToScreen(QPointF metric)
{
    QPointF ref = metricBodyToRef(metric);
    ref.setX(refToScreenX(ref.x()));
    ref.setY(refToScreenY(ref.y()));
    return ref;
}

void HSIDisplay::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (event->type() == QMouseEvent::MouseButtonDblClick)
    {
        QPointF p = screenToMetricBody(event->posF());
        if (!directSending)
        {
            setBodySetpointCoordinateXY(p.x(), p.y());
            if (!userZSetPointSet) setBodySetpointCoordinateZ(0.0);
        }
        //        qDebug() << "Double click at x: " << screenToRefX(event->x()) - xCenterPos << "y:" << screenToRefY(event->y()) - yCenterPos;
    }
}

void HSIDisplay::mouseReleaseEvent(QMouseEvent * event)
{
    // FIXME hardcode yaw to current value
    //setBodySetpointCoordinateYaw(0);
    if (mouseHasMoved)
    {
        if (event->type() == QMouseEvent::MouseButtonRelease && event->button() == Qt::RightButton)
        {
            if (dragStarted)
            {
                if (!directSending) setBodySetpointCoordinateYaw(uiYawSet);
                dragStarted = false;
            }
        }
        if (event->type() == QMouseEvent::MouseButtonRelease && event->button() == Qt::LeftButton)
        {
            if (leftDragStarted)
            {
                if (!directSending) setBodySetpointCoordinateZ(uiZSetCoordinate);
                leftDragStarted = false;
            }
        }
    }
    mouseHasMoved = false;
}

void HSIDisplay::mousePressEvent(QMouseEvent * event)
{
    if (event->type() == QMouseEvent::MouseButtonPress)
    {
        if (event->button() == Qt::RightButton)
        {
            startX = event->x();
            // Start tracking mouse move
            dragStarted = true;
        }
        else if (event->button() == Qt::LeftButton)
        {
            startY = event->y();
            leftDragStarted = true;
        }
    }
    mouseHasMoved = false;
}

void HSIDisplay::mouseMoveEvent(QMouseEvent * event)
{
    if (event->type() == QMouseEvent::MouseMove)
    {
        if (dragStarted) uiYawSet -= 0.06f*(startX - event->x()) / this->frameSize().width();

        if (leftDragStarted)
        {
            //            uiZSetCoordinate -= 0.06f*(startY - event->y()) / this->frameSize().height();
            //            setBodySetpointCoordinateZ(uiZSetCoordinate);
        }

        if (leftDragStarted || dragStarted) mouseHasMoved = true;
    }
}

void HSIDisplay::keyPressEvent(QKeyEvent* event)
{
    QPointF bodySP = metricWorldToBody(QPointF(uiXSetCoordinate, uiYSetCoordinate));

    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && actionPending)
    {
        actionPending = false;
        statusMessage = "SETPOINT SENT";
        statusClearTimer.start();
        sendBodySetPointCoordinates();
    }
    else
    {
        // FIXME hardcode yaw to current value
        setBodySetpointCoordinateYaw(0);


        // Reset setpoints to current position / orientation
        // if not initialized
        if (!userYawSetPointSet)
        {
            setBodySetpointCoordinateYaw(0);
        }

        if (!userZSetPointSet)
        {
            setBodySetpointCoordinateZ(0);
        }

        if (!userXYSetPointSet)
        {
            setBodySetpointCoordinateXY(0, 0);
        }

        if ((event->key() ==  Qt::Key_W))
        {
            setBodySetpointCoordinateXY(qBound(-1.5, bodySP.x()+0.2, +1.5), bodySP.y());
        }

        if ((event->key() ==  Qt::Key_S))
        {
            setBodySetpointCoordinateXY(qBound(-1.5, bodySP.x()-0.2, +1.5), bodySP.y());
        }

        if ((event->key() ==  Qt::Key_A))
        {
            setBodySetpointCoordinateXY(bodySP.x(), qBound(-1.5, bodySP.y()-0.2, +1.5));
        }

        if ((event->key() ==  Qt::Key_D))
        {
            setBodySetpointCoordinateXY(bodySP.x(), qBound(-1.5, bodySP.y()+0.2, +1.5));
        }

        if ((event->key() ==  Qt::Key_Up))
        {
            setBodySetpointCoordinateZ(-0.2);
        }

        if ((event->key() ==  Qt::Key_Down))
        {
            setBodySetpointCoordinateZ(+0.2);
        }

        if ((event->key() ==  Qt::Key_Left))
        {
            setBodySetpointCoordinateYaw(-0.2);
        }

        if ((event->key() ==  Qt::Key_Right))
        {
            setBodySetpointCoordinateYaw(0.2);
        }
    }


    // Overwrite any existing non-zero body setpoints
    // for escape
    if ((event->key() == Qt::Key_Escape) || (event->key() == Qt::Key_R))
    {
        setBodySetpointCoordinateXY(0, 0);
        setBodySetpointCoordinateZ(0);
        setBodySetpointCoordinateYaw(0);
        statusMessage = "CANCELLED, PRESS <ENTER> TO SEND";
    }

    if ((event->key() == Qt::Key_T))
    {
        directSending = !directSending;
        statusMessage = (directSending) ? "DIRECT SEND ON" : "DIRECT SEND OFF";
        statusClearTimer.start();
    }

    if (actionPending && (directSending || (event->key() == Qt::Key_Escape)))
    {
        actionPending = false;
        statusMessage = "SETPOINT SENT";
        statusClearTimer.start();
        sendBodySetPointCoordinates();
    }

    HDDisplay::keyPressEvent(event);
}

void HSIDisplay::contextMenuEvent (QContextMenuEvent* event)
{
    event->ignore();
}

void HSIDisplay::setMetricWidth(double width)
{
    if (width != metricWidth) {
        metricWidth = width;
        emit metricWidthChanged(metricWidth);
    }
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void HSIDisplay::setActiveUAS(UASInterface* uas)
{
    if (this->uas != NULL) {
        disconnect(this->uas, SIGNAL(gpsSatelliteStatusChanged(int,int,float,float,float,bool)), this, SLOT(updateSatellite(int,int,float,float,float,bool)));
        disconnect(this->uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(attitudeThrustSetPointChanged(UASInterface*,double,double,double,double,quint64)), this, SLOT(updateAttitudeSetpoints(UASInterface*,double,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(positionSetPointsChanged(int,float,float,float,float,quint64)), this, SLOT(updatePositionSetpoints(int,float,float,float,float,quint64)));
        disconnect(uas, SIGNAL(userPositionSetPointsChanged(int,float,float,float,float)), this, SLOT(updateUserPositionSetpoints(int,float,float,float,float)));
        disconnect(this->uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
        disconnect(this->uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));

        disconnect(this->uas, SIGNAL(attitudeControlEnabled(bool)), this, SLOT(updateAttitudeControllerEnabled(bool)));
        disconnect(this->uas, SIGNAL(positionXYControlEnabled(bool)), this, SLOT(updatePositionXYControllerEnabled(bool)));
        disconnect(this->uas, SIGNAL(positionZControlEnabled(bool)), this, SLOT(updatePositionZControllerEnabled(bool)));
        disconnect(this->uas, SIGNAL(positionYawControlEnabled(bool)), this, SLOT(updatePositionYawControllerEnabled(bool)));

        disconnect(this->uas, SIGNAL(localizationChanged(UASInterface*,int)), this, SLOT(updateLocalization(UASInterface*,int)));
        disconnect(this->uas, SIGNAL(visionLocalizationChanged(UASInterface*,int)), this, SLOT(updateVisionLocalization(UASInterface*,int)));
        disconnect(this->uas, SIGNAL(gpsLocalizationChanged(UASInterface*,int)), this, SLOT(updateGpsLocalization(UASInterface*,int)));
        disconnect(this->uas, SIGNAL(irUltraSoundLocalizationChanged(UASInterface*,int)), this, SLOT(updateInfraredUltrasoundLocalization(UASInterface*,int)));
        disconnect(this->uas, SIGNAL(objectDetected(uint,int,int,QString,int,float,float)), this, SLOT(updateObjectPosition(uint,int,int,QString,int,float,float)));
    }

    connect(uas, SIGNAL(gpsSatelliteStatusChanged(int,int,float,float,float,bool)), this, SLOT(updateSatellite(int,int,float,float,float,bool)));
    connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(attitudeThrustSetPointChanged(UASInterface*,double,double,double,double,quint64)), this, SLOT(updateAttitudeSetpoints(UASInterface*,double,double,double,double,quint64)));
    connect(uas, SIGNAL(positionSetPointsChanged(int,float,float,float,float,quint64)), this, SLOT(updatePositionSetpoints(int,float,float,float,float,quint64)));
    connect(uas, SIGNAL(userPositionSetPointsChanged(int,float,float,float,float)), this, SLOT(updateUserPositionSetpoints(int,float,float,float,float)));
    connect(uas, SIGNAL(speedChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateSpeed(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(attitudeChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateAttitude(UASInterface*,double,double,double,quint64)));

    connect(uas, SIGNAL(attitudeControlEnabled(bool)), this, SLOT(updateAttitudeControllerEnabled(bool)));
    connect(uas, SIGNAL(positionXYControlEnabled(bool)), this, SLOT(updatePositionXYControllerEnabled(bool)));
    connect(uas, SIGNAL(positionZControlEnabled(bool)), this, SLOT(updatePositionZControllerEnabled(bool)));
    connect(uas, SIGNAL(positionYawControlEnabled(bool)), this, SLOT(updatePositionYawControllerEnabled(bool)));

    connect(uas, SIGNAL(localizationChanged(UASInterface*,int)), this, SLOT(updateLocalization(UASInterface*,int)));
    connect(uas, SIGNAL(visionLocalizationChanged(UASInterface*,int)), this, SLOT(updateVisionLocalization(UASInterface*,int)));
    connect(uas, SIGNAL(gpsLocalizationChanged(UASInterface*,int)), this, SLOT(updateGpsLocalization(UASInterface*,int)));
    connect(uas, SIGNAL(irUltraSoundLocalizationChanged(UASInterface*,int)), this, SLOT(updateInfraredUltrasoundLocalization(UASInterface*,int)));
    connect(uas, SIGNAL(objectDetected(uint,int,int,QString,int,float,float)), this, SLOT(updateObjectPosition(uint,int,int,QString,int,float,float)));

    this->uas = uas;

    resetMAVState();
}

void HSIDisplay::updateSpeed(UASInterface* uas, double vx, double vy, double vz, quint64 time)
{
    Q_UNUSED(uas);
    Q_UNUSED(time);
    this->vx = vx;
    this->vy = vy;
    this->vz = vz;
    this->speed = sqrt(pow(vx, 2.0) + pow(vy, 2.0) + pow(vz, 2.0));
}

void HSIDisplay::setBodySetpointCoordinateXY(double x, double y)
{
    if (uas)
    {
        userSetPointSet = true;
        userXYSetPointSet = true;
        // Set coordinates and send them out to MAV

        QPointF sp(x, y);
        sp = metricBodyToWorld(sp);
        uiXSetCoordinate = sp.x();
        uiYSetCoordinate = sp.y();

        qDebug() << "Attempting to set new setpoint at x: " << x << "metric y:" << y;


        //sendBodySetPointCoordinates();
        statusMessage = "POSITION SET, PRESS <ENTER> TO SEND";
        actionPending = true;
        statusClearTimer.start();
    }
}

void HSIDisplay::setBodySetpointCoordinateZ(double z)
{
    if (uas)
    {
        userSetPointSet = true;
        userZSetPointSet = true;
        // Set coordinates and send them out to MAV
        uiZSetCoordinate = z+uas->getLocalZ();
        statusMessage = "Z SET, PRESS <ENTER> TO SEND";
        actionPending = true;
        statusClearTimer.start();
    }
    //sendBodySetPointCoordinates();
}

void HSIDisplay::setBodySetpointCoordinateYaw(double yaw)
{
    if (uas)
    {
        if (!userXYSetPointSet && setPointKnown)
        {
            uiXSetCoordinate = bodyXSetCoordinate;
            uiYSetCoordinate = bodyYSetCoordinate;
        }
        else if (!userXYSetPointSet && mavInitialized)
        {
            QPointF coord = metricBodyToWorld(QPointF(0.0, 0.0));
            uiXSetCoordinate = coord.x();
            uiYSetCoordinate = coord.y();
        }
        userSetPointSet = true;
        userYawSetPointSet = true;
        // Set coordinates and send them out to MAV
        uiYawSet = atan2(sin(yaw+uas->getYaw()), cos(yaw+uas->getYaw()));
        statusMessage = "YAW SET, PRESS <ENTER> TO SEND";
        statusClearTimer.start();
        actionPending = true;
    }
    //sendBodySetPointCoordinates();
}

void HSIDisplay::sendBodySetPointCoordinates()
{
    // Send the coordinates to the MAV
    if (uas)
    {
        uas->setLocalPositionSetpoint(uiXSetCoordinate, uiYSetCoordinate, uiZSetCoordinate, uiYawSet);
    }
}

void HSIDisplay::updateAttitudeSetpoints(UASInterface* uas, double rollDesired, double pitchDesired, double yawDesired, double thrustDesired, quint64 usec)
{
    Q_UNUSED(uas);
    Q_UNUSED(usec);
    attXSet = pitchDesired;
    attYSet = rollDesired;
    attYawSet = yawDesired;
    altitudeSet = thrustDesired;
}

void HSIDisplay::updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 time)
{
    Q_UNUSED(uas);
    Q_UNUSED(time);
    this->roll = roll;
    this->pitch = pitch;
    this->yaw = yaw;
}

void HSIDisplay::updateUserPositionSetpoints(int uasid, float xDesired, float yDesired, float zDesired, float yawDesired)
{
	Q_UNUSED(uasid);
    uiXSetCoordinate = xDesired;
    uiYSetCoordinate = yDesired;
    uiZSetCoordinate = zDesired;
    uiYawSet = yawDesired;
    userXYSetPointSet = true;
    userZSetPointSet = true;
    userYawSetPointSet = true;
    userSetPointSet = true;
}

void HSIDisplay::updatePositionSetpoints(int uasid, float xDesired, float yDesired, float zDesired, float yawDesired, quint64 usec)
{
    Q_UNUSED(uasid);
    Q_UNUSED(usec);
    bodyXSetCoordinate = xDesired;
    bodyYSetCoordinate = yDesired;
    bodyZSetCoordinate = zDesired;
    bodyYawSet = yawDesired;
    mavInitialized = true;
    setPointKnown = true;
    positionSetPointKnown = true;

    if (!userSetPointSet && !dragStarted)
    {
        uiXSetCoordinate = bodyXSetCoordinate;
        uiYSetCoordinate = bodyYSetCoordinate;
        //        uiZSetCoordinate = bodyZSetCoordinate;
        uiYawSet= bodyYawSet;
    }
}

void HSIDisplay::updateLocalPosition(UASInterface*, double x, double y, double z, quint64 usec)
{
    this->x = x;
    this->y = y;
    this->z = z;
    localAvailable = usec;
}

void HSIDisplay::updateGlobalPosition(UASInterface*, double lat, double lon, double alt, quint64 usec)
{
    this->lat = lat;
    this->lon = lon;
    this->alt = alt;
    globalAvailable = usec;
}

void HSIDisplay::updateSatellite(int uasid, int satid, float elevation, float azimuth, float snr, bool used)
{
    Q_UNUSED(uasid);
    // If slot is empty, insert object
    if (satid != 0) // Satellite PRNs currently range from 1-32, but are never zero
    if (gpsSatellites.contains(satid)) {
        gpsSatellites.value(satid)->update(satid, elevation, azimuth, snr, used);
    } else {
        gpsSatellites.insert(satid, new GPSSatellite(satid, elevation, azimuth, snr, used));
    }
}

void HSIDisplay::updatePositionYawControllerEnabled(bool enabled)
{
    yawControlEnabled = enabled;
    yawControlKnown = true;
}

/**
 * @param fix 0: lost, 1: 2D local position hold, 2: 2D localization, 3: 3D localization
 */
void HSIDisplay::updateLocalization(UASInterface* uas, int fix)
{
    Q_UNUSED(uas);
    positionFix = fix;
    positionFixKnown = true;
    //qDebug() << "LOCALIZATION FIX CALLED";
}
/**
 * @param fix 0: lost, 1: at least one satellite, but no GPS fix, 2: 2D localization, 3: 3D localization
 */
void HSIDisplay::updateGpsLocalization(UASInterface* uas, int fix)
{
    Q_UNUSED(uas);
    gpsFix = fix;
    gpsFixKnown = true;
}
/**
 * @param fix 0: lost, 1: 2D local position hold, 2: 2D localization, 3: 3D localization
 */
void HSIDisplay::updateVisionLocalization(UASInterface* uas, int fix)
{
    Q_UNUSED(uas);
    visionFix = fix;
    visionFixKnown = true;
    //qDebug() << "VISION FIX GOT CALLED";
}

/**
 * @param fix 0: lost, 1-N: Localized with N ultrasound or infrared sensors
 */
void HSIDisplay::updateInfraredUltrasoundLocalization(UASInterface* uas, int fix)
{
    Q_UNUSED(uas);
    iruFix = fix;
    iruFixKnown = true;
}

QColor HSIDisplay::getColorForSNR(float snr)
{
    QColor color;
    if (snr > 0 && snr < 30) {
        color = QColor(250, 10, 10);
    } else if (snr >= 30 && snr < 35) {
        color = QColor(230, 230, 10);
    } else if (snr >= 35 && snr < 40) {
        color = QColor(90, 200, 90);
    } else if (snr >= 40) {
        color = QColor(20, 200, 20);
    } else {
        color = QColor(180, 180, 180);
    }
    return color;
}

void HSIDisplay::drawSetpointXYZYaw(float x, float y, float z, float yaw, const QColor &color, QPainter &painter)
{
    if (uas)
    {
        float radius = vwidth / 18.0f;
        QPen pen(color);
        pen.setWidthF(refLineWidthToPen(1.6f));
        pen.setColor(color);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPointF in(x, y);
        // Transform from body to world coordinates
        in = metricWorldToBody(in);
        // Scale from metric to screen reference coordinates
        QPointF p = metricBodyToRef(in);

        QPolygonF poly(4);
        // Top point
        poly.replace(0, QPointF(p.x()-radius, p.y()-radius));
        // Right point
        poly.replace(1, QPointF(p.x()+radius, p.y()-radius));
        // Bottom point
        poly.replace(2, QPointF(p.x()+radius, p.y()+radius));
        poly.replace(3, QPointF(p.x()-radius, p.y()+radius));

        // Label
        paintText(QString("z: %1").arg(z, 3, 'f', 1, QChar(' ')), color, 2.1f, p.x()-radius, p.y()-radius-3.5f, &painter);

        drawPolygon(poly, &painter);

        radius *= 0.8f;
        drawLine(p.x(), p.y(), p.x()+sin(-yaw + uas->getYaw() + M_PI) * radius, p.y()+cos(-yaw + uas->getYaw() + M_PI) * radius, refLineWidthToPen(0.6f), color, &painter);

        // Draw center dot
        painter.setBrush(color);
        drawCircle(p.x(), p.y(), radius * 0.1f, 0.1f, color, &painter);
    }
}

void HSIDisplay::drawWaypoint(QPainter& painter, const QColor& color, float width, const QVector<Waypoint*>& list, int i, const QPointF& p)
{
    painter.setBrush(Qt::NoBrush);

    // Setup pen for foreground
    QPen pen(color);
    pen.setWidthF(width);

    // DRAW WAYPOINT
    float waypointSize = vwidth / 20.0f * 2.0f;
    QPolygonF poly(4);
    // Top point
    poly.replace(0, QPointF(p.x(), p.y()-waypointSize/2.0f));
    // Right point
    poly.replace(1, QPointF(p.x()+waypointSize/2.0f, p.y()));
    // Bottom point
    poly.replace(2, QPointF(p.x(), p.y() + waypointSize/2.0f));
    poly.replace(3, QPointF(p.x() - waypointSize/2.0f, p.y()));

    float radius = (waypointSize/2.0f) * 0.8 * (1/sqrt(2.0f));
    float acceptRadius = list.at(i)->getAcceptanceRadius();

    // Draw background
    pen.setColor(Qt::black);
    painter.setPen(pen);
    drawLine(p.x(), p.y(), p.x()+sin(list.at(i)->getYaw()/180.0*M_PI-yaw) * radius, p.y()-cos(list.at(i)->getYaw()/180.0*M_PI-yaw) * radius, refLineWidthToPen(0.4f*3.0f), Qt::black, &painter);
    drawPolygon(poly, &painter);
    drawCircle(p.x(), p.y(), metricToRef(acceptRadius), 3.0, Qt::black, &painter);

    // Draw foreground
    pen.setColor(color);
    painter.setPen(pen);
    drawLine(p.x(), p.y(), p.x()+sin(list.at(i)->getYaw()/180.0*M_PI-yaw) * radius, p.y()-cos(list.at(i)->getYaw()/180.0*M_PI-yaw) * radius, refLineWidthToPen(0.4f), color, &painter);
    drawPolygon(poly, &painter);
    drawCircle(p.x(), p.y(), metricToRef(acceptRadius), 1.0, Qt::green, &painter);
}

void HSIDisplay::drawWaypoints(QPainter& painter)
{
    if (uas)
    {
        const QVector<Waypoint*>& list = uas->getWaypointManager()->getWaypointEditableList();

        // Do not work on empty lists
        if (list.size() == 0) return;

        QColor color;
        painter.setBrush(Qt::NoBrush);

        // XXX Ugly hacks, needs rewrite

        QPointF lastWaypoint;
        QPointF currentWaypoint;
        int currentIndex = 0;

        for (int i = 0; i < list.size(); i++)
        {
            QPointF in;
            if (list.at(i)->getFrame() == MAV_FRAME_LOCAL_NED)
            {
                // Do not transform
                in = QPointF(list.at(i)->getX(), list.at(i)->getY());
            } else {
                // Transform to local coordinates first
                double x = list.at(i)->getX();
                double y = list.at(i)->getY();
                in = QPointF(x, y);
            }
            // Transform from world to body coordinates
            in = metricWorldToBody(in);
            // Scale from metric to screen reference coordinates
            QPointF p = metricBodyToRef(in);

            // Select color based on if this is the current waypoint
            if (list.at(i)->getCurrent())
            {
                currentIndex = i;
                currentWaypoint = p;
            }
            else
            {
                drawWaypoint(painter, QGC::colorCyan, refLineWidthToPen(0.4f), list, i, p);
            }

            // DRAW CONNECTING LINE
            // Draw line from last waypoint to this one
            if (!lastWaypoint.isNull())
            {
                drawLine(lastWaypoint.x(), lastWaypoint.y(), p.x(), p.y(), refLineWidthToPen(0.4f*2.0f), Qt::black, &painter);
                drawLine(lastWaypoint.x(), lastWaypoint.y(), p.x(), p.y(), refLineWidthToPen(0.4f), QGC::colorCyan, &painter);
            }
            lastWaypoint = p;
        }

        drawWaypoint(painter, QGC::colorYellow, refLineWidthToPen(0.8f), list, currentIndex, currentWaypoint);
    }
}

void HSIDisplay::drawSafetyArea(const QPointF &topLeft, const QPointF &bottomRight, const QColor &color, QPainter &painter)
{
    QPen pen(color);
    pen.setWidthF(refLineWidthToPen(0.1f));
    pen.setColor(color);
    pen.setBrush(Qt::NoBrush);
    painter.setPen(pen);
    painter.drawRect(QRectF(metricBodyToScreen(metricWorldToBody(topLeft)), metricBodyToScreen(metricWorldToBody(bottomRight))));
}

void HSIDisplay::drawGPS(QPainter &painter)
{
    float xCenter = xCenterPos;
    float yCenter = xCenterPos;
    // Max satellite circle radius

    const float margin = 0.15f;  // 20% margin of total width on each side
    float radius = (vwidth - vwidth * 2.0f * margin) / 2.0f;
    quint64 currTime = MG::TIME::getGroundTimeNowUsecs();

    // Draw satellite labels
    //    QString label;
    //    label.sprintf("%05.1f", value);
    //    paintText(label, color, 4.5f, xRef-7.5f, yRef-2.0f, painter);

    QMapIterator<int, GPSSatellite*> i(gpsSatellites);
    while (i.hasNext()) {
        i.next();
        GPSSatellite* sat = i.value();

        // Check if update is not older than 10 seconds, else delete satellite
        if (sat->lastUpdate + 10000000 < currTime) {
            // Delete and go to next satellite
            gpsSatellites.remove(i.key());
            if (i.hasNext()) {
                i.next();
                sat = i.value();
            } else {
                continue;
            }
        }

        if (sat) {
            // Draw satellite
            QBrush brush;
            QColor color = getColorForSNR(sat->snr);
            brush.setColor(color);
            if (sat->used) {
                brush.setStyle(Qt::SolidPattern);
            } else {
                brush.setStyle(Qt::NoBrush);
            }
            painter.setPen(Qt::SolidLine);
            painter.setPen(color);
            painter.setBrush(brush);

            float xPos = xCenter + (sin(((sat->azimuth/255.0f)*360.0f)/180.0f * M_PI) * cos(sat->elevation/180.0f * M_PI)) * radius;
            float yPos = yCenter - (cos(((sat->azimuth/255.0f)*360.0f)/180.0f * M_PI) * cos(sat->elevation/180.0f * M_PI)) * radius;

            // Draw circle for satellite, filled for used satellites
            drawCircle(xPos, yPos, vwidth*0.02f, 1.0f, color, &painter);
            // Draw satellite PRN
            paintText(QString::number(sat->id), QColor(255, 255, 255), 2.9f, xPos+1.7f, yPos+2.0f, &painter);
        }
    }
}

void HSIDisplay::drawObjects(QPainter &painter)
{
    Q_UNUSED(painter);
}

void HSIDisplay::drawPositionDirection(float xRef, float yRef, float radius, const QColor& color, QPainter* painter)
{
    if (xyControlKnown && xyControlEnabled) {
        // Draw the needle
        const float maxWidth = radius / 5.0f;
        const float minWidth = maxWidth * 0.3f;

        float angle = atan2(posXSet, -posYSet);
        angle -= (float)M_PI/2.0f;

        QPolygonF p(6);

        //radius *= ((posXSaturation + posYSaturation) - sqrt(pow(posXSet, 2), pow(posYSet, 2))) / (2*posXSaturation);

        radius *= sqrt(pow(posXSet, 2) + pow(posYSet, 2)) / sqrt(posXSaturation + posYSaturation);

        p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.4f));
        p.replace(1, QPointF(xRef-minWidth/2.0f, yRef-radius * 0.9f));
        p.replace(2, QPointF(xRef+minWidth/2.0f, yRef-radius * 0.9f));
        p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef-radius * 0.4f));
        p.replace(4, QPointF(xRef,               yRef-radius * 0.36f));
        p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.4f));

        rotatePolygonClockWiseRad(p, angle, QPointF(xRef, yRef));

        QBrush indexBrush;
        indexBrush.setColor(color);
        indexBrush.setStyle(Qt::SolidPattern);
        painter->setPen(Qt::SolidLine);
        painter->setPen(color);
        painter->setBrush(indexBrush);
        drawPolygon(p, painter);

        //qDebug() << "DRAWING POS SETPOINT X:" << posXSet << "Y:" << posYSet << angle;
    }
}

void HSIDisplay::drawAttitudeDirection(float xRef, float yRef, float radius, const QColor& color, QPainter* painter)
{
    if (attControlKnown && attControlEnabled) {
        // Draw the needle
        const float maxWidth = radius / 10.0f;
        const float minWidth = maxWidth * 0.3f;

        float angle = atan2(attYSet, -attXSet);

        radius *= sqrt(attXSet*attXSet + attYSet*attYSet) / sqrt(attXSaturation*attXSaturation + attYSaturation*attYSaturation);

        QPolygonF p(6);

        p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.4f));
        p.replace(1, QPointF(xRef-minWidth/2.0f, yRef-radius * 0.9f));
        p.replace(2, QPointF(xRef+minWidth/2.0f, yRef-radius * 0.9f));
        p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef-radius * 0.4f));
        p.replace(4, QPointF(xRef,               yRef-radius * 0.36f));
        p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.4f));

        rotatePolygonClockWiseRad(p, angle, QPointF(xRef, yRef));

        QBrush indexBrush;
        indexBrush.setColor(color);
        indexBrush.setStyle(Qt::SolidPattern);
        painter->setPen(Qt::SolidLine);
        painter->setPen(color);
        painter->setBrush(indexBrush);
        drawPolygon(p, painter);

        // TODO Draw Yaw indicator

        //qDebug() << "DRAWING ATT SETPOINT X:" << attXSet << "Y:" << attYSet << angle;
    }
}

void HSIDisplay::drawAltitudeSetpoint(float xRef, float yRef, float radius, const QColor& color, QPainter* painter)
{
    if (zControlKnown && zControlEnabled) {
        // Draw the circle
        QPen circlePen(Qt::SolidLine);
        circlePen.setWidth(refLineWidthToPen(0.5f));
        circlePen.setColor(color);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(circlePen);
        drawCircle(xRef, yRef, radius, 200.0f, color, painter);
        //drawCircle(xRef, yRef, radius, 200.0f, 170.0f, 1.0f, color, painter);

        //    // Draw the value
        //    QString label;
        //    label.sprintf("%05.1f", value);
        //    paintText(label, color, 4.5f, xRef-7.5f, yRef-2.0f, painter);
    }
}

void HSIDisplay::wheelEvent(QWheelEvent* event)
{
    double zoomScale = 0.005; // Scaling of zoom value
    if(event->delta() > 0) {
        // Reduce width -> Zoom in
        metricWidth -= event->delta() * zoomScale;
    } else {
        // Increase width -> Zoom out
        metricWidth -= event->delta() * zoomScale;
    }
    metricWidth = qBound(0.5, metricWidth, 9999.0);
    emit metricWidthChanged(metricWidth);
}

void HSIDisplay::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event) {
        refreshTimer->start(updateInterval);
    }
}

void HSIDisplay::hideEvent(QHideEvent* event)
{
    // React only to internal (post-display)
    // events
    Q_UNUSED(event) {
        refreshTimer->stop();
    }
}

void HSIDisplay::updateJoystick(double roll, double pitch, double yaw, double thrust, int xHat, int yHat)
{
    Q_UNUSED(roll);
    Q_UNUSED(pitch);
    Q_UNUSED(yaw);
    Q_UNUSED(thrust);
    Q_UNUSED(xHat);
    Q_UNUSED(yHat);
}

void HSIDisplay::pressKey(int key)
{
    Q_UNUSED(key);
}


