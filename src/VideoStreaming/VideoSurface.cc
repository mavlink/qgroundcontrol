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

#if defined(QGC_GST_STREAMING)
#include "VideoSurface_p.h"
#endif
#include "VideoSurface.h"

#include <QtCore/QDebug>
#include <QtQuick/QQuickItem>

VideoSurface::VideoSurface(QObject *parent)
    : QObject(parent)
#if defined(QGC_GST_STREAMING)
    , _data(new VideoSurfacePrivate)
    , _lastFrame(0)
#endif
{
}

VideoSurface::~VideoSurface()
{
#if defined(QGC_GST_STREAMING)
    if (_data->videoSink != NULL) {
        gst_element_set_state(_data->videoSink, GST_STATE_NULL);
    }
    delete _data;
#endif
}

#if defined(QGC_GST_STREAMING)
GstElement* VideoSurface::videoSink() const
{
    if (_data->videoSink == NULL) {
        if ((_data->videoSink = gst_element_factory_make("qtquick2videosink", NULL)) == NULL) {
            qCritical("Failed to create qtquick2videosink. Make sure it is installed correctly");
            return NULL;
        }
        g_signal_connect(_data->videoSink, "update", G_CALLBACK(onUpdateThunk), (void* )this);
    }
    return _data->videoSink;
}

void VideoSurface::onUpdate()
{
    _lastFrame = time(0);
    Q_FOREACH(QQuickItem *item, _data->items) {
        item->update();
    }
}

void VideoSurface::onUpdateThunk(GstElement* sink, gpointer data)
{
    Q_UNUSED(sink);
    VideoSurface* pThis = (VideoSurface* )data;
    pThis->onUpdate();
}
#endif

