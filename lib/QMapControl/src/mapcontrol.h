/*
*
* This file is part of QMapControl,
* an open-source cross-platform map widget
*
* Copyright (C) 2007 - 2008 Kai Winter
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with QMapControl. If not, see <http://www.gnu.org/licenses/>.
*
* Contact e-mail: kaiwinter@gmx.de
* Program URL   : http://qmapcontrol.sourceforge.net/
*
*/

#ifndef MAPCONTROL_H
#define MAPCONTROL_H

#include <QtGui>

#include "layermanager.h"
#include "layer.h"
#include "mapadapter.h"
#include "geometry.h"
#include "imagemanager.h"

//! QMapControl namespace
namespace qmapcontrol
{
    class LayerManager;
    class MapAdapter;
    class Layer;

    //! The control element of the widget and also the widget itself
    /*!
     * This is the main widget.
     * To this control layers can be added.
     * A MapControl have to be instantiated with a QSize which sets the size the widget takes in a layout.
     * The given size is also the size, which is asured to be filled with map images.
     *
     * @author Kai Winter <kaiwinter@gmx.de>
     */
    class MapControl : public QWidget
    {
        Q_OBJECT

    public:
        //! Declares what actions the mouse move has on the map
        enum MouseMode
        {
            Panning, /*!< The map is moved */
            Dragging, /*!< A rectangular can be drawn */
            None, /*!< Mouse move events have no efect to the map */
        };

        //! The constructor of MapControl
        /*!
         * The MapControl is the widget which displays the maps.
         * The size describes the area, which gets filled with map data
         * When you give no MouseMode, the mouse is moving the map.
         * You can change the MouseMode on runtime, to e.g. Dragging, which lets the user drag a rectangular box.
         * After the dragging a signal with the size of the box is emitted.
         * The mousemode ´None´ can be used, to completely define the control of the map yourself.
         * @param size the size which the widget should fill with map data
         * @param mousemode the way mouseevents are handled
         */
        MapControl ( QSize size, MouseMode mousemode = Panning );

        ~MapControl();

        //! adds a layer
        /*!
         * If multiple layers are added, they are painted in the added order.
         * @param layer the layer which should be added
         */
        void addLayer ( Layer* layer );

        //! returns the layer with the given name
        /*!
         * @param  layername name of the wanted layer
         * @return the layer with the given name
         */
        Layer* layer ( const QString& layername ) const;

        //! returns the names of all layers
        /*!
         * @return returns a QList with the names of all layers
         */
        QList<QString> layers() const;

        //! returns the number of existing layers
        /*!
         * @return returns the number of existing layers
         */
        int numberOfLayers() const;

        //! returns the coordinate of the center of the map
        /*!
         * @return returns the coordinate of the middle of the screen
         */
        QPointF	currentCoordinate() const;

        //! returns the current zoom level
        /*!
         * @return returns the current zoom level
         */
        int currentZoom() const;

        //! sets the middle of the map to the given coordinate
        /*!
         * @param  coordinate the coordinate which the view´s middle should be set to
         */
        void setView ( const QPointF& coordinate ) const;

        //! sets the view, so all coordinates are visible
        /*!
         * @param  coordinates the Coorinates which should be visible
         */
        void setView ( const QList<QPointF> coordinates ) const;

        //! sets the view and zooms in, so all coordinates are visible
        /*!
         * The code of setting the view to multiple coordinates is "brute force" and pretty slow.
         * Have to be reworked.
         * @param  coordinates the Coorinates which should be visible
         */
        void setViewAndZoomIn ( const QList<QPointF> coordinates ) const;

        //! sets the view to the given Point
        /*!
         *
         * @param point the geometric point the view should be set to
         */
        void setView ( const Point* point ) const;

        //! Keeps the center of the map on the Geometry, even when it moves
        /*!
         * To stop the following the method stopFollowing() have to be called
         * @param  geometry the Geometry which should stay centered.
         */
        void followGeometry ( const Geometry* geometry ) const;

        //TODO:
        // void followGeometry(const QList<Geometry*>) const;

        //! Stops the following of a Geometry
        /*!
         * if the view is set to follow a Geometry this method stops the trace.
         * See followGeometry().
         * @param geometry the Geometry which should not followed anymore
         */
        void stopFollowing ( Geometry* geometry );

        //! Smoothly moves the center of the view to the given Coordinate
        /*!
         * @param  coordinate the Coordinate which the center of the view should moved to
         */
        void moveTo	( QPointF coordinate );

        //! sets the Mouse Mode of the MapControl
        /*!
         * There are three MouseModes declard by an enum.
         * The MouesMode Dragging draws an rectangular in the map while the MouseButton is pressed.
         * When the Button is released a boxDragged() signal is emitted.
         *
         * The second MouseMode (the default) is Panning, which allows to drag the map around.
         * @param mousemode the MouseMode
         */
        void setMouseMode ( MouseMode mousemode );

