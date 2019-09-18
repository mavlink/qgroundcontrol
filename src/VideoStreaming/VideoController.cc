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

void VideoController::setPipeline(GstElement *pipeline)
{
    if (_pipeline != pipeline) {
        _pipeline = pipeline;
        Q_EMIT pipelineChanged(pipeline);
    }
    startVideo();
}

void VideoController::setVideoSink(GstElement *videoSink)
{
    if (_videoSink != videoSink) {
        _videoSink = videoSink;
        Q_EMIT videoSinkChanged(videoSink);
    }
    startVideo();
}

void VideoController::setVideoItem(QObject *videoItem) {
    if (_videoItem != videoItem) {
        _videoItem = videoItem;
        g_object_set(_videoSink, "widget", videoItem, nullptr);
        Q_EMIT videoItemChanged(_videoItem);
    }
    startVideo();
}

void VideoController::startVideo() {
    if (_pipeline && _videoSink && _videoItem) {
        _shouldStartVideo = true;
        update();
    }
}

QSGNode *VideoController::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    if (_shouldStartVideo) {
        gst_element_set_state(_pipeline, GST_STATE_PLAYING);
        _shouldStartVideo = false;
    }
    return node;
}

QObject *VideoController::videoItem() const {
    return _videoItem;
}

GstElement *VideoController::pipeline() const {
    return _pipeline;
}

GstElement *VideoController::videoSink() const {
    return _videoSink;
}
