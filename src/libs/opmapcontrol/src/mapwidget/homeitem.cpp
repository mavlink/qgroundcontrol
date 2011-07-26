/**
******************************************************************************
*
* @file       homeitem.cpp
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
#include "homeitem.h"
namespace mapcontrol
{
    HomeItem::HomeItem(MapGraphicItem* map,OPMapWidget* parent):safe(true),map(map),mapwidget(parent),showsafearea(true),safearea(1000),altitude(0)
    {
        pic.load(QString::fromUtf8(":/markers/images/home2.svg"));
        pic=pic.scaled(30,30,Qt::IgnoreAspectRatio);
        this->setFlag(QGraphicsItem::ItemIgnoresTransformations,true);
        localposition=map->FromLatLngToLocal(mapwidget->CurrentPosition());
        this->setPos(localposition.X(),localposition.Y());
        this->setZValue(4);
        coord=internals::PointLatLng(50,50);

//        this->setFlag(QGraphicsItem::ItemIsMovable,true);
//        this->setFlag(QGraphicsItem::ItemIgnoresTransformations,true);
//        this->setFlag(QGraphicsItem::ItemIsSelectable,true);
    }

    void HomeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
        painter->drawPixmap(-pic.width()/2,-pic.height()/2,pic);
        if(showsafearea)
        {
            if(safe)
                painter->setPen(Qt::green);
            else
                painter->setPen(Qt::red);
            painter->drawEllipse(QPointF(0,0),localsafearea,localsafearea);
            //   painter->drawRect(QRectF(-localsafearea,-localsafearea,localsafearea*2,localsafearea*2));
        }

    }
    QRectF HomeItem::boundingRect()const
    {
        if(!showsafearea)
            return QRectF(-pic.width()/2,-pic.height()/2,pic.width(),pic.height());
        else
            return QRectF(-localsafearea,-localsafearea,localsafearea*2,localsafearea*2);
    }


    int HomeItem::type()const
    {
        return Type;
    }
    void HomeItem::RefreshPos()
    {
        prepareGeometryChange();
        localposition=map->FromLatLngToLocal(coord);
        this->setPos(localposition.X(),localposition.Y());
        if(showsafearea)
            localsafearea=safearea/map->Projection()->GetGroundResolution(map->ZoomTotal(),coord.Lat());

    }

//    void HomeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
//    {
//        if(event->button()==Qt::LeftButton)
//        {
//            coord=map->FromLocalToLatLng(this->pos().x(),this->pos().y());
//            QString coord_str = " " + QString::number(coord.Lat(), 'f', 6) + "   " + QString::number(coord.Lng(), 'f', 6);
//            qDebug() << "WP MOVE:" << coord_str << __FILE__ << __LINE__;
//            isDragging=false;
//            RefreshToolTip();

//            emit WPValuesChanged(this);
//        }
//        QGraphicsItem::mouseReleaseEvent(event);
//    }

}
