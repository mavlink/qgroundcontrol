#ifndef VIDEOCONTROLLER_H
#define VIDEOCONTROLLER_H

#include <QQuickItem>
#include <gst/gst.h>

class VideoController : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *videoItem WRITE setVideoItem READ videoItem NOTIFY videoItemChanged)
    Q_PROPERTY(GstElement *pipeline WRITE setPipeline READ pipeline NOTIFY pipelineChanged)
    Q_PROPERTY(GstElement *videoSink WRITE setVideoSink READ videoSink NOTIFY videoSinkChanged)

public:
    VideoController(QQuickItem *parent = nullptr);
    virtual ~VideoController();

    /* This is the Qml GstGLVideoItem  */
    QObject *videoItem() const;
    Q_SLOT void setVideoItem(QObject *videoItem);
    Q_SIGNAL void videoItemChanged(QObject *videoItem);

    void setPipeline(GstElement *pipeline);
    GstElement *pipeline() const;
    Q_SIGNAL void pipelineChanged(GstElement *pipeline);

    void setVideoSink(GstElement *videoSink);
    GstElement *videoSink() const;
    Q_SIGNAL void videoSinkChanged(GstElement *videoSink);

    /* This method sets the pipeline state to playing as soon as Qt allows it */
    Q_INVOKABLE void startVideo();

    /* This *actually* enables the video set by startVideo() */
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *);

private:
    QObject *_videoItem;
    GstElement *_pipeline;
    GstElement *_videoSink;
    bool _shouldStartVideo;
};

#endif // VIDEOCONTROLLER_H
