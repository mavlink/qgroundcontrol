/**
******************************************************************************
*
* @file       gpsitem.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      A graphicsItem representing a UAV
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
#include "../internals/pureprojection.h"
#include "gpsitem.h"
namespace mapcontrol
{
    GPSItem::GPSItem(MapGraphicItem* map,OPMapWidget* parent,QString uavPic):map(map),mapwidget(parent),showtrail(true),showtrailline(true),trailtime(5),traildistance(50),autosetreached(true)
    ,autosetdistance(100)
    {
        pic.load(uavPic);
       // Don't scale but trust the image we are given
       // pic=pic.scaled(50,33,Qt::IgnoreAspectRatio);
        localposition=map->FromLatLngToLocal(mapwidget->CurrentPosition());
        this->setPos(localposition.X(),localposition.Y());
        this->setZValue(4);
        trail=new QGraphicsItemGroup();
        trail->setParentItem(map);
        trailLine=new QGraphicsItemGroup();
        trailLine->setParentItem(map);
        this->setFlag(QGraphicsItem::ItemIgnoresTransformations,true);
        mapfollowtype=UAVMapFollowType::None;
        trailtype=UAVTrailType::ByDistance;
        timer.start();
    }
    GPSItem::~GPSItem()
    {
        delete trail;
    }

    void GPSItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option);
        Q_UNUSED(widget);
       // painter->rotate(-90);
        painter->drawPixmap(-pic.width()/2,-pic.height()/2,pic);
       //   painter->drawRect(QRectF(-pic.width()/2,-pic.height()/2,pic.width()-1,pic.height()-1));
    }
    QRectF GPSItem::boundingRect()const
    {
        return QRectF(-pic.width()/2,-pic.height()/2,pic.width(),pic.height());
    }
    void GPSItem::SetUAVPos(const internals::PointLatLng &position, const int &altitude)
    {
        if(coord.IsEmpty())
            lastcoord=coord;
        if(coord!=position)
        {

            if(trailtype==UAVTrailType::ByTimeElapsed)
            {
                if(timer.elapsed()>trailtime*1000)
                {
                    trail->addToGroup(new TrailItem(position,altitude,Qt::green,this));
                    if(!lasttrailline.IsEmpty())
                        trailLine->addToGroup((new TrailLineItem(lasttrailline,position,Qt::green,map)));
                    lasttrailline=position;
                    timer.restart();
                }

            }
            else if(trailtype==UAVTrailType::ByDistance)
            {
                if(qAbs(internals::PureProjection::DistanceBetweenLatLng(lastcoord,position)*1000)>traildistance)
                {
                    trail->addToGroup(new TrailItem(position,altitude,Qt::green,this));
                    if(!lasttrailline.IsEmpty())

                        trailLine->addToGroup((new TrailLineItem(lasttrailline,position,Qt::green,this)));
                    lasttrailline=position;
                    lastcoord=position;
                }
            }
            coord=position;
            this->altitude=altitude;
            RefreshPos();
            /*if(mapfollowtype==UAVMapFollowType::CenterAndRotateMap||mapfollowtype==UAVMapFollowType::CenterMap)
            {
                mapwidget->SetCurrentPosition(coord);
            }*/
            this->update();
            /*if(autosetreached)
            {
                foreach(QGraphicsItem* i,map->childItems())
                {
                    WayPointItem* wp=qgraphicsitem_cast<WayPointItem*>(i);
                    if(wp)
                    {
                        if(Distance3D(wp->Coord(),wp->Altitude())<autosetdistance)
                        {
                            wp->SetReached(true);
                            emit UAVReachedWayPoint(wp->Number(),wp);
                        }
                    }
                }
            }
            if(mapwidget->Home!=0)
            {
                //verify if the UAV is inside the safety bouble
                if(Distance3D(mapwidget->Home->Coord(),mapwidget->Home->Altitude())>mapwidget->Home->SafeArea())
                {
                    if(mapwidget->Home->safe!=false)
                    {
                        mapwidget->Home->safe=false;
                        mapwidget->Home->update();
                        emit UAVLeftSafetyBouble(this->coord);
                    }
                }
                else
                {
                    if(mapwidget->Home->safe!=true)
                    {
                        mapwidget->Home->safe=true;
                        mapwidget->Home->update();
                    }
                }

            }*/
        }
    }

    /**
      * Rotate the UAV Icon on the map, or rotate the map
      * depending on the display mode
      */
    void GPSItem::SetUAVHeading(const qreal &value)
    {
        if(mapfollowtype==UAVMapFollowType::CenterAndRotateMap)
        {
            mapwidget->SetRotate(-value);
        }
        else {
            if (this->rotation() != value)
                this->setRotation(value);
        }
    }


    int GPSItem::type()const
    {
        return Type;
    }


    void GPSItem::RefreshPos()
    {
        localposition=map->FromLatLngToLocal(coord);
        this->setPos(localposition.X(),localposition.Y());
        foreach(QGraphicsItem* i,trail->childItems())
        {
            TrailItem* w=qgraphicsitem_cast<TrailItem*>(i);
            if(w)
                w->setPos(map->FromLatLngToLocal(w->coord).X(),map->FromLatLngToLocal(w->coord).Y());
        }
        foreach(QGraphicsItem* i,trailLine->childItems())
        {
            TrailLineItem* ww=qgraphicsitem_cast<TrailLineItem*>(i);
            if(ww)
                ww->setLine(map->FromLatLngToLocal(ww->coord1).X(),map->FromLatLngToLocal(ww->coord1).Y(),map->FromLatLngToLocal(ww->coord2).X(),map->FromLatLngToLocal(ww->coord2).Y());
        }

    }
    void GPSItem::SetTrailType(const UAVTrailType::Types &value)
    {
        trailtype=value;
        if(trailtype==UAVTrailType::ByTimeElapsed)
            timer.restart();
    }
    void GPSItem::SetShowTrail(const bool &value)
    {
        showtrail=value;
        trail->setVisible(value);

    }
    void GPSItem::SetShowTrailLine(const bool &value)
    {
        showtrailline=value;
        trailLine->setVisible(value);
    }
    void GPSItem::DeleteTrail()const
    {
        foreach(QGraphicsItem* i,trail->childItems())
            delete i;
        foreach(QGraphicsItem* i,trailLine->childItems())
            delete i;
    }
    double GPSItem::Distance3D(const internals::PointLatLng &coord, const int &altitude)
    {
       return sqrt(pow(internals::PureProjection::DistanceBetweenLatLng(this->coord,coord)*1000,2)+
       pow(static_cast<float>(this->altitude-altitude),2));

    }
    void GPSItem::SetUavPic(QString UAVPic)
    {
        pic.load(":/uavs/images/"+UAVPic);
    }
}
