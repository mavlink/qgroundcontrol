#include "VideoController.h"

#include <QQuickWindow>

VideoController::VideoController(QQuickItem *parent)
: QQuickItem(parent)
, _videoItem(nullptr)
, _pipeline(nullptr)
, _videoSink(nullptr)
, _shouldStartVideo(false)
{
    qDebug() << "Starting the video controller";
    setFlag(ItemHasContents);
}

VideoController::~VideoController() {
    qDebug() << "Destroying the Video Controller";
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
        g_object_set(_videoSink, "widget", videoItem, nullptr);
        Q_EMIT videoItemChanged(_videoItem);
        qDebug() << "video item set" << videoItem;
    }
    startVideo();
}

void VideoController::startVideo() {
    if (_pipeline && _videoSink && _videoItem) {
        _shouldStartVideo = true;
        qDebug() << "request to start the video";
        update();
    }
}

QSGNode *VideoController::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    Q_UNUSED(data)
    if (_shouldStartVideo) {
        qDebug() << "Video should start";
        gst_element_set_state(_pipeline, GST_STATE_PLAYING);
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-paused");
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
