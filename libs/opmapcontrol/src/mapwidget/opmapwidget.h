/**
******************************************************************************
*
* @file       opmapwidget.h
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      The Map Widget, this is the part exposed to the user
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
#ifndef OPMAPWIDGET_H
#define OPMAPWIDGET_H

#include "../mapwidget/mapgraphicitem.h"
#include "../core/geodecoderstatus.h"
#include "../core/maptype.h"
#include "../core/languagetype.h"
#include "../core/diagnostics.h"
#include "configuration.h"
#include <QObject>
#include <QtOpenGL/QGLWidget>
#include "waypointitem.h"
#include "QtSvg/QGraphicsSvgItem"
#include "uavitem.h"
#include "gpsitem.h"
#include "homeitem.h"
#include "waypointlineitem.h"
#include "mapripper.h"
#include "uavtrailtype.h"
namespace mapcontrol
{
    class UAVItem;
    class GPSItem;
    class HomeItem;
    /**
    * @brief Collection of static functions to help dealing with various enums used
    *       Contains functions for enumToString conversio, StringToEnum, QStringList of enum values...
    *
    * @class Helper opmapwidget.h "opmapwidget.h"
    */
    class Helper
    {
    public:
        /**
         * @brief Converts from String to Type
         *
         * @param value String to convert
         * @return
         */
        static MapType::Types MapTypeFromString(QString const& value){return MapType::TypeByStr(value);}
        /**
         * @brief Converts from Type to String
         */
        static QString StrFromMapType(MapType::Types const& value){return MapType::StrByType(value);}
        /**
         * @brief Returns QStringList with string representing all the enum values
         */
        static QStringList MapTypes(){return MapType::TypesList();}

        /**
        * @brief Converts from String to Type
        */
        static GeoCoderStatusCode::Types GeoCoderStatusCodeFromString(QString const& value){return GeoCoderStatusCode::TypeByStr(value);}
        /**
        * @brief Converts from Type to String
        */
        static QString StrFromGeoCoderStatusCode(GeoCoderStatusCode::Types const& value){return GeoCoderStatusCode::StrByType(value);}
        /**
        * @brief Returns QStringList with string representing all the enum values
        */
        static QStringList GeoCoderTypes(){return GeoCoderStatusCode::TypesList();}

        /**
        * @brief Converts from String to Type
        */
        static internals::MouseWheelZoomType::Types MouseWheelZoomTypeFromString(QString const& value){return internals::MouseWheelZoomType::TypeByStr(value);}
        /**
        * @brief Converts from Type to String
        */
        static QString StrFromMouseWheelZoomType(internals::MouseWheelZoomType::Types const& value){return internals::MouseWheelZoomType::StrByType(value);}
        /**
        * @brief Returns QStringList with string representing all the enum values
        */
        static QStringList MouseWheelZoomTypes(){return internals::MouseWheelZoomType::TypesList();}
        /**
        * @brief Converts from String to Type
        */
        static core::LanguageType::Types LanguageTypeFromString(QString const& value){return core::LanguageType::TypeByStr(value);}
        /**
        * @brief Converts from Type to String
        */
        static QString StrFromLanguageType(core::LanguageType::Types const& value){return core::LanguageType::StrByType(value);}
        /**
        * @brief Returns QStringList with string representing all the enum values
        */
        static QStringList LanguageTypes(){return core::LanguageType::TypesList();}
        /**
        * @brief Converts from String to Type
        */
        static core::AccessMode::Types AccessModeFromString(QString const& value){return core::AccessMode::TypeByStr(value);}
        /**
        * @brief Converts from Type to String
        */
        static QString StrFromAccessMode(core::AccessMode::Types const& value){return core::AccessMode::StrByType(value);}
        /**
        * @brief Returns QStringList with string representing all the enum values
        */
        static QStringList AccessModeTypes(){return core::AccessMode::TypesList();}

        /**
        * @brief Converts from String to Type
        */
        static UAVMapFollowType::Types UAVMapFollowFromString(QString const& value){return UAVMapFollowType::TypeByStr(value);}
        /**
        * @brief Converts from Type to String
        */
        static QString StrFromUAVMapFollow(UAVMapFollowType::Types const& value){return UAVMapFollowType::StrByType(value);}
        /**
        * @brief Returns QStringList with string representing all the enum values
        */
        static QStringList UAVMapFollowTypes(){return UAVMapFollowType::TypesList();}
        /**
         * @brief Converts from String to Type
         */
        static UAVTrailType::Types UAVTrailTypeFromString(QString const& value){return UAVTrailType::TypeByStr(value);}
        /**
         * @brief Converts from Type to String
         */
        static QString StrFromUAVTrailType(UAVTrailType::Types const& value){return UAVTrailType::StrByType(value);}
        /**
         * @brief Returns QStringList with string representing all the enum values
         */
        static QStringList UAVTrailTypes(){return UAVTrailType::TypesList();}
    };

    class OPMapWidget:public QGraphicsView
    {
        Q_OBJECT

        // Q_PROPERTY(int MaxZoom READ MaxZoom WRITE SetMaxZoom)
        Q_PROPERTY(int MinZoom READ MinZoom WRITE SetMinZoom)
                Q_PROPERTY(bool ShowTileGridLines READ ShowTileGridLines WRITE SetShowTileGridLines)
                Q_PROPERTY(double Zoom READ ZoomTotal WRITE SetZoom)
                Q_PROPERTY(qreal Rotate READ Rotate WRITE SetRotate)
                Q_ENUMS(internals::MouseWheelZoomType::Types)
                Q_ENUMS(internals::GeoCoderStatusCode::Types)

    public:
                QSize sizeHint() const;
        /**
        * @brief Constructor
        *
        * @param parent parent widget
        * @param config pointer to configuration classed to be used
        * @return
        */
        OPMapWidget(QWidget *parent=0,Configuration *config=new Configuration);
        ~OPMapWidget();

        /**
        * @brief Returns true if map is showing gridlines
        *
        * @return bool
        */
        bool ShowTileGridLines()const {return map->showTileGridLines;}

        /**
        * @brief Defines if map is to show gridlines
        *
        * @param value
        * @return
        */
        void SetShowTileGridLines(bool const& value){map->showTileGridLines=value;map->update();}

        /**
        * @brief Returns the maximum zoom for the map
        *
        */
        int MaxZoom()const{return map->MaxZoom();}

        //  void SetMaxZoom(int const& value){map->maxZoom = value;}

        /**
        * @brief
        *
        */
        int MinZoom()const{return map->minZoom;}
        /**
        * @brief
        *
        * @param value
        */
        void SetMinZoom(int const& value){map->minZoom = value;}

        internals::MouseWheelZoomType::Types GetMouseWheelZoomType(){return  map->core->GetMouseWheelZoomType();}
        void SetMouseWheelZoomType(internals::MouseWheelZoomType::Types const& value){map->core->SetMouseWheelZoomType(value);}
        //  void SetMouseWheelZoomTypeByStr(const QString &value){map->core->SetMouseWheelZoomType(internals::MouseWheelZoomType::TypeByStr(value));}
        //  QString GetMouseWheelZoomTypeStr(){return map->GetMouseWheelZoomTypeStr();}

        internals::RectLatLng SelectedArea()const{return  map->selectedArea;}
        void SetSelectedArea(internals::RectLatLng const& value){ map->selectedArea = value;this->update();}

        bool CanDragMap()const{return map->CanDragMap();}
        void SetCanDragMap(bool const& value){map->SetCanDragMap(value);}

        internals::PointLatLng CurrentPosition()const{return map->core->CurrentPosition();}
        void SetCurrentPosition(internals::PointLatLng const& value){map->core->SetCurrentPosition(value);}

        double ZoomReal(){return map->Zoom();}
        double ZoomDigi(){return map->ZoomDigi();}
        double ZoomTotal(){return map->ZoomTotal();}

        qreal Rotate(){return map->rotation;}
        void SetRotate(qreal const& value);

        void ReloadMap(){map->ReloadMap(); map->resize();}

        GeoCoderStatusCode::Types SetCurrentPositionByKeywords(QString const& keys){return map->SetCurrentPositionByKeywords(keys);}

        bool UseOpenGL(){return useOpenGL;}
        void SetUseOpenGL(bool const& value);

        MapType::Types GetMapType(){return map->core->GetMapType();}
        void SetMapType(MapType::Types const& value){map->lastimage=QImage(); map->core->SetMapType(value);}

        bool isStarted(){return map->core->isStarted();}

        Configuration* configuration;

        internals::PointLatLng currentMousePosition();

        void SetFollowMouse(bool const& value){followmouse=value;this->setMouseTracking(followmouse);}
        bool FollowMouse(){return followmouse;}

        internals::PointLatLng GetFromLocalToLatLng(QPointF p) {return map->FromLocalToLatLng(p.x(),p.y());}

        /** @brief Convert meters to pixels */
        float metersToPixels(double meters);

        /** @brief Return the bearing from one point to another .. in degrees */
        double bearing(internals::PointLatLng from, internals::PointLatLng to);

        /** @brief Return a destination lat/lon point given a source lat/lon point and the bearing and distance from the source point */
        internals::PointLatLng destPoint(internals::PointLatLng source, double bear, double dist);

        /**
        * @brief Creates a new WayPoint on the center of the map
        *
        * @return WayPointItem a pointer to the WayPoint created
        */
        WayPointItem* WPCreate();
        /**
        * @brief Creates a new WayPoint
        *
        * @param item the WayPoint to create
        */
        void WPCreate(WayPointItem* item);
        /**
        * @brief Creates a new WayPoint
        *
        * @param id the system (MAV) id this waypoint belongs to
        * @param item the WayPoint to create
        */
        void WPCreate(int id, WayPointItem* item);
        /**
        * @brief Creates a new WayPoint
        *
        * @param coord the coordinates in LatLng of the WayPoint
        * @param altitude the Altitude of the WayPoint
        * @return WayPointItem a pointer to the WayPoint created
        */
        WayPointItem* WPCreate(internals::PointLatLng const& coord,int const& altitude);
        /**
        * @brief Creates a new WayPoint
        *
        * @param coord the coordinates in LatLng of the WayPoint
        * @param altitude the Altitude of the WayPoint
        * @param description the description of the WayPoint
        * @return WayPointItem a pointer to the WayPoint created
        */
        WayPointItem* WPCreate(internals::PointLatLng const& coord,int const& altitude, QString const& description);
        /**
        * @brief Inserts a new WayPoint on the specified position
        *
        * @param position index of the WayPoint
        * @return WayPointItem a pointer to the WayPoint created
        */
        WayPointItem* WPInsert(int const& position);
        /**
        * @brief Inserts a new WayPoint on the specified position
        *
        * @param item the WayPoint to Insert
        * @param position index of the WayPoint
        */
        void WPInsert(WayPointItem* item,int const& position);
        /**
        * @brief Inserts a new WayPoint on the specified position
        *
        * @param coord the coordinates in LatLng of the WayPoint
        * @param altitude the Altitude of the WayPoint
        * @param position index of the WayPoint
        * @return WayPointItem a pointer to the WayPoint Inserted
        */
        WayPointItem* WPInsert(internals::PointLatLng const& coord,int const& altitude,int const& position);
        /**
        * @brief Inserts a new WayPoint on the specified position
        *
        * @param coord the coordinates in LatLng of the WayPoint
        * @param altitude the Altitude of the WayPoint
        * @param description the description of the WayPoint
        * @param position index of the WayPoint
        * @return WayPointItem a pointer to the WayPoint Inserted
        */
        WayPointItem* WPInsert(internals::PointLatLng const& coord,int const& altitude, QString const& description,int const& position);

        /**
        * @brief Deletes the WayPoint
        *
        * @param item the WayPoint to delete
        */
        void WPDelete(WayPointItem* item);
        /**
        * @brief deletes all WayPoints
        *
        */
        void WPDeleteAll();
        /**
        * @brief Returns the currently selected WayPoints
        *
        * @return @return QList<WayPointItem *>
        */
        QList<WayPointItem*> WPSelected();

        /**
        * @brief Renumbers the WayPoint and all others as needed
        *
        * @param item the WayPoint to renumber
        * @param newnumber the WayPoint's new number
        */
        void WPRenumber(WayPointItem* item,int const& newnumber);

        void SetShowCompass(bool const& value);

        // FIXME XXX Move to protected namespace
        UAVItem* UAV;
        QMap<int, QGraphicsItemGroup*> waypointLines;
        GPSItem* GPS;
        HomeItem* Home;
        // END OF FIXME XXX

        UAVItem* AddUAV(int id);
        void AddUAV(int id, UAVItem* uav);
        /** @brief Deletes UAV and its waypoints from map */
        void DeleteUAV(int id);
        UAVItem* GetUAV(int id);
        const QList<UAVItem*> GetUAVS();
        QGraphicsItemGroup* waypointLine(int id);
        void SetShowUAV(bool const& value);
        bool ShowUAV()const{return showuav;}
        void SetShowHome(bool const& value);
        bool ShowHome()const{return showhome;}
        void SetShowDiagnostics(bool const& value);
        void SetUavPic(QString UAVPic);
                QMap<int, UAVItem*> UAVS;
    private:
        internals::Core *core;
        QGraphicsScene mscene;
        bool useOpenGL;
        GeoCoderStatusCode x;
        MapType y;
        core::AccessMode xx;
        internals::PointLatLng currentmouseposition;
        bool followmouse;
        QGraphicsSvgItem *compass;
        bool showuav;
        bool showhome;
        QTimer * diagTimer;
        bool showDiag;
        QGraphicsTextItem * diagGraphItem;

    private slots:
        void diagRefresh();
        //   WayPointItem* item;//apagar
    protected:
        MapGraphicItem *map;
        void resizeEvent(QResizeEvent *event);
        void showEvent ( QShowEvent * event );
        void closeEvent(QCloseEvent *event);
        void mouseMoveEvent ( QMouseEvent * event );
        void ConnectWP(WayPointItem* item);
        //    private slots:
    signals:
        void zoomChanged(double zoomt,double zoom, double zoomd);
        /**
        * @brief Notify connected widgets about new map zoom
        */
        void zoomChanged(int newZoom);
        /**
        * @brief fires when one of the WayPoints numbers changes (not fired if due to a auto-renumbering)
        *
        * @param oldnumber WayPoint old number
        * @param newnumber WayPoint new number
        * @param waypoint a pointer to the WayPoint that was renumbered
        */
        void WPNumberChanged(int const& oldnumber,int const& newnumber,WayPointItem* waypoint);
        /**
        * @brief Fired when the description, altitude or coordinates of a WayPoint changed
        *
        * @param waypoint a pointer to the WayPoint
        */
        void WPValuesChanged(WayPointItem* waypoint);
        /**
        * @brief Fires when a new WayPoint is inserted
        *
        * @param number new WayPoint number
        * @param waypoint WayPoint inserted
        */
        void WPReached(WayPointItem* waypoint);
        /**
        * @brief Fires when a new WayPoint is inserted
        *
        * @param number new WayPoint number
        * @param waypoint WayPoint inserted
        */
        void WPInserted(int const& number,WayPointItem* waypoint);
        /**
        * @brief Fires When a WayPoint is deleted
        *
        * @param number number of the deleted WayPoint
        */
        void WPDeleted(int const& number);
        /**
        * @brief Fires When a WayPoint is Reached
        *
        * @param number number of the Reached WayPoint
        */
        void UAVReachedWayPoint(int const& waypointnumber,WayPointItem* waypoint);
        /**
        * @brief Fires When the UAV lives the safety bouble
        *
        * @param position the position of the UAV
        */
        void UAVLeftSafetyBouble(internals::PointLatLng const& position);

        /**
        * @brief Fires when map position changes
        *
        * @param point the point in LatLng of the new center of the map
        */
        void OnCurrentPositionChanged(internals::PointLatLng point);
        /**
        * @brief Fires when there are no more tiles to load
        *
        */
        void OnTileLoadComplete();
        /**
        * @brief Fires when tiles loading begins
        *
        */
        void OnTileLoadStart();
        /**
        * @brief Fires when the map is dragged
        *
        */
        void OnMapDrag();
        /**
        * @brief Fires when map zoom changes
        *
        */
        void OnMapZoomChanged();
        /**
        * @brief Fires when map type changes
        *
        * @param type The maps new type
        */
        void OnMapTypeChanged(MapType::Types type);
        /**
        * @brief Fires when an error ocurred while loading a tile
        *
        * @param zoom tile zoom
        * @param pos tile position
        */
        void OnEmptyTileError(int zoom, core::Point pos);
        /**
        * @brief Fires when the number of tiles in the load queue changes
        *
        * @param number the number of tiles still in the queue
        */
        void OnTilesStillToLoad(int number);
    public slots:
        /**
        * @brief Ripps the current selection to the DB
        */
        void RipMap();

        /**
        * @brief Sets the map zoom level
        */
        void SetZoom(double const& value){map->SetZoom(value);}
        /**
        * @brief Sets the map zoom level
        */
        void SetZoom(int const& value){map->SetZoom(value);}

        /**
        * @brief Notify external widgets about map zoom change
        */
        void emitMapZoomChanged()
        {
            emit zoomChanged(ZoomReal());
        }

    };
}
#endif // OPMAPWIDGET_H
