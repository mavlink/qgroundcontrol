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
 *   @brief Implementation of Head Down Display (HDD)
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QFile>
#include <QGLWidget>
#include <QStringList>
#include "UASManager.h"
#include "HDDisplay.h"
#include "ui_HDDisplay.h"

#include <QDebug>

HDDisplay::HDDisplay(QStringList* plotList, QWidget *parent) :
        QWidget(parent),
        uas(NULL),
        values(QMap<QString, float>()),
        valuesDot(QMap<QString, float>()),
        valuesMean(QMap<QString, float>()),
        valuesCount(QMap<QString, int>()),
        lastUpdate(QMap<QString, quint64>()),
        minValues(),
        maxValues(),
        goodRanges(),
        critRanges(),
        xCenterOffset(0.0f),
        yCenterOffset(0.0f),
        vwidth(80.0f),
        vheight(80.0f),
        backgroundColor(QColor(0, 0, 0)),
        defaultColor(QColor(70, 200, 70)),
        setPointColor(QColor(200, 20, 200)),
        warningColor(Qt::yellow),
        criticalColor(Qt::red),
        infoColor(QColor(20, 200, 20)),
        fuelColor(criticalColor),
        warningBlinkRate(5),
        refreshTimer(new QTimer(this)),
        hardwareAcceleration(true),
        strongStrokeWidth(1.5f),
        normalStrokeWidth(1.0f),
        fineStrokeWidth(0.5f),
        acceptList(plotList),
        m_ui(new Ui::HDDisplay)
{
    //m_ui->setupUi(this);


    // Refresh timer
    refreshTimer->setInterval(60);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(repaint()));
    //connect(refreshTimer, SIGNAL(timeout()), this, SLOT(paintGL()));

    fontDatabase = QFontDatabase();
    const QString fontFileName = ":/general/vera.ttf"; ///< Font file is part of the QRC file and compiled into the app
    const QString fontFamilyName = "Bitstream Vera Sans";
    if(!QFile::exists(fontFileName)) qDebug() << "ERROR! font file: " << fontFileName << " DOES NOT EXIST!";

    fontDatabase.addApplicationFont(fontFileName);
    font = fontDatabase.font(fontFamilyName, "Roman", (int)(10*scalingFactor*1.2f+0.5f));
    if (font.family() != fontFamilyName) qDebug() << "ERROR! Font not loaded: " << fontFamilyName;

    // Connect with UAS
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    //start();
}

HDDisplay::~HDDisplay()
{
    delete m_ui;
}

void HDDisplay::paintEvent(QPaintEvent * event)
{
    paintGL();
}

void HDDisplay::paintGL()
{
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
    float leftSpacing = gaugeWidth * spacing;
    float xCoord = leftSpacing + gaugeWidth/2.0f;

    float topSpacing = leftSpacing;
    float yCoord = topSpacing + gaugeWidth/2.0f;

    for (int i = 0; i < acceptList->size(); ++i)
    {
        QString value = acceptList->at(i);
        drawGauge(xCoord, yCoord, gaugeWidth/2.0f, minValues.value(value, 0.0f), maxValues.value(value, 1.0f), value, values.value(value, minValues.value(value, 0.0f)), gaugeColor, &painter, goodRanges.value(value, qMakePair(0.0f, 0.5f)), critRanges.value(value, qMakePair(0.7f, 1.0f)), true);
        xCoord += gaugeWidth + leftSpacing;
        // Move one row down if necessary
        if (xCoord + gaugeWidth > vwidth)
        {
            yCoord += topSpacing + gaugeWidth;
            xCoord = leftSpacing + gaugeWidth/2.0f;
        }
    }
}

void HDDisplay::start()
{
    refreshTimer->start();
}

void HDDisplay::stop()
{
    refreshTimer->stop();
}

/**
 *
 * @param uas the UAS/MAV to monitor/display with the HUD
 */
