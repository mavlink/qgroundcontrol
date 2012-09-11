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
#include "traillineitem.h"

namespace mapcontrol
{
    TrailLineItem::TrailLineItem(internals::PointLatLng const& coord1,internals::PointLatLng const& coord2, QBrush color, QGraphicsItem* parent):QGraphicsLineItem(parent),coord1(coord1),coord2(coord2)
    {
        m_brush=color;
        QPen pen;
        pen.setBrush(m_brush);
        pen.setWidth(1);
        this->setPen(pen);
    }
/*
    void TrailLineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
      //  painter->drawRect(QRectF(-3,-3,6,6));
        painter->setBrush(m_brush);
        QPen pen;
        pen.setBrush(m_brush);
        pen.setWidth(2);
        painter->drawLine(this->line().x1(),this->line().y1(),this->line().x2(),this->line().y2());
    }
*/
    int TrailLineItem::type()const
    {
        return Type;
    }


}
