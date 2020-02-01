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

#pragma once

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
    GstElement* videoSink();
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
    time_t  _lastFrame;
    bool    _refed;
#endif
};

Q_DECLARE_METATYPE(VideoSurface*)

