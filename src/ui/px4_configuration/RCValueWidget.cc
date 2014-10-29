/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "RCValueWidget.h"

#include <QPainter>
#include <QDebug>

#define DIMMEST_COLOR   QColor::fromRgb(0,100,0)
#define MIDBRIGHT_COLOR   QColor::fromRgb(0,180,0)
#define BRIGHTEST_COLOR   QColor::fromRgb(64,255,0)

RCValueWidget::RCValueWidget(QWidget *parent) :
    QWidget(parent),
    _smallMode(false),
    _value(_centerValue),
    _min(_centerValue),
    _max(_centerValue),
    _trim(_centerValue),
    _minValid(false),
    _maxValid(false),
    _showMinMax(false),
    _showTrim(false),
    _fgColor(255, 255, 255)
{
    setAutoFillBackground(true);
}

/// @brief Draws the control
void RCValueWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    
    Q_UNUSED(event);
    
    int curVal = _value;
    if (curVal > _maxRange)  {
        curVal = _maxRange;
    }
    if (curVal < _minRange) {
        curVal = _minRange;
    }
    
    QPen pen(_fgColor);
    pen.setWidth(1);
    painter.setPen(pen);
    
    if (_smallMode) {
        painter.drawLine(0, height() / 2, width(), height() / 2);
    } else {
        // The value bar itself it centered in the control
        QRect rectValueBar(0, (rect().height() - _barHeight)/2, rect().width() - 1, _barHeight);
        
        painter.drawRoundedRect(rectValueBar, _barHeight/2, _barHeight/2);
    }
    
    // Draw the RC value circle
    int radius = _smallMode ? 4 : _barHeight;
    int curValNormalized;
    if (_reversed) {
        curValNormalized = _centerValue - (curVal - _centerValue);
    } else {
        curValNormalized = curVal;
    }
    QPoint  ptCenter(width() * ((float)(curValNormalized-_minRange) / (_maxRange-_minRange)), height() / 2);
    QBrush brush(QColor(128, 128, 128));
    painter.setBrush(brush);
    painter.drawEllipse(ptCenter, radius, radius);
    
#if 0
    int fontHeight = painter.fontMetrics().height();
    int rowHeigth = fontHeight + 2;

    painter.setBrush(Qt::Dense4Pattern);
    painter.setPen(QColor::fromRgb(128,128,64));

    int curVal = _value;
    if (curVal > _maxRange)  {
        curVal = _maxRange;
    }
    if (curVal < _minRange) {
        curVal = _minRange;
    }

    if (isEnabled()) {
        QLinearGradient hGradientBrush(0, 0, this->width(), this->height());
        hGradientBrush.setColorAt(0.0,DIMMEST_COLOR);
        hGradientBrush.setColorAt(0.5,MIDBRIGHT_COLOR);
        hGradientBrush.setColorAt(1.0, BRIGHTEST_COLOR);
        
        // Calculate how much horizontal space we need to show a min/max value. We must be able to display four numeric
        // digits for the rc value, plus we add another digit for spacing.
        int minMaxDisplayWidth = painter.fontMetrics().width("00000");

        // Draw the value axis line with center and end point tick marks. We need to leave enough space on the left and the right
        // for the Min/Max value displays.
        QRect rcValueAxis(minMaxDisplayWidth, rowHeigth * 2, width() - (minMaxDisplayWidth * 2), rowHeigth);
        int yLine = rcValueAxis.y() + rcValueAxis.height() / 2;
        painter.drawLine(rcValueAxis.left(), yLine, rcValueAxis.right(), yLine);
        painter.drawLine(rcValueAxis.left(), rcValueAxis.top(), rcValueAxis.left(), rcValueAxis.bottom());
        painter.drawLine(rcValueAxis.right(), rcValueAxis.top(), rcValueAxis.right(), rcValueAxis.bottom());
        painter.drawLine(rcValueAxis.left() + rcValueAxis.width() / 2, rcValueAxis.top(), rcValueAxis.left() + rcValueAxis.width() / 2, rcValueAxis.bottom());
        
        painter.setPen(QColor::fromRgb(50,255,50));
        painter.setBrush(hGradientBrush);

        if (_showMinMax) {
            QString text;
            
            // Draw the Min numeric value display to the left
            painter.drawText(0, rowHeigth, minMaxDisplayWidth, fontHeight, Qt::AlignHCenter | Qt::AlignBottom, "Min");
            if (_minValid) {
                text = QString::number(_min);
            } else {
                text = "----";
            }
            painter.drawText(0, rowHeigth * 2, minMaxDisplayWidth, fontHeight, Qt::AlignHCenter | Qt::AlignBottom, text);

            // Draw the Max numeric value display to the right
            painter.drawText(width() - minMaxDisplayWidth, rowHeigth, minMaxDisplayWidth, fontHeight, Qt::AlignHCenter | Qt::AlignBottom, "Max");
            if (_maxValid) {
                text = QString::number(_max);
            } else {
                text = QString::number(_max);
            }
            painter.drawText(width() - minMaxDisplayWidth, rowHeigth * 2, minMaxDisplayWidth, fontHeight, Qt::AlignHCenter | Qt::AlignBottom, text);
            
            // Draw the Min/Max tick marks on the axis
            int xTick;
            if (_minValid) {
                int xTick = rcValueAxis.left() + (rcValueAxis.width() * ((float)(_min-_minRange) / (_maxRange-_minRange)));
                painter.drawLine(xTick, rcValueAxis.top(), xTick, rcValueAxis.bottom());
            }
            if (_maxValid) {
                xTick = rcValueAxis.left() + (rcValueAxis.width() * ((float)(_max-_minRange) / (_maxRange-_minRange)));
                painter.drawLine(xTick, rcValueAxis.top(), xTick, rcValueAxis.bottom());
            }
        }
        
        if (_showTrim) {
            // Draw the Trim value pointer
            _drawValuePointer(&painter,
                              rcValueAxis.left() + (rcValueAxis.width() * ((float)(_trim-_minRange) / (_maxRange-_minRange))),      // x position for tip of triangle
                              (rowHeigth * 2) + (rowHeigth / 2) - 1,                                                                // y position for tip of triangle
                              rowHeigth / 2,                                                                                        // heigth of triangle
                              false);                                                                                               // draw upside down
            
            // Draw the Trim value label
            QString trimStr = tr("Trim %1").arg(QString::number(_trim));
            
            int trimTextWidth = painter.fontMetrics().width(trimStr);
            
            painter.drawText(rcValueAxis.left() + (rcValueAxis.width() * ((float)(_trim-_minRange) / (_maxRange-_minRange))) - (trimTextWidth / 2),
                             rowHeigth,
                             trimTextWidth,
                             fontHeight,
                             Qt::AlignLeft | Qt::AlignBottom,
                             trimStr);
        }
        
        // Draw the RC value pointer
        _drawValuePointer(&painter,
                          rcValueAxis.left() + (rcValueAxis.width() * ((float)(curVal-_minRange) / (_maxRange-_minRange))),     // x position for tip of triangle
                          (rowHeigth * 2) + (rowHeigth / 2) + 1,                                                                // y position for tip of triangle
                          rowHeigth / 2,                                                                                        // heigth of triangle
                          true);                                                                                                // draw right side up

        // Draw the control value
        QString valueStr = QString::number(_value);
        
        int valueTextWidth = painter.fontMetrics().width(valueStr);
        
        painter.drawText(rcValueAxis.left() + (rcValueAxis.width() * ((float)(_value-_minRange) / (_maxRange-_minRange))) - (valueTextWidth / 2),
                         rowHeigth * 3,
                         valueTextWidth,
                         fontHeight,
                         Qt::AlignLeft | Qt::AlignBottom,
                         valueStr);

        painter.setPen(QColor::fromRgb(0,128,0));
        painter.setBrush(hGradientBrush);
    } else {
        painter.setPen(QColor(Qt::gray));
        painter.drawText(0, 0, width(), height(), Qt::AlignHCenter | Qt::AlignVCenter, tr("not available"));
    }
