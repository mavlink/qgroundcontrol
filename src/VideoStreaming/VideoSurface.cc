#include "VideoSurface.h"
#include "VideoReceiver.h"

#include <gst/gst.h>

VideoSurface::VideoSurface(QQuickItem *parent)
  : QQuickItem(parent)
  , _videoItem(nullptr)
  , _videoReceiver(nullptr)
  , _shouldStartVideo(false)
{
    qDebug() << "Starting a video surface";
    // This flag is needed so the item will call updatePaintNode.
    setFlag(ItemHasContents);
}

VideoSurface::~VideoSurface() {
    qDebug() << "Killing the video Surface";
    auto *sink =_videoReceiver->videoSink();
    auto *pipeline = _videoReceiver->pipeline();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_object_set(sink, "widget", nullptr, nullptr);
}

void VideoSurface::setVideoItem(QObject *videoItem) {
    if (_videoItem != videoItem) {
        qDebug() << "Setting the video item";
        _videoItem = videoItem;
        Q_EMIT videoItemChanged(_videoItem);
    }
    startVideo();
}

void VideoSurface::startVideo() {
    if (!_videoItem || !_videoReceiver) {
        qDebug() << "Cant start the video yet";
        return;
    }

    auto *sink =_videoReceiver->videoSink();
    auto *pipeline = _videoReceiver->pipeline();
    if (pipeline && sink) {
        GObject *videoSinkHasWidget = nullptr;
        /* the qtqmlsink needs a property named 'Widget' to be set,
         * then it will send data to that widget directly without our intervention
         * this widget is the GstGlVideoItem we need to create in Qml
         */
        g_object_get(sink, "widget", videoSinkHasWidget, nullptr);
        if (!videoSinkHasWidget) {
            g_object_set(sink, "widget", _videoItem, nullptr);
        }

        _shouldStartVideo = true;
        update();
    }
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
    if (_videoReceiver != videoReceiver) {
        qDebug() << "Setting the video receiver";
        _videoReceiver = videoReceiver;
        Q_EMIT videoReceiverChanged(_videoReceiver);
    }
    startVideo();
}

void VideoSurface::takeSnapshot() {
    _videoReceiver->takeSnapshot();
}

VideoReceiver *VideoSurface::videoReceiver() const
{
    return _videoReceiver;
}

QSGNode *VideoSurface::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    Q_UNUSED(data)
    if (!_shouldStartVideo && !_shouldPauseVideo) {
        return node;
    }
    auto *pipeline = _videoReceiver->pipeline();
    // Do not try to play an already playing pipeline, pause it first.
    if (_shouldStartVideo) {
        qDebug() << "Setting the state to PLAYING";
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    } else if (_shouldPauseVideo) {
        qDebug() << "Setting the state to PAUSED";
        gst_element_set_state(pipeline, GST_STATE_PAUSED);
    }
    _shouldStartVideo = false;
    _shouldPauseVideo = false;
    return node;
}

