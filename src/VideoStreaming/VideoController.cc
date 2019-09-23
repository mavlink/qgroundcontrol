#include "VideoController.h"

#include <QQuickWindow>

VideoController::VideoController(QQuickItem *parent)
: QQuickItem(parent)
, _videoItem(nullptr)
, _pipeline(nullptr)
, _videoSink(nullptr)
, _shouldStartVideo(false)
{
    setFlag(ItemHasContents);
}

VideoController::~VideoController() {
    gst_element_set_state (_pipeline, GST_STATE_NULL);
}

void VideoController::setVideoReceiver(QObject *videoReceiver)
{
    _videoReceiver = qobject_cast<VideoReceiver*>(videoReceiver);
    _pipeline = _videoReceiver->pipeline();
    _videoSink = _videoReceiver->videoSink();
    startVideo();
}

void VideoController::setVideoItem(QObject *videoItem) {
    if (_videoItem != videoItem) {
        _videoItem = videoItem;
        Q_EMIT videoItemChanged(_videoItem);
    }
    startVideo();
}

void VideoController::startVideo() {
    if (_pipeline && _videoSink && _videoItem) {

        GObject *videoSinkHasWidget = nullptr;
        g_object_get(_videoSink, "widget", videoSinkHasWidget, nullptr);
        if (!videoSinkHasWidget) {
            g_object_set(_videoSink, "widget", _videoItem, nullptr);
        }

        _shouldStartVideo = true;
        update();
    }
}

QSGNode *VideoController::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    Q_UNUSED(data)
    if (_shouldStartVideo) {
        gst_element_set_state(_pipeline, GST_STATE_PLAYING);
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-playing");
        _shouldStartVideo = false;
    }
    return node;
}

QObject *VideoController::videoItem() const {
    return _videoItem;
}

QObject *VideoController::videoReceiver() const {
    return _videoReceiver;
}
