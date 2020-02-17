#include "VideoSurface.h"
#include "VideoReceiver.h"

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#endif

VideoSurface::VideoSurface(QQuickItem *parent)
  : QQuickItem(parent)
  , _videoItem(nullptr)
  , _videoReceiver(nullptr)
  , _shouldStartVideo(false)
{
    // This flag is needed so the item will call updatePaintNode.
    setFlag(ItemHasContents);
}

VideoSurface::~VideoSurface() {
#if defined(QGC_GST_STREAMING)
    if (auto *pipeline = _videoReceiver->pipeline()) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }

    auto *sink =_videoReceiver->videoSink();
    if (!sink) {
        return;
    }

    GstElement *realSink = gst_bin_get_by_name(GST_BIN(sink), "qmlsink");
    if (realSink) {
        g_object_set(sink, "widget", nullptr, nullptr);
    }
#endif
}

void VideoSurface::setVideoItem(QObject *videoItem) {
    if (!videoItem) {
        return;
    }

    if (_videoItem != videoItem) {
        qDebug() << "Setting the video item";
        _videoItem = videoItem;
        Q_EMIT videoItemChanged(_videoItem);
    }
    startVideo();
}

void VideoSurface::startVideo() {
#if defined(QGC_GST_STREAMING)

    if (!_videoItem || !_videoReceiver) {
        qDebug() << "Cant start the video yet";
        return;
    }

    auto *sink =_videoReceiver->videoSink();
    auto *pipeline = _videoReceiver->pipeline();
    if (pipeline && sink) {
        GObject *videoSinkHasWidget = nullptr;
        GstElement *realSink = gst_bin_get_by_name(GST_BIN(sink), "qmlglsink");

        /* the qtqmlsink needs a property named 'Widget' to be set,
         * then it will send data to that widget directly without our intervention
         * this widget is the GstGlVideoItem we need to create in Qml
         */
        g_object_get(realSink, "widget", videoSinkHasWidget, nullptr);
        if (!videoSinkHasWidget) {
            g_object_set(realSink, "widget", _videoItem, nullptr);
        }

        _shouldStartVideo = true;
        update();
    }
#endif
}

void VideoSurface::pauseVideo() {
    _shouldPauseVideo = true;
    update();
}

QObject *VideoSurface::videoItem() const {
    return _videoItem;
}

void VideoSurface::setVideoReceiver(VideoReceiver *videoReceiver)
{
    if (!videoReceiver) {
        return;
    }

    if (_videoReceiver != videoReceiver) {
        qDebug() << "Setting the video receiver";
        _videoReceiver = videoReceiver;
        Q_EMIT videoReceiverChanged(_videoReceiver);
    }
    startVideo();
}

VideoReceiver *VideoSurface::videoReceiver() const
{
    return _videoReceiver;
}

QSGNode *VideoSurface::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
#if defined(QGC_GST_STREAMING)
    Q_UNUSED(data)
    if (!_shouldStartVideo && !_shouldPauseVideo) {
        return node;
    }
    auto *pipeline = _videoReceiver->pipeline();
    // Do not try to play an already playing pipeline, pause it first.
    if (_shouldStartVideo) {
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    } else if (_shouldPauseVideo) {
        gst_element_set_state(pipeline, GST_STATE_PAUSED);
    }
    _shouldStartVideo = false;
    _shouldPauseVideo = false;
    return node;
#endif
}

