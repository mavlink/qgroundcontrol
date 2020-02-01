/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Surface
 *   @author Gus Grubba <gus@auterion.com>
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
    , _refed(false)
#endif
{
}

VideoSurface::~VideoSurface()
{
#if defined(QGC_GST_STREAMING)
    if (!_refed && _data->videoSink != nullptr) {
        gst_element_set_state(_data->videoSink, GST_STATE_NULL);
    }
    delete _data;
#endif
}

#if defined(QGC_GST_STREAMING)
GstElement* VideoSurface::videoSink()
{
    if (_data->videoSink == nullptr) {
        if ((_data->videoSink = gst_element_factory_make("qtquick2videosink", nullptr)) == nullptr) {
            qCritical("Failed to create qtquick2videosink. Make sure it is installed correctly");
            return nullptr;
        }
        g_object_set(G_OBJECT(_data->videoSink), "sync", gboolean(false), nullptr);
        g_signal_connect(_data->videoSink, "update", G_CALLBACK(onUpdateThunk), (void* )this);
        _refed = true;
    }
    return _data->videoSink;
}

void VideoSurface::onUpdate()
{
    _lastFrame = time(nullptr);
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