void HDDisplay::setActiveUAS(UASInterface* uas)
{
    //qDebug() << "ATTEMPTING TO SET UAS";
    if (this->uas != NULL && this->uas != uas)
    {
        // Disconnect any previously connected active MAV
        disconnect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    }

    // Now connect the new UAS

    //if (this->uas != uas)
    // {
    //qDebug() << "UAS SET!" << "ID:" << uas->getUASID();
    // Setup communication
    connect(uas, SIGNAL(valueChanged(UASInterface*,QString,double,quint64)), this, SLOT(updateValue(UASInterface*,QString,double,quint64)));
    //}
}

/**
 * Rotate a polygon around a point
 *
 * @param p polygon to rotate
 * @param origin the rotation center
 * @param angle rotation angle, in radians
 * @return p Polygon p rotated by angle around the origin point
 */
void HDDisplay::rotatePolygonClockWiseRad(QPolygonF& p, float angle, QPointF origin)
{
    // Standard 2x2 rotation matrix, counter-clockwise
    //
    //   |  cos(phi)   sin(phi) |
    //   | -sin(phi)   cos(phi) |
    //
    for (int i = 0; i < p.size(); i++)
    {
        QPointF curr = p.at(i);

        const float x = curr.x();
        const float y = curr.y();

        curr.setX(((cos(angle) * (x-origin.x())) + (-sin(angle) * (y-origin.y()))) + origin.x());
        curr.setY(((sin(angle) * (x-origin.x())) + (cos(angle) * (y-origin.y()))) + origin.y());
        p.replace(i, curr);
    }
}

void HDDisplay::drawPolygon(QPolygonF refPolygon, QPainter* painter)
{
    // Scale coordinates
    QPolygonF draw(refPolygon.size());
    for (int i = 0; i < refPolygon.size(); i++)
    {
        QPointF curr;
        curr.setX(refToScreenX(refPolygon.at(i).x()));
        curr.setY(refToScreenY(refPolygon.at(i).y()));
        draw.replace(i, curr);
    }
    painter->drawPolygon(draw);
}

void HDDisplay::drawChangeRateStrip(float xRef, float yRef, float height, float minRate, float maxRate, float value, QPainter* painter)
{
    QBrush brush(defaultColor, Qt::NoBrush);
    painter->setBrush(brush);
    QPen rectPen(Qt::SolidLine);
    rectPen.setWidth(0);
    rectPen.setColor(defaultColor);
    painter->setPen(rectPen);

    float scaledValue = value;

    // Saturate value
    if (value > maxRate) scaledValue = maxRate;
    if (value < minRate) scaledValue = minRate;

    //           x (Origin: xRef, yRef)
    //           -
    //           |
    //           |
    //           |
    //           =
    //           |
    //   -0.005 >|
    //           |
    //           -

    const float width = height / 8.0f;
    const float lineWidth = 0.5f;

    // Indicator lines
    // Top horizontal line
    drawLine(xRef, yRef, xRef+width, yRef, lineWidth, defaultColor, painter);
    // Vertical main line
    drawLine(xRef+width/2.0f, yRef, xRef+width/2.0f, yRef+height, lineWidth, defaultColor, painter);
    // Zero mark
    drawLine(xRef, yRef+height/2.0f, xRef+width, yRef+height/2.0f, lineWidth, defaultColor, painter);
    // Horizontal bottom line
    drawLine(xRef, yRef+height, xRef+width, yRef+height, lineWidth, defaultColor, painter);

    // Text
    QString label;
    label.sprintf("< %06.2f", value);
    paintText(label, defaultColor, 3.0f, xRef+width/2.0f, yRef+height-((scaledValue - minRate)/(maxRate-minRate))*height - 1.6f, painter);
}

