/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief QGC Video Surface
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEO_SURFACE_H
#define VIDEO_SURFACE_H

#include <QtCore/QObject>

#include <gst/gst.h>

class VideoSurfacePrivate;

class VideoSurface : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(VideoSurface)
public:
    explicit VideoSurface(QObject *parent = 0);
    virtual ~VideoSurface();

    /*! Returns the video sink element that provides this surface's image.
     * The element will be constructed the first time that this function
     * is called. The surface will always keep a reference to this element.
     */
    GstElement* videoSink() const;

protected:
    void onUpdate();
    static void onUpdateThunk(GstElement* sink, gpointer data);

private:
    friend class VideoItem;
    VideoSurfacePrivate * const _data;
};

Q_DECLARE_METATYPE(VideoSurface*)

#endif // VIDEO_SURFACE_H
