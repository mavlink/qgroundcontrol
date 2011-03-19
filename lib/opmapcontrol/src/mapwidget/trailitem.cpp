/**
******************************************************************************
*
* @file       trailitem.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      A graphicsItem representing a trail point
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
*
*****************************************************************************/
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "trailitem.h"
#include <QDateTime>
namespace mapcontrol
{
    TrailItem::TrailItem(internals::PointLatLng const& coord,int const& altitude, QBrush color, QGraphicsItem* parent):QGraphicsItem(parent),coord(coord)
    {
        m_brush=color;
        QDateTime time=QDateTime::currentDateTime();
        QString coord_str = " " + QString::number(coord.Lat(), 'f', 6) + "   " + QString::number(coord.Lng(), 'f', 6);
        setToolTip(QString(tr("Position:")+"%1\n"+tr("Altitude:")+"%2\n"+tr("Time:")+"%3").arg(coord_str).arg(QString::number(altitude)).arg(time.toString()));
    }

    void TrailItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
      //  painter->drawRect(QRectF(-3,-3,6,6));
        painter->setBrush(m_brush);
        painter->drawEllipse(-2,-2,4,4);
    }
    QRectF TrailItem::boundingRect()const
    {
        return QRectF(-2,-2,4,4);
    }


    int TrailItem::type()const
    {
        return Type;
    }


}
