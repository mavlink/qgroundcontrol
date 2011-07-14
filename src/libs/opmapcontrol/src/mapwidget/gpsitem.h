/**
******************************************************************************
*
* @file       gpsitem.h
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
#ifndef GPSITEM_H
#define GPSITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include <QLabel>
#include "../internals/pointlatlng.h"
#include "mapgraphicitem.h"
#include "waypointitem.h"
#include <QObject>
#include "uavmapfollowtype.h"
#include "uavtrailtype.h"
#include <QtSvg/QSvgRenderer>
#include "opmapwidget.h"
#include "trailitem.h"
#include "traillineitem.h"
namespace mapcontrol
{
    class WayPointItem;
    class OPMapWidget;
    /**
* @brief A QGraphicsItem representing the UAV
*
* @class UAVItem gpsitem.h "mapwidget/gpsitem.h"
*/
    class GPSItem:public QObject,public QGraphicsItem
    {
        Q_OBJECT
        Q_INTERFACES(QGraphicsItem)
    public:
                enum { Type = UserType + 6 };
        GPSItem(MapGraphicItem* map,OPMapWidget* parent, QString uavPic=QString::fromUtf8(":/uavs/images/mapquad.png"));
        ~GPSItem();
        /**
        * @brief Sets the UAV position
        *
        * @param position LatLng point
        * @param altitude altitude in meters
        */
        void SetUAVPos(internals::PointLatLng const& position,int const& altitude);
        /**
        * @brief Sets the UAV heading
        *
        * @param value heading angle (north=0deg)
        */
        void SetUAVHeading(qreal const& value);
        /**
        * @brief Returns the UAV position
        *
        * @return internals::PointLatLng
        */
        internals::PointLatLng UAVPos()const{return coord;}
        /**
        * @brief Sets the Map follow type
        *
        * @param value can be "none"(map doesnt follow UAV), "CenterMap"(map moves to keep UAV centered) or "CenterAndRotateMap"(map moves and rotates to keep UAV centered and straight)
        */
        void SetMapFollowType(UAVMapFollowType::Types const& value){mapfollowtype=value;}
        /**
        * @brief Sets the trail type
        *
        * @param value can be "NoTrail"(no trail is plotted), "ByTimeElapsed"(a trail point is plotted each TrailTime()) or ByDistance(a trail point is plotted if the distance between the UAV and the last trail point is bigger than TrailDistance())
        */
        void SetTrailType(UAVTrailType::Types const& value);
        /**
        * @brief Returns the map follow method used
        *
        * @return UAVMapFollowType::Types
        */
        UAVMapFollowType::Types GetMapFollowType()const{return mapfollowtype;}
        /**
        * @brief Returns the UAV trail type. It can be plotted by time elapsed or distance
        *
        * @return UAVTrailType::Types
        */
        UAVTrailType::Types GetTrailType()const{return trailtype;}

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                    QWidget *widget);
        void RefreshPos();
        QRectF boundingRect() const;
        /**
        * @brief Sets the trail time to be used if TrailType is ByTimeElapsed
        *
        * @param seconds the UAV trail time elapsed value. If the trail type is time elapsed
        *        a trail point will be plotted each "value returned" seconds.
        */
        void SetTrailTime(int const& seconds){trailtime=seconds;}
        /**
        * @brief Returns the UAV trail time elapsed value. If the trail type is time elapsed
        *        a trail point will be plotted each "value returned" seconds.
        *
        * @return int
        */
        int TrailTime()const{return trailtime;}
        /**
        * @brief Sets the trail distance to be used if TrailType is ByDistance
        *
        * @param distance the UAV trail plot distance.
        *        If the trail type is ByDistance a trail dot is plotted if
        *        the distance between the current UAV position and the last trail point
        *        is bigger than the returned value
        */
        void SetTrailDistance(int const& distance){traildistance=distance;}
        /**
        * @brief Returns the UAV trail plot distance.
        *        If the trail type is distance diference a trail dot is plotted if
        *        the distance between the current UAV position and the last trail point
        *        is bigger than the returned value
        *
        * @return int
        */
        int TrailDistance()const{return traildistance;}
        /**
        * @brief Returns true if UAV trail is shown
        *
        * @return bool
        */
        bool ShowTrail()const{return showtrail;}
        /**
        * @brief Returns true if UAV trail line is shown
        *
        * @return bool
        */
        bool ShowTrailLine()const{return showtrailline;}
        /**
        * @brief Used to define if the UAV displays a trail
        *
        * @param value
        */
        void SetShowTrail(bool const& value);
        /**
        * @brief Used to define if the UAV displays a trail line
        *
        * @param value
        */
        void SetShowTrailLine(bool const& value);
        /**
        * @brief Deletes all the trail points
        */
        void DeleteTrail()const;
        /**
        * @brief Returns true if the UAV automaticaly sets WP reached value (changing its color)
        *
        * @return bool
        */
        bool AutoSetReached()const{return autosetreached;}
        /**
        * @brief Defines if the UAV can set the WP's "reached" value automaticaly.
        *
        * @param value
        */
        void SetAutoSetReached(bool const& value){autosetreached=value;}
        /**
        * @brief Returns the 3D distance in meters necessary for the UAV to set WP's to "reached"
        *
        * @return double
        */
        double AutoSetDistance()const{return autosetdistance;}
        /**
        * @brief Sets the  the 3D distance in meters necessary for the UAV to set WP's to "reached"
        *
        * @param value
        */
        void SetAutoSetDistance(double const& value){autosetdistance=value;}

        int type() const;

        void SetUavPic(QString UAVPic);
    private:
        MapGraphicItem* map;

        int altitude;
        UAVMapFollowType::Types mapfollowtype;
        UAVTrailType::Types trailtype;
        internals::PointLatLng coord;
        internals::PointLatLng lastcoord;
        QPixmap pic;
        core::Point localposition;
        OPMapWidget* mapwidget;
        QGraphicsItemGroup* trail;
        QGraphicsItemGroup * trailLine;
        internals::PointLatLng lasttrailline;
        QTime timer;
        bool showtrail;
        bool showtrailline;
        int trailtime;
        int traildistance;
        bool autosetreached;
        double Distance3D(internals::PointLatLng const& coord, int const& altitude);
        double autosetdistance;
      //  QRectF rect;

    public slots:

    signals:
        void UAVReachedWayPoint(int const& waypointnumber,WayPointItem* waypoint);
        void UAVLeftSafetyBouble(internals::PointLatLng const& position);
    };
}
#endif // GPSITEM_H
