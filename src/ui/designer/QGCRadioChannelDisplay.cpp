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

    int currval = m_value;
    if (currval > m_max)
    {
        currval = m_max;
    }
    if (currval < m_min)
    {
        currval = m_min;
    }

    if (m_orientation == Qt::Vertical)
    {
        painter.drawRect(0,0,width()-1,(height()-1) - (painter.fontMetrics().height() * 2));
        painter.setBrush(Qt::SolidPattern);
        painter.setPen(QColor::fromRgb(50,255,50));
        //m_value - m_min / m_max - m_min

        if (!m_showMinMax)
        {
            int newval = (height()-2-(painter.fontMetrics().height() * 2)) * ((float)(currval - m_min) / ((m_max-m_min)+1));
            int newvaly = (height()-2-(painter.fontMetrics().height() * 2)) - newval;
            painter.drawRect(1,newvaly,width()-3,((height()-2) - newvaly - (painter.fontMetrics().height() * 2)));
        }
        else
        {
            int newval = (height()-2-(painter.fontMetrics().height() * 2)) * ((float)(currval / 3001.0));
            int newvaly = (height()-2-(painter.fontMetrics().height() * 2)) - newval;
            painter.drawRect(1,newvaly,width()-3,((height()-2) - newvaly - (painter.fontMetrics().height() * 2)));
        }

        QString valstr = QString::number(m_value);
        painter.setPen(QColor::fromRgb(255,255,255));
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(m_name)/2.0),((height()-3) - (painter.fontMetrics().height()*1)),m_name);
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(valstr)/2.0),((height()-3) - (painter.fontMetrics().height() * 0)),valstr);
        if (m_showMinMax)
        {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QColor::fromRgb(255,0,0));
            int maxyval = (height()-3 - (painter.fontMetrics().height() * 2)) - (((height()-3-(painter.fontMetrics().height() * 2)) * ((float)m_max / 3000.0)));
            int minyval = (height()-3 - (painter.fontMetrics().height() * 2)) - (((height()-3-(painter.fontMetrics().height() * 2)) * ((float)m_min / 3000.0)));
            painter.drawRect(2,maxyval,width()-3,minyval - maxyval);
            QString minstr = QString::number(m_min);
            painter.drawText((width() / 2.0) - (painter.fontMetrics().width("min")/2.0),minyval,"min");
            painter.drawText((width() / 2.0) - (painter.fontMetrics().width(minstr)/2.0),minyval + painter.fontMetrics().height(),minstr);

            QString maxstr = QString::number(m_max);
            painter.drawText((width() / 2.0) - (painter.fontMetrics().width("max")/2.0),maxyval,"max");
            painter.drawText((width() / 2.0) - (painter.fontMetrics().width(maxstr)/2.0),maxyval + painter.fontMetrics().height(),maxstr);

            //painter.drawRect(width() * ,2,((width()-1) * ((float)m_max / 3000.0)) - (width() * ((float)m_min / 3000.0)),(height()-5) - (painter.fontMetrics().height() * 2));
        }
    }
    else
    {
        painter.drawRect(0,0,width()-1,(height()-1) - (painter.fontMetrics().height() * 2));
        painter.setBrush(Qt::SolidPattern);
        painter.setPen(QColor::fromRgb(50,255,50));
        if (!m_showMinMax)
        {
            painter.drawRect(1,1,(width()-3) * ((float)(currval-m_min) / (m_max-m_min)),(height()-3) - (painter.fontMetrics().height() * 2));
        }
        else
        {
            painter.drawRect(1,1,(width()-3) * ((float)currval / 3000.0),(height()-3) - (painter.fontMetrics().height() * 2));
        }
        painter.setPen(QColor::fromRgb(255,255,255));
        QString valstr = QString::number(m_value);
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(m_name)/2.0),((height()-3) - (painter.fontMetrics().height()*1)),m_name);
        painter.drawText((width()/2.0) - (painter.fontMetrics().width(valstr)/2.0),((height()-3) - (painter.fontMetrics().height() * 0)),valstr);
        if (m_showMinMax)
        {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QColor::fromRgb(255,0,0));
            painter.drawRect(width() * ((float)m_min / 3000.0),2,((width()-1) * ((float)m_max / 3000.0)) - (width() * ((float)m_min / 3000.0)),(height()-5) - (painter.fontMetrics().height() * 2));

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