void HDDisplay::drawGauge(float xRef, float yRef, float radius, float min, float max, QString name, float value, const QColor& color, QPainter* painter, QPair<float, float> goodRange, QPair<float, float> criticalRange, bool solid)
{
    // Draw the circle
    QPen circlePen(Qt::SolidLine);

    // Rotate the whole gauge with this angle (in radians) for the zero position
    const float zeroRotation = 0.49f;

    // Scale the rotation so that the gauge does one revolution
    // per max. change
    const float rangeScale = ((2.0f * M_PI) / (max - min)) * 0.72f;

    const float nameHeight = radius / 2.5f;
    paintText(name.toUpper(), color, nameHeight*0.7f, xRef-radius, yRef-radius, painter);

    if (!solid)
    {
        circlePen.setStyle(Qt::DotLine);
    }
    circlePen.setWidth(refLineWidthToPen(radius/12.0f));
    circlePen.setColor(color);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(circlePen);
    drawCircle(xRef, yRef+nameHeight, radius, 0.0f, 170.0f, 1.0f, color, painter);

    QString label;
    label.sprintf("%05.1f", value);


    // Text
    // height
    const float textHeight = radius/2.0f;
    const float textX = xRef-radius/6.0f;
    const float textY = yRef+radius/2.0f;

    // Draw background rectangle
    // FIXME add background color for use in instrument panel
    QBrush brush(QColor(0, 0, 0), Qt::SolidPattern);
    painter->setBrush(brush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(refToScreenX(xRef-radius/2.5f), refToScreenY(yRef+nameHeight+radius/4.0f), refToScreenX(radius+radius/2.0f), refToScreenY((radius - radius/4.0f)*1.2f));

    // Draw good value and crit. value markers
    if (goodRange.first != goodRange.second)
    {
        QRectF rectangle(refToScreenX(xRef-radius/2.0f), refToScreenY(yRef+nameHeight-radius/2.0f), refToScreenX(radius*2.0f), refToScreenX(radius*2.0f));
        painter->setPen(Qt::green);
        //int start = ((goodRange.first*rangeScale+zeroRotation)/M_PI)*180.0f * 16.0f;// + 16.0f * 60.0f;
        //int span  = start - ((goodRange.second*rangeScale+zeroRotation)/M_PI)*180.0f * 16.0f;
        //painter->drawArc(rectangle, start, span);
    }

    if (criticalRange.first != criticalRange.second)
    {
        QRectF rectangle(refToScreenX(xRef-radius/2.0f-3.0f), refToScreenY(yRef+nameHeight-radius/2.0f-3.0f), refToScreenX(radius*2.0f), refToScreenX(radius*2.0f));
        painter->setPen(Qt::yellow);
        //int start = ((criticalRange.first*rangeScale+zeroRotation)/M_PI)*180.0f * 16.0f - 180.0f*16.0f;// + 16.0f * 60.0f;
        //int span  = start - ((criticalRange.second*rangeScale+zeroRotation)/M_PI)*180.0f * 16.0f + 180.0f*16.0f;
        //painter->drawArc(rectangle, start, span);
    }

    // Draw the value
    //painter->setPen(textColor);
    paintText(label, color, textHeight, textX, textY+nameHeight, painter);
    //paintText(label, color, ((radius - radius/3.0f)*1.1f), xRef-radius/2.5f, yRef+radius/3.0f, painter);

    // Draw the needle

    const float maxWidth = radius / 6.0f;
    const float minWidth = maxWidth * 0.3f;

    QPolygonF p(6);

    p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef+nameHeight+radius * 0.05f));
    p.replace(1, QPointF(xRef-minWidth/2.0f, yRef+nameHeight+radius * 0.9f));
    p.replace(2, QPointF(xRef+minWidth/2.0f, yRef+nameHeight+radius * 0.9f));
    p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef+nameHeight+radius * 0.05f));
    p.replace(4, QPointF(xRef,               yRef+nameHeight+radius * 0.0f));
    p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef+nameHeight+radius * 0.05f));


    rotatePolygonClockWiseRad(p, value*rangeScale+zeroRotation, QPointF(xRef, yRef+nameHeight));

    QBrush indexBrush;
    indexBrush.setColor(color);
    indexBrush.setStyle(Qt::SolidPattern);
    painter->setPen(Qt::NoPen);
    painter->setBrush(indexBrush);
    drawPolygon(p, painter);
}


