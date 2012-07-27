/**
******************************************************************************
*
* @file       trailitem.h
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      A graphicsItem representing a WayPoint
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
#ifndef TRAILITEM_H
#define TRAILITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include <QLabel>
#include "../internals/pointlatlng.h"
#include <QObject>

namespace mapcontrol
{

    class TrailItem:public QObject,public QGraphicsItem
    {
        Q_OBJECT
        Q_INTERFACES(QGraphicsItem)
    public:
                enum { Type = UserType + 3 };
        TrailItem(internals::PointLatLng const& coord,int const& altitude, QBrush color, QGraphicsItem* parent);
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                    QWidget *widget);
        QRectF boundingRect() const;
        int type() const;
        internals::PointLatLng coord;
    private:
        QBrush m_brush;


    public slots:

    signals:

    };
}
#endif // TRAILITEM_H


