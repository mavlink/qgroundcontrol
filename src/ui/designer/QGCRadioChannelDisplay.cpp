#include "QGCRadioChannelDisplay.h"
#include <QPainter>
QGCRadioChannelDisplay::QGCRadioChannelDisplay(QWidget *parent) : QWidget(parent)

{
    m_showMinMax = false;
    m_min = 0;
    m_max = 1;
    m_value = 1500;
    m_orientation = Qt::Vertical;
    m_name = "Yaw";

    m_fillBrush = QBrush(Qt::green, Qt::SolidPattern);


}
void QGCRadioChannelDisplay::setName(QString name)
{
    m_name = name;
    update();
}

void QGCRadioChannelDisplay::setOrientation(Qt::Orientation orient)
{
    m_orientation = orient;
    update();
}
void QGCRadioChannelDisplay::paintEvent(QPaintEvent *event)
{
    //Values range from 0-3000.
    //1500 is the middle, static servo value.
    QPainter painter(this);

    int fontHeight = painter.fontMetrics().height();
    int twiceFontHeight = fontHeight * 2;

    painter.setBrush(Qt::Dense4Pattern);
    painter.setPen(QColor::fromRgb(128,128,64));

    int curVal = m_value;
    if (curVal > m_max)  {
        curVal = m_max;
    }
    if (curVal < m_min) {
        curVal = m_min;
    }

    if (m_orientation == Qt::Vertical)
    {
        //draw border
        painter.drawRect(0,0,width()-1,(height()-1) - twiceFontHeight);
        painter.setPen(QColor::fromRgb(50,255,50));
        painter.setBrush(Qt::SolidPattern);

        //draw the text value of the widget, and its label
        QString valStr = QString::number(m_value);
        painter.setPen(QColor::fromRgb(255,255,255));
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(m_name)/2.0),((height()-3) - (fontHeight*1)),m_name);
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(valStr)/2.0),((height()-3) - (fontHeight * 0)),valStr);


        painter.setPen(QColor::fromRgb(128,128,64));
        painter.setBrush(m_fillBrush);

        if (!m_showMinMax) {
            //draw just the value
            int newval = (height()-2-twiceFontHeight) * ((float)(curVal - m_min) / ((m_max-m_min)+1));
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

            int yMax = (height()-3 - twiceFontHeight) - (((height()-3-twiceFontHeight) * ((float)m_max / 3000.0)));
            int yMin = (height()-3 - twiceFontHeight) - (((height()-3-twiceFontHeight) * ((float)m_min / 3000.0)));
            painter.drawRect(2,yMax,width()-3,yMin - yMax);

            //draw min and max labels
            QString minstr = QString::number(m_min);
            painter.drawText((width() / 2.0) - (painter.fontMetrics().width("min")/2.0),yMin,"min");
            painter.drawText((width() / 2.0) - (painter.fontMetrics().width(minstr)/2.0),yMin + fontHeight,minstr);

            QString maxstr = QString::number(m_max);
            painter.drawText((width() / 2.0) - (painter.fontMetrics().width("max")/2.0),yMax,"max");
            painter.drawText((width() / 2.0) - (painter.fontMetrics().width(maxstr)/2.0),yMax + fontHeight,maxstr);

        }
    }
    else //horizontal orientation
    {
        //draw the value
        painter.drawRect(0,0,width()-1,(height()-1) - twiceFontHeight);
        painter.setPen(QColor::fromRgb(50,255,50));
        painter.setBrush(m_fillBrush);

        //draw the value string
        painter.setPen(QColor::fromRgb(255,255,255));
        QString valstr = QString::number(m_value);
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(m_name)/2.0),((height()-3) - (fontHeight*1)),m_name);
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(valstr)/2.0),((height()-3) - (fontHeight * 0)),valstr);


        painter.setPen(QColor::fromRgb(0,128,0));
        painter.setBrush(m_fillBrush);

        if (!m_showMinMax) {
            //draw just the value
            painter.drawRect(1,1,(width()-3) * ((float)(curVal-m_min) / (m_max-m_min)),(height()-3) - twiceFontHeight);
        }
        else {
            //draw the value
            painter.drawRect(1,1,(width()-3) * ((float)curVal / 3000.0),(height()-3) - twiceFontHeight);

             //draw the min and max bars
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QColor::fromRgb(255,0,0));
            painter.drawRect(width() * ((float)m_min / 3000.0),2,((width()-1) * ((float)m_max / 3000.0)) - (width() * ((float)m_min / 3000.0)),(height()-5) - twiceFontHeight);

            //draw the min and max strings
            QString minstr = QString::number(m_min);
            painter.drawText((width() * ((float)m_min / 3000.0)) - (painter.fontMetrics().width("min")/2.0),((height()-3) - (painter.fontMetrics().height()*1)),"min");
            painter.drawText((width() * ((float)m_min / 3000.0)) - (painter.fontMetrics().width(minstr)/2.0),((height()-3) - (painter.fontMetrics().height() * 0)),minstr);

            QString maxstr = QString::number(m_max);
            painter.drawText((width() * ((float)m_max / 3000.0)) - (painter.fontMetrics().width("max")/2.0),((height()-3) - (painter.fontMetrics().height()*1)),"max");
            painter.drawText((width() * ((float)m_max / 3000.0)) - (painter.fontMetrics().width(maxstr)/2.0),((height()-3) - (painter.fontMetrics().height() * 0)),maxstr);
        }
    }
}

void QGCRadioChannelDisplay::setValue(int value)
{
    if (value < 0)
    {
        m_value = 0;
    }
    else if (value > 3000)
    {
        m_value = 3000;
    }
    else
    {
        m_value = value;
    }
    update();
}

void QGCRadioChannelDisplay::showMinMax()
{
    m_showMinMax = true;
    update();
}

void QGCRadioChannelDisplay::hideMinMax()
{
    m_showMinMax = false;
    update();
}

void QGCRadioChannelDisplay::setMin(int value)
{
    m_min = value;
    if (m_min == m_max)
    {
        m_min--;
    }
    update();
}

void QGCRadioChannelDisplay::setMax(int value)
{
    m_max = value;
    if (m_min == m_max)
    {
        m_max++;
    }
    update();
}
