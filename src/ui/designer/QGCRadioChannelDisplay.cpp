#include "QGCRadioChannelDisplay.h"
#include <QPainter>

#define DIMMEST_COLOR   QColor::fromRgb(0,100,0)
#define MIDBRIGHT_COLOR   QColor::fromRgb(0,180,0)
#define BRIGHTEST_COLOR   QColor::fromRgb(64,255,0)

QGCRadioChannelDisplay::QGCRadioChannelDisplay(QWidget *parent) :
    QGroupBox(parent),
    _orientation(Qt::Horizontal),
    _value(_centerValue),
    _min(_centerValue),
    _max(_centerValue),
    _trim(_centerValue),
    _showMinMax(false),
    _showTrim(false)
{

}

void QGCRadioChannelDisplay::setName(QString name)
{
    _name = name;
    setTitle(_name);
}

void QGCRadioChannelDisplay::setOrientation(Qt::Orientation orient)
{
    _orientation = orient;
    update();
}

void QGCRadioChannelDisplay::paintEvent(QPaintEvent *event)
{
    QGroupBox::paintEvent(event);
    
    //Values range from 0-3000.
    //1500 is the middle, static servo value.
    QPainter painter(this);

    int fontHeight = painter.fontMetrics().height();
    int rowHeigth = fontHeight + 2;
    int twiceFontHeight = fontHeight * 2;

    painter.setBrush(Qt::Dense4Pattern);
    painter.setPen(QColor::fromRgb(128,128,64));

    int curVal = _value;
    if (curVal > _maxRange)  {
        curVal = _maxRange;
    }
    if (curVal < _minRange) {
        curVal = _minRange;
    }

    if (_orientation == Qt::Vertical)
    {
        QLinearGradient gradientBrush(0, 0, this->width(), this->height());
        gradientBrush.setColorAt(1.0,DIMMEST_COLOR);
        gradientBrush.setColorAt(0.5,MIDBRIGHT_COLOR);
        gradientBrush.setColorAt(0.0, BRIGHTEST_COLOR);

        //draw border
        painter.drawRect(0,0,width()-1,(height()-1) - twiceFontHeight);
        painter.setPen(QColor::fromRgb(50,255,50));
        painter.setBrush(Qt::SolidPattern);

        //draw the text value of the widget, and its label
        painter.setPen(QColor::fromRgb(255,255,255));
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(_name)/2.0),((height()-3) - (fontHeight*1)),_name);

        if (isEnabled()) {
            QString valStr = QString::number(_value);
            painter.drawText((width()/2.0) - (painter.fontMetrics().width(valStr)/2.0),((height()-3) - (fontHeight * 0)),valStr);

            painter.setPen(QColor::fromRgb(128,128,64));
            painter.setBrush(gradientBrush);

            if (!_showMinMax) {
                //draw just the value
                int newval = (height()-2-twiceFontHeight) * ((float)(curVal - _min) / ((_max-_min)+1));
                int yVal = (height()-2-twiceFontHeight) - newval;
                painter.drawRect(1,yVal,width()-3,((height()-2) - yVal - twiceFontHeight));
            }
            else {
                //draw the value
                int newval = (height()-2-twiceFontHeight) * ((float)(curVal / 3001.0));
                int yVal = (height()-2-twiceFontHeight) - newval;
                painter.drawRect(1,yVal,width()-3,((height()-2) - yVal - twiceFontHeight));

                //draw min max indicator bars
                painter.setPen(QColor::fromRgb(255,0,0));
                painter.setBrush(Qt::NoBrush);

                int yMax = (height()-3 - twiceFontHeight) - (((height()-3-twiceFontHeight) * ((float)_max / 3000.0)));
                int yMin = (height()-3 - twiceFontHeight) - (((height()-3-twiceFontHeight) * ((float)_min / 3000.0)));
                painter.drawRect(2,yMax,width()-3,yMin - yMax);

                //draw min and max labels
                QString minstr = QString::number(_min);
                painter.drawText((width() / 2.0) - (painter.fontMetrics().width("min")/2.0),yMin,"min");
                painter.drawText((width() / 2.0) - (painter.fontMetrics().width(minstr)/2.0),yMin + fontHeight,minstr);

                QString maxstr = QString::number(_max);
                painter.drawText((width() / 2.0) - (painter.fontMetrics().width("max")/2.0),yMax,"max");
                painter.drawText((width() / 2.0) - (painter.fontMetrics().width(maxstr)/2.0),yMax + fontHeight,maxstr);

            }
        }
    }
    else //horizontal orientation
    {
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
                // Draw the Min numeric value display to the left
                painter.drawText(0, rowHeigth, minMaxDisplayWidth, fontHeight, Qt::AlignHCenter | Qt::AlignBottom, "Min");
                painter.drawText(0, rowHeigth * 2, minMaxDisplayWidth, fontHeight, Qt::AlignHCenter | Qt::AlignBottom, QString::number(_min));

                // Draw the Max numeric value display to the right
                painter.drawText(width() - minMaxDisplayWidth, rowHeigth, minMaxDisplayWidth, fontHeight, Qt::AlignHCenter | Qt::AlignBottom, "Max");
                painter.drawText(width() - minMaxDisplayWidth, rowHeigth * 2, minMaxDisplayWidth, fontHeight, Qt::AlignHCenter | Qt::AlignBottom, QString::number(_max));
                
                // Draw the Min/Max tick marks on the axis
                int xTick = rcValueAxis.left() + (rcValueAxis.width() * ((float)(_min-_minRange) / (_maxRange-_minRange)));
                painter.drawLine(xTick, rcValueAxis.top(), xTick, rcValueAxis.bottom());
                xTick = rcValueAxis.left() + (rcValueAxis.width() * ((float)(_max-_minRange) / (_maxRange-_minRange)));
                painter.drawLine(xTick, rcValueAxis.top(), xTick, rcValueAxis.bottom());
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
    }
}

void QGCRadioChannelDisplay::setValue(int value)
{
    if (value < 0)
    {
        _value = 0;
    }
    else if (value > 3000)
    {
        _value = 3000;
    }
    else
    {
        _value = value;
    }
    update();
}

void QGCRadioChannelDisplay::showMinMax(bool show)
{
    _showMinMax = show;
    update();
}

void QGCRadioChannelDisplay::showTrim(bool show)
{
    _showTrim = show;
    update();
}

void QGCRadioChannelDisplay::setValueAndRange(int val, int min, int max)
{
    setValue(val);
    setMinMax(min,max);
}

void QGCRadioChannelDisplay::setMinMax(int min, int max)
{
    _min = min;
    _max = max;
    update();
}

void QGCRadioChannelDisplay::setMin(int value)
{
    _min = value;
    if (_min == _max)
    {
        _min--;
    }
    update();
}

void QGCRadioChannelDisplay::setMax(int value)
{
    _max = value;
    if (_min == _max)
    {
        _max++;
    }
    update();
}

void QGCRadioChannelDisplay::setTrim(int value)
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
void QGCRadioChannelDisplay::_drawValuePointer(QPainter* painter, int xTip, int yTip, int height, bool rightSideUp)
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