#endif
}

void RCValueWidget::setValue(int value)
{
    _value = value;
    update();
}

void RCValueWidget::showMinMax(bool show)
{
    _showMinMax = show;
    update();
}

void RCValueWidget::showTrim(bool show)
{
    _showTrim = show;
    update();
}

void RCValueWidget::setValueAndMinMax(int val, int min, int max)
{
    setValue(val);
    setMinMax(min,max);
}

void RCValueWidget::setMinMax(int min, int max)
{
    _min = min;
    _max = max;
    update();
}

void RCValueWidget::setMin(int min)
{
    _min = min;
    update();
}

void RCValueWidget::setMax(int max)
{
    _max = max;
    update();
}

void RCValueWidget::setTrim(int value)
{
    _trim = value;
    update();
}

/// @brief Draw rc value pointer triangle.
///     @param painter Draw using this painter
///     @param x X position for tip of triangle
///     @param y Y position for tip of triangle
///     @param heigth Height of triangle
///     @param rightSideUp true: draw triangle right side up, false: draw triangle upside down
void RCValueWidget::_drawValuePointer(QPainter* painter, int xTip, int yTip, int height, bool rightSideUp)
{
    QPointF trianglePoints[3];
    
    // Topmost tip of triangle points to value
    trianglePoints[0].setX(xTip);
    trianglePoints[0].setY(yTip);
    
    int yBottom;
    if (rightSideUp) {
        yBottom = yTip + height;
    } else {
        yBottom = yTip - height;
    }
    
    // Right bottom tip of triangle
    trianglePoints[1].setX(xTip + (height * 0.75));
    trianglePoints[1].setY(yBottom);
    
    // Left bottom tip of triangle
    trianglePoints[2].setX(xTip - (height * 0.75));
    trianglePoints[2].setY(yBottom);
    
    painter->drawPolygon (trianglePoints, 3);
}

/// @brief Set whether the Min range value is valid or not.
void RCValueWidget::setMinValid(bool valid)
{
    _minValid = valid;
    update();
}

/// @brief Set whether the Max range value is valid or not.
void RCValueWidget::setMaxValid(bool valid)
{
    _maxValid = valid;
    update();
}
