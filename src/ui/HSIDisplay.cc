/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

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
#include "UASManager.h"
#include "HSIDisplay.h"
#include "MG.h"

#include <QDebug>

HSIDisplay::HSIDisplay(QWidget *parent) :
        HDDisplay(NULL, parent),
        gpsSatellites(),
        satellitesUsed(0),
        attXSet(0),
        attYSet(0),
        attYawSet(0),
        altitudeSet(1.0),
        posXSet(0),
        posYSet(0),
        posZSet(0),
        attXSaturation(0.33),
        attYSaturation(0.33),
        attYawSaturation(0.33),
        posXSaturation(1.0),
        posYSaturation(1.0),
        altitudeSaturation(1.0),
        lat(0),
        lon(0),
        alt(0),
        globalAvailable(0),
        x(0),
        y(0),
        z(0),
        localAvailable(0)
{
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
}

void HSIDisplay::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event);
    //paintGL();
    static quint64 interval = 0;
    //qDebug() << "INTERVAL:" << MG::TIME::getGroundTimeNow() - interval << __FILE__ << __LINE__;
    interval = MG::TIME::getGroundTimeNow();
    paintDisplay();
}

void HSIDisplay::paintDisplay()
{
    // Center location of the HSI gauge items

    float xCenterPos = vwidth/2.0f;
    float yCenterPos = vheight/2.0f;

    // Size of the ring instrument
    const float margin = 0.1f;  // 10% margin of total width on each side
    float baseRadius = (vwidth - vwidth * 2.0f * margin) / 2.0f;

    quint64 refreshInterval = 100;
    quint64 currTime = MG::TIME::getGroundTimeNow();
    if (currTime - lastPaintTime < refreshInterval)
    {
        // FIXME Need to find the source of the spurious paint events
        //return;
    }
    lastPaintTime = currTime;
    // Draw instruments
    // TESTING THIS SHOULD BE MOVED INTO A QGRAPHICSVIEW
    // Update scaling factor
    // adjust scaling to fit both horizontally and vertically
    scalingFactor = this->width()/vwidth;
    double scalingFactorH = this->height()/vheight;
    if (scalingFactorH < scalingFactor) scalingFactor = scalingFactorH;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.fillRect(QRect(0, 0, width(), height()), backgroundColor);
    const QColor ringColor = QColor(200, 250, 200);

    // Draw base instrument
    // ----------------------
    const int ringCount = 2;
    for (int i = 0; i < ringCount; i++)
    {
        float radius = (vwidth - vwidth * 2.0f * margin) / (2.0f * i+1) / 2.0f;
        drawCircle(vwidth/2.0f, vheight/2.0f, radius, 0.1f, ringColor, &painter);
    }

    // Draw center indicator
    drawCircle(vwidth/2.0f, vheight/2.0f, 1.0f, 0.1f, ringColor, &painter);

    // Draw orientation labels
    paintText(tr("N"), ringColor, 3.5f, xCenterPos - 1.0f, yCenterPos - baseRadius - 5.5f, &painter);
    paintText(tr("S"), ringColor, 3.5f, xCenterPos - 1.0f, yCenterPos + baseRadius + 1.5f, &painter);
    paintText(tr("E"), ringColor, 3.5f, xCenterPos + baseRadius + 2.0f, yCenterPos - 1.75f, &painter);
    paintText(tr("W"), ringColor, 3.5f, xCenterPos - baseRadius - 5.5f, yCenterPos - 1.75f, &painter);

    // ----------------------

    // Draw state indicator

    // Draw position
    QColor positionColor(20, 20, 200);
    drawPositionSetpoint(xCenterPos, yCenterPos, baseRadius, positionColor, &painter);

    // Draw attitude
    QColor attitudeColor(200, 20, 20);
    drawPositionSetpoint(xCenterPos, yCenterPos, baseRadius, attitudeColor, &painter);



    // Draw satellites
    drawGPS();

    if (localAvailable > 0)
    {
        QString str;
        str.sprintf("%05.2f %05.2f %05.2f m", x, y, z);
        paintText(str, ringColor, 4.5f, xCenterPos + baseRadius - 5.5f, yCenterPos + baseRadius - 20.75f, &painter);
    }


    //drawSystemIndicator(10.0f-gaugeWidth/2.0f, 20.0f, 10.0f, 40.0f, 15.0f, &painter);
    //drawGauge(15.0f, 15.0f, gaugeWidth/2.0f, 0, 1.0f, "thrust", values.value("thrust", 0.0f), gaugeColor, &painter, qMakePair(0.45f, 0.8f), qMakePair(0.8f, 1.0f), true);
    //drawGauge(15.0f+gaugeWidth*1.7f, 15.0f, gaugeWidth/2.0f, 0, 10.0f, "altitude", values.value("altitude", 0.0f), gaugeColor, &painter, qMakePair(1.0f, 2.5f), qMakePair(0.0f, 0.5f), true);

    // Left spacing from border / other gauges, measured from left edge to center
    //    float leftSpacing = gaugeWidth * spacing;
    //    float xCoord = leftSpacing + gaugeWidth/2.0f;
    //
    //    float topSpacing = leftSpacing;
    //    float yCoord = topSpacing + gaugeWidth/2.0f;
    //
    //    for (int i = 0; i < acceptList->size(); ++i)
    //    {
    //        QString value = acceptList->at(i);
    //        drawGauge(xCoord, yCoord, gaugeWidth/2.0f, minValues.value(value, -1.0f), maxValues.value(value, 1.0f), value, values.value(value, minValues.value(value, 0.0f)), gaugeColor, &painter, goodRanges.value(value, qMakePair(0.0f, 0.5f)), critRanges.value(value, qMakePair(0.7f, 1.0f)), true);
    //        xCoord += gaugeWidth + leftSpacing;
    //        // Move one row down if necessary
    //        if (xCoord + gaugeWidth > vwidth)
    //        {
    //            yCoord += topSpacing + gaugeWidth;
    //            xCoord = leftSpacing + gaugeWidth/2.0f;
    //        }
    //    }
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void HSIDisplay::setActiveUAS(UASInterface* uas)
{
    HDDisplay::setActiveUAS(uas);
    //qDebug() << "ATTEMPTING TO SET UAS";
    if (this->uas != NULL && this->uas != uas)
    {
        // Disconnect any previously connected active MAV
        //disconnect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    }

    connect(uas, SIGNAL(gpsSatelliteStatusChanged(int,int,float,float,float,bool)), this, SLOT(updateSatellite(int,int,float,float,float,bool)));
    connect(uas, SIGNAL(localPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateLocalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
    connect(uas, SIGNAL(attitudeThrustSetPointChanged(UASInterface*,double,double,double,double,quint64)), this, SLOT(updateAttitudeSetpoints(UASInterface*,double,double,double,double,quint64)));

    // Now connect the new UAS

    //if (this->uas != uas)
    // {
    //qDebug() << "UAS SET!" << "ID:" << uas->getUASID();
    // Setup communication
    //connect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    //}
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

void HSIDisplay::updatePositionSetpoints(int uasid, double xDesired, double yDesired, double zDesired, quint64 usec)
{
    Q_UNUSED(usec);
    Q_UNUSED(uasid);
    posXSet = xDesired;
    posYSet = yDesired;
    posZSet = zDesired;
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
    //qDebug() << "UPDATED SATELLITE";
    // If slot is empty, insert object
    if (gpsSatellites.contains(satid))
    {
        gpsSatellites.value(satid)->update(satid, elevation, azimuth, snr, used);
    }
    else
    {
        gpsSatellites.insert(satid, new GPSSatellite(satid, elevation, azimuth, snr, used));
    }
}

QColor HSIDisplay::getColorForSNR(float snr)
{
    QColor color;
    if (snr > 0 && snr < 30)
    {
        color = QColor(250, 10, 10);
    }
    else if (snr >= 30 && snr < 35)
    {
        color = QColor(230, 230, 10);
    }
    else if (snr >= 35 && snr < 40)
    {
        color = QColor(90, 200, 90);
    }
    else if (snr >= 40)
    {
        color = QColor(20, 200, 20);
    }
    else
    {
        color = QColor(180, 180, 180);
    }
    return color;
}

void HSIDisplay::drawGPS()
{
    float xCenter = vwidth/2.0f;
    float yCenter = vwidth/2.0f;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);

    // Max satellite circle radius

    const float margin = 0.15f;  // 20% margin of total width on each side
    float radius = (vwidth - vwidth * 2.0f * margin) / 2.0f;
    quint64 currTime = MG::TIME::getGroundTimeNowUsecs();

    // Draw satellite labels
    //    QString label;
    //    label.sprintf("%05.1f", value);
    //    paintText(label, color, 4.5f, xRef-7.5f, yRef-2.0f, painter);

    QMapIterator<int, GPSSatellite*> i(gpsSatellites);
    while (i.hasNext())
    {
        i.next();
        GPSSatellite* sat = i.value();

        // Check if update is not older than 5 seconds, else delete satellite
        if (sat->lastUpdate + 1000000 < currTime)
        {
            // Delete and go to next satellite
            gpsSatellites.remove(i.key());
            if (i.hasNext())
            {
                i.next();
                sat = i.value();
            }
            else
            {
                continue;
            }
        }

        if (sat)
        {
            // Draw satellite
            QBrush brush;
            QColor color = getColorForSNR(sat->snr);
            brush.setColor(color);
            if (sat->used)
            {
                brush.setStyle(Qt::SolidPattern);
            }
            else
            {
                brush.setStyle(Qt::NoBrush);
            }
            painter.setPen(Qt::SolidLine);
            painter.setPen(color);
            painter.setBrush(brush);

            float xPos = xCenter + (sin(((sat->azimuth/255.0f)*360.0f)/180.0f * M_PI) * cos(sat->elevation/180.0f * M_PI)) * radius;
            float yPos = yCenter - (cos(((sat->azimuth/255.0f)*360.0f)/180.0f * M_PI) * cos(sat->elevation/180.0f * M_PI)) * radius;

            drawCircle(xPos, yPos, vwidth*0.02f, 1.0f, color, &painter);
            paintText(QString::number(sat->id), QColor(255, 255, 255), 2.9f, xPos+1.7f, yPos+2.0f, &painter);
        }
    }
}

void HSIDisplay::drawObjects()
{

}

void HSIDisplay::drawPositionSetpoint(float xRef, float yRef, float radius, const QColor& color, QPainter* painter)
{
    // Draw the needle
    const float maxWidth = radius / 10.0f;
    const float minWidth = maxWidth * 0.3f;

    float angle = asin(posXSet) + acos(posYSet);

    QPolygonF p(6);

    p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(1, QPointF(xRef-minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(2, QPointF(xRef+minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(4, QPointF(xRef,               yRef-radius * 0.46f));
    p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));

    rotatePolygonClockWiseRad(p, angle, QPointF(xRef, yRef));

    QBrush indexBrush;
    indexBrush.setColor(color);
    indexBrush.setStyle(Qt::SolidPattern);
    painter->setPen(Qt::SolidLine);
    painter->setPen(color);
    painter->setBrush(indexBrush);
    drawPolygon(p, painter);
}

void HSIDisplay::drawAttitudeSetpoint(float xRef, float yRef, float radius, const QColor& color, QPainter* painter)
{
    // Draw the needle
    const float maxWidth = radius / 10.0f;
    const float minWidth = maxWidth * 0.3f;

    float angle = asin(attXSet) + acos(attYSet);

    QPolygonF p(6);

    p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(1, QPointF(xRef-minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(2, QPointF(xRef+minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(4, QPointF(xRef,               yRef-radius * 0.46f));
    p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));

    rotatePolygonClockWiseRad(p, angle, QPointF(xRef, yRef));

    QBrush indexBrush;
    indexBrush.setColor(color);
    indexBrush.setStyle(Qt::SolidPattern);
    painter->setPen(Qt::SolidLine);
    painter->setPen(color);
    painter->setBrush(indexBrush);
    drawPolygon(p, painter);

    // TODO Draw Yaw indicator
}

void HSIDisplay::drawAltitudeSetpoint(float xRef, float yRef, float radius, const QColor& color, QPainter* painter)
{
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
