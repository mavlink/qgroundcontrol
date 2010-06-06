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
#include "UASManager.h"
#include "HSIDisplay.h"
#include "MG.h"

#include <QDebug>

HSIDisplay::HSIDisplay(QStringList* plotList, QWidget *parent) :
        HDDisplay(plotList, parent),
{
    
}

void HSIDisplay::paintDisplay()
{
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
    const int columns = 3;
    const float spacing = 0.4f; // 40% of width
    const float gaugeWidth = vwidth / (((float)columns) + (((float)columns+1) * spacing + spacing * 0.1f));
    const QColor gaugeColor = QColor(200, 200, 200);
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

    // Now connect the new UAS

    //if (this->uas != uas)
    // {
    //qDebug() << "UAS SET!" << "ID:" << uas->getUASID();
    // Setup communication
    //connect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    //}
}

void HSIDisplay::drawGPS()
{
	
}

void HSIDisplay::drawObjects()
{
	
}

void HSIDisplay::drawBaseLines(float xRef, float yRef, float radius, float yaw, const QColor& color, QPainter* painter, bool solid)
{
    // Draw the circle
    QPen circlePen(Qt::SolidLine);
    if (!solid) circlePen.setStyle(Qt::DotLine);
    circlePen.setWidth(refLineWidthToPen(0.5f));
    circlePen.setColor(defaultColor);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(circlePen);
    drawCircle(xRef, yRef, radius, 200.0f, color, painter);
    //drawCircle(xRef, yRef, radius, 200.0f, 170.0f, 1.0f, color, painter);

    QString label;
    label.sprintf("%05.1f", value);

    // Draw the value
    paintText(label, color, 4.5f, xRef-7.5f, yRef-2.0f, painter);

    // Draw the needle
    // Scale the rotation so that the gauge does one revolution
    // per max. change
    const float rangeScale = (2.0f * M_PI);
    const float maxWidth = radius / 10.0f;
    const float minWidth = maxWidth * 0.3f;

    QPolygonF p(6);

    p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(1, QPointF(xRef-minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(2, QPointF(xRef+minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(4, QPointF(xRef,               yRef-radius * 0.46f));
    p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));

    rotatePolygonClockWiseRad(p, yaw*rangeScale, QPointF(xRef, yRef));

    QBrush indexBrush;
    indexBrush.setColor(defaultColor);
    indexBrush.setStyle(Qt::SolidPattern);
    painter->setPen(Qt::SolidLine);
    painter->setPen(defaultColor);
    painter->setBrush(indexBrush);
    drawPolygon(p, painter);
}

void HDDisplay::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