        //! returns the current MouseMode
        /*!
         * For a explanation for the MouseModes see setMouseMode()
         * @return the current MouseMode
         */
        MapControl::MouseMode mouseMode();

        //int rotation;

        //! Enable persistent caching of map tiles
        /*!
         * Call this method to allow the QMapControl widget to save map tiles
         * persistent (also over application restarts).
         * Tiles are stored in the subdirectory "QMapControl.cache" within the
         * user's home directory. This can be changed by giving a path.
         * @param path the path to the cache directory
         */
        void enablePersistentCache ( const QDir& path=QDir::homePath() + "/QMapControl.cache" );


        //! Sets the proxy for HTTP connections
        /*!
         * This method sets the proxy for HTTP connections.
         * This is not provided by the current Qtopia version!
         * @param host the proxy´s hostname or ip
         * @param port the proxy´s port
         */
        void setProxy ( QString host, int port );

        //! Displays the scale within the widget
        /*!
         *
         * @param show true if the scale should be displayed
         */
        void showScale ( bool show );


        //! Displays the Lat and Lon within the widget
        /*!
         *
         * @param show true if Lat and Lon should be displayed
         */
        void showCoord ( bool show );

    private:
        LayerManager* layermanager;
        QPoint screen_middle; // middle of the widget (half size)

        QPoint pre_click_px; // used for scrolling (MouseMode Panning)
        QPoint current_mouse_pos; // used for scrolling and dragging (MouseMode Panning/Dragging)

        QSize size; // size of the widget

        QPointF currentWorldCoordinate; // updated by mouseMove

        QList<double> distanceList;

        bool mousepressed;
        MouseMode mymousemode;
        bool scaleVisible;
        bool cursorPosVisible;

        bool m_loadingFlag;

        QMutex moveMutex; // used for method moveTo()
        QPointF target; // used for method moveTo()
        int steps; // used for method moveTo()

        QPointF clickToWorldCoordinate ( QPoint click );
        MapControl& operator= ( const MapControl& rhs );
        MapControl ( const MapControl& old );

    protected:
        void paintEvent ( QPaintEvent* evnt );
        void mousePressEvent ( QMouseEvent* evnt );
        void mouseReleaseEvent ( QMouseEvent* evnt );
        void mouseMoveEvent ( QMouseEvent* evnt );

    signals:
        void mouseEvent(const QMouseEvent* evnt);

        //! Emitted AFTER a MouseEvent occured
        /*!
         * This signals allows to receive click events within the MapWidget together with the world coordinate.
         * It is emitted on MousePressEvents and MouseReleaseEvents.
         * The kind of the event can be obtained by checking the events type.
         * @param  evnt the QMouseEvent that occured
         * @param  coordinate the corresponding world coordinate
         */
        void mouseEventCoordinate ( const QMouseEvent* evnt, const QPointF coordinate );

        //! Emitted on mouse move generating
        /*!
         * This signals allows to receive the mouse position in the world coordinate.
         * It is emitted on mouseMoveEvents.
         * setMouseTracking must be set programatically to have this method work.
         * @param  coordinate the corresponding world coordinate
         */
        void mouseMoveCoordinateEvent(const QPointF coordinate);

        //! Emitted, after a Rectangular is dragged.
        /*!
         * It is possible to select a rectangular area in the map, if the MouseMode is set to Dragging.
         * The coordinates are in world coordinates
         * @param  QRectF the dragged Rect
         */
        void boxDragged ( const QRectF );

        //! This signal is emitted, when a Geometry is clicked
        /*!
         * @param geometry The clicked Geometry object
         * @param coord_px  The coordinate in pixel coordinates
         */
        void geometryClicked ( Geometry* geometry, QPoint coord_px );

        //! This signal is emitted, after the view have changed
        /*!
         * @param coordinate The current coordinate
         * @param zoom The current zoom
         */
        void viewChanged ( const QPointF &coordinate, int zoom );

    public slots:
        //! zooms in one step
        void zoomIn();

        //! zooms out one step
        void zoomOut();

        //! sets the given zoomlevel
        /*!
         * @param zoomlevel the zoomlevel
         */
        void setZoom ( int zoomlevel );

        //! scrolls the view to the left
        void scrollLeft ( int pixel=10 );

        //! scrolls the view to the right
        void scrollRight ( int pixel=10 );

        //! scrolls the view up
        void scrollUp ( int pixel=10 );

        //! scrolls the view down
        void scrollDown ( int pixel=10 );

        //! scrolls the view by the given point
        void scroll ( const QPoint scroll );

        //! updates the map for the given rect
        /*!
         * @param rect the area which should be repainted
         */
        void updateRequest ( QRect rect );

        //! updates the hole map by creating a new offscreen image
        /*!
         *
         */
        void updateRequestNew();

        //! Resizes the map to the given size
        /*!
         * @param newSize The new size
         */
        void resize(const QSize newSize);

    private slots:
        void tick();
        void loadingFinished();
        void positionChanged ( Geometry* geom );
    };
}
#endif
