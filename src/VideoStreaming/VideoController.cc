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

void VideoController::setPipeline(GstElement *pipeline)
{
    if (_pipeline != pipeline) {
        _pipeline = pipeline;
    }

    if (_pipeline && _videoItem) {
        startVideo();
    }
}

VideoController::~VideoController() {
    qDebug() << "Destroying the Video Controller";
    gst_element_set_state (_pipeline, GST_STATE_NULL);
}

void VideoController::setVideoItem(QObject *videoItem) {
    if (_videoItem != videoItem) {
        _videoItem = videoItem;
        g_object_set(_videoSink, "widget", videoItem, nullptr);
        Q_EMIT videoItemChanged(_videoItem);
    }
    if (_pipeline && _videoItem) {
        startVideo();
    }
}

void VideoController::startVideo() {
    _shouldStartVideo = true;
    update();
}

QSGNode *VideoController::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    if (_shouldStartVideo) {
        gst_element_set_state(_pipeline, GST_STATE_PLAYING);
        _shouldStartVideo = false;
    }
    return node;
}

QObject *VideoController::videoItem() const {
    return _videoItem;
}
