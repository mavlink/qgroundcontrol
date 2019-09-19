#include "VideoController.h"

#include <QQuickWindow>

#if 0
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
        qDebug() << "Initializing the State to Playing" << gst_element_set_state(_pipeline, GST_STATE_PLAYING);
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

#endif


VideoController::VideoController(QQuickItem *parent)
: QQuickItem(parent)
, m_videoItem(nullptr)
, m_pipeline(nullptr)
, m_videoSink(nullptr)
, m_shouldStartVideo(false)
{
    qDebug() << "Starting the video controller";
    setFlag(ItemHasContents);
    preparePipeline();
}

VideoController::~VideoController() {
    qDebug() << "Destroying the Video Controller";
    gst_element_set_state (m_pipeline, GST_STATE_NULL);
    gst_object_unref (m_pipeline);
}

void VideoController::preparePipeline() {
  m_pipeline = gst_pipeline_new (nullptr);

  GstElement *src = gst_element_factory_make ("videotestsrc", nullptr);
  GstElement *glupload = gst_element_factory_make ("glupload", nullptr);

  m_videoSink = gst_element_factory_make ("qmlglsink", nullptr);

  gst_bin_add_many (GST_BIN (m_pipeline), src, glupload, m_videoSink, nullptr);
  gst_element_link_many (src, glupload, m_videoSink, nullptr);
}

void VideoController::setVideoItem(QObject *videoItem) {
    if (m_videoItem != videoItem) {
        m_videoItem = videoItem;
        g_object_set(m_videoSink, "widget", videoItem, nullptr);
        Q_EMIT videoItemChanged(m_videoItem);
    }
}

void VideoController::startVideo() {
    m_shouldStartVideo = true;
    update();
}

QSGNode *VideoController::updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
{
    if (m_shouldStartVideo) {
        gst_element_set_state (m_pipeline, GST_STATE_PLAYING);
        m_shouldStartVideo = false;
    }
    return node;
}

QObject *VideoController::videoItem() const {
    return m_videoItem;
}