void HDDisplay::drawSystemIndicator(float xRef, float yRef, int maxNum, float maxWidth, float maxHeight, QPainter* painter)
{
    if (values.size() > 0)
    {
        QString selectedKey = values.begin().key();
        //   | | | | | |
        //   | | | | | |
        //   x speed: 2.54

        // One column per value
        QMapIterator<QString, float> value(values);

        float x = xRef;
        float y = yRef;

        const float vspacing = 1.0f;
        float width = 1.5f;
        float height = 1.5f;
        const float hspacing = 0.6f;

        int i = 0;
        while (value.hasNext() && i < maxNum && x < maxWidth && y < maxHeight)
        {
            value.next();
            QBrush brush(Qt::SolidPattern);


            if (value.value() < 0.01f && value.value() > -0.01f)
            {
                brush.setColor(Qt::gray);
            }
            else if (value.value() > 0.01f)
            {
                brush.setColor(Qt::blue);
            }
            else
            {
                brush.setColor(Qt::yellow);
            }

            painter->setBrush(brush);
            painter->setPen(Qt::NoPen);

            // Draw current value colormap
            painter->drawRect(refToScreenX(x), refToScreenY(y), refToScreenX(width), refToScreenY(height));

            // Draw change rate colormap
            painter->drawRect(refToScreenX(x), refToScreenY(y+height+hspacing), refToScreenX(width), refToScreenY(height));

            // Draw mean value colormap
            painter->drawRect(refToScreenX(x), refToScreenY(y+2.0f*(height+hspacing)), refToScreenX(width), refToScreenY(height));

            // Add spacing
            x += width+vspacing;

            // Iterate
            i++;
        }

        // Draw detail label
        QString detail = "NO DATA AVAILABLE";

        if (values.contains(selectedKey))
        {
            detail = values.find(selectedKey).key();
            detail.append(": ");
            detail.append(QString::number(values.find(selectedKey).value()));
        }
        paintText(detail, QColor(255, 255, 255), 3.0f, xRef, yRef+3.0f*(height+hspacing)+1.0f, painter);
    }
}

