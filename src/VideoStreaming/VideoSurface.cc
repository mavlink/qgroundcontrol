#include "VideoSurface.h"
#include "VideoReceiver.h"

#include <gst/gst.h>

VideoSurface::VideoSurface(QQuickItem *parent)
  : QQuickItem(parent)
  , _videoItem(nullptr)
  , _shouldStartVideo(false)
{
    setFlag(ItemHasContents);
}

VideoSurface::~VideoSurface() {
    auto *sink =_videoReceiver->videoSink();
    auto *pipeline = _videoReceiver->pipeline();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    g_object_set(sink, "widget", nullptr, nullptr);
}

void VideoSurface::setVideoItem(QObject *videoItem) {
    if (_videoItem != videoItem) {
        _videoItem = videoItem;
        Q_EMIT videoItemChanged(_videoItem);
    }
    startVideo();
}

void VideoSurface::startVideo() {
    auto *sink =_videoReceiver->videoSink();
    auto *pipeline = _videoReceiver->pipeline();

    if (pipeline && sink && _videoItem) {
        GObject *videoSinkHasWidget = nullptr;
        g_object_get(sink, "widget", videoSinkHasWidget, nullptr);
        if (!videoSinkHasWidget) {
            g_object_set(sink, "widget", _videoItem, nullptr);
        }

        _shouldStartVideo = true;
        update();
    }
}

QObject *VideoSurface::videoItem() const {
    return _videoItem;
}

void VideoSurface::setVideoReceiver(VideoReceiver *videoReceiver)
{
    if (_videoReceiver != videoReceiver) {
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
    Q_UNUSED(data)
    if (_shouldStartVideo) {
        auto *pipeline = _videoReceiver->pipeline();
        GstState state;
        gst_element_get_state(pipeline, &state, nullptr, 5);
        if ( state == GST_STATE_PLAYING ) {
             gst_element_set_state(pipeline, GST_STATE_PAUSED);
        }

        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        //GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-playing");
        _shouldStartVideo = false;
    }
    return node;
}

