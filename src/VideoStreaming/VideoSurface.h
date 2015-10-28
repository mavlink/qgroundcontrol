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

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#endif

#if defined(QGC_GST_STREAMING)
class VideoSurfacePrivate;
#endif

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
#if defined(QGC_GST_STREAMING)
    GstElement* videoSink() const;
    time_t      lastFrame() { return _lastFrame; }
    void        setLastFrame(time_t t) { _lastFrame = t; }
#endif

protected:
#if defined(QGC_GST_STREAMING)
    void onUpdate();
    static void onUpdateThunk(GstElement* sink, gpointer data);
#endif

private:
    friend class VideoItem;
#if defined(QGC_GST_STREAMING)
    VideoSurfacePrivate * const _data;
    time_t _lastFrame;
#endif
};

Q_DECLARE_METATYPE(VideoSurface*)

#endif // VIDEO_SURFACE_H