void HDDisplay::drawChangeIndicatorGauge(float xRef, float yRef, float radius, float expectedMaxChange, float value, const QColor& color, QPainter* painter, bool solid)
{
    // Draw the circle
    QPen circlePen(Qt::SolidLine);
    if (!solid) circlePen.setStyle(Qt::DotLine);
    circlePen.setWidth(refLineWidthToPen(0.5f));
    circlePen.setColor(defaultColor);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(circlePen);
    drawCircle(xRef, yRef, radius, 200.0f, 170.0f, 1.0f, color, painter);

    QString label;
    label.sprintf("%05.1f", value);

    // Draw the value
    paintText(label, color, 4.5f, xRef-7.5f, yRef-2.0f, painter);

    // Draw the needle
    // Scale the rotation so that the gauge does one revolution
    // per max. change
    const float rangeScale = (2.0f * M_PI) / expectedMaxChange;
    const float maxWidth = radius / 10.0f;
    const float minWidth = maxWidth * 0.3f;

    QPolygonF p(6);

    p.replace(0, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(1, QPointF(xRef-minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(2, QPointF(xRef+minWidth/2.0f, yRef-radius * 0.9f));
    p.replace(3, QPointF(xRef+maxWidth/2.0f, yRef-radius * 0.5f));
    p.replace(4, QPointF(xRef,               yRef-radius * 0.46f));
    p.replace(5, QPointF(xRef-maxWidth/2.0f, yRef-radius * 0.5f));

    rotatePolygonClockWiseRad(p, value*rangeScale, QPointF(xRef, yRef));

    QBrush indexBrush;
    indexBrush.setColor(defaultColor);
    indexBrush.setStyle(Qt::SolidPattern);
    painter->setPen(Qt::SolidLine);
    painter->setPen(defaultColor);
    painter->setBrush(indexBrush);
    drawPolygon(p, painter);
}

/**
 * Paint text on top of the image and OpenGL drawings
 *
 * @param text chars to write
 * @param color text color
 * @param fontSize text size in mm
 * @param refX position in reference units (mm of the real instrument). This is relative to the measurement unit position, NOT in pixels.
 * @param refY position in reference units (mm of the real instrument). This is relative to the measurement unit position, NOT in pixels.
 */
void HDDisplay::paintText(QString text, QColor color, float fontSize, float refX, float refY, QPainter* painter)
{
    QPen prevPen = painter->pen();
    float pPositionX = refToScreenX(refX) - (fontSize*scalingFactor*0.072f);
    float pPositionY = refToScreenY(refY) - (fontSize*scalingFactor*0.212f);

    //QFont font("Bitstream Vera Sans");
    font.setPixelSize((int)(fontSize*scalingFactor*1.26f));

    QFontMetrics metrics = QFontMetrics(font);
    int border = qMax(4, metrics.leading());
    QRect rect = metrics.boundingRect(0, 0, width() - 2*border, int(height()*0.125),
                                      Qt::AlignLeft | Qt::TextWordWrap, text);
    painter->setPen(color);
    painter->setFont(font);
    painter->setRenderHint(QPainter::TextAntialiasing);
    painter->drawText(pPositionX, pPositionY,
                      rect.width(), rect.height(),
                      Qt::AlignCenter | Qt::TextWordWrap, text);
    painter->setPen(prevPen);
}

float HDDisplay::refLineWidthToPen(float line)
{
    return line * 2.50f;
}

void HDDisplay::updateValue(UASInterface* uas, QString name, double value, quint64 msec)
{
    if (this->uas == uas)
    {
        // Update mean
        const float oldMean = valuesMean.value(name, 0.0f);
        const int meanCount = valuesCount.value(name, 0);
        valuesMean.insert(name, (oldMean * meanCount +  value) / (meanCount + 1));
        valuesCount.insert(name, meanCount + 1);
        valuesDot.insert(name, (value - values.value(name, 0.0f)) / ((msec - lastUpdate.value(name, 0))/1000.0f));
        values.insert(name, value);
        lastUpdate.insert(name, msec);
    }
}

/**
 * @param y coordinate in pixels to be converted to reference mm units
 * @return the screen coordinate relative to the QGLWindow origin
 */
float HDDisplay::refToScreenX(float x)
{
    return (scalingFactor * x);
}

/**
 * @param x coordinate in pixels to be converted to reference mm units
 * @return the screen coordinate relative to the QGLWindow origin
 */
float HDDisplay::refToScreenY(float y)
{
    return (scalingFactor * y);
}

void HDDisplay::drawLine(float refX1, float refY1, float refX2, float refY2, float width, const QColor& color, QPainter* painter)
{
    QPen pen(Qt::SolidLine);
    pen.setWidth(refLineWidthToPen(width));
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawLine(QPoint(refToScreenX(refX1), refToScreenY(refY1)), QPoint(refToScreenX(refX2), refToScreenY(refY2)));
}

void HDDisplay::drawEllipse(float refX, float refY, float radiusX, float radiusY, float startDeg, float endDeg, float lineWidth, const QColor& color, QPainter* painter)
{
    QPen pen(painter->pen().style());
    pen.setWidth(refLineWidthToPen(lineWidth));
    pen.setColor(color);
    painter->setPen(pen);
    painter->drawEllipse(QPointF(refToScreenX(refX), refToScreenY(refY)), refToScreenX(radiusX), refToScreenY(radiusY));
}

void HDDisplay::drawCircle(float refX, float refY, float radius, float startDeg, float endDeg, float lineWidth, const QColor& color, QPainter* painter)
{
    drawEllipse(refX, refY, radius, radius, startDeg, endDeg, lineWidth, color, painter);
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
