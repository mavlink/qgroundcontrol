#ifndef VIDEOCONTROLLER_H
#define VIDEOCONTROLLER_H

#include <QQuickItem>
#include <gst/gst.h>
#include "VideoReceiver.h"

class VideoController : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *videoItem WRITE setVideoItem READ videoItem NOTIFY videoItemChanged)
    Q_PROPERTY(QObject *videoReceiver WRITE setVideoReceiver READ videoReceiver NOTIFY videoReceiverChanged)

public:
    VideoController(QQuickItem *parent = nullptr);
    virtual ~VideoController();

    /* This is the Qml GstGLVideoItem  */
    QObject *videoItem() const;
    Q_SLOT void setVideoItem(QObject *videoItem);
    Q_SIGNAL void videoItemChanged(QObject *videoItem);

    void setVideoReceiver(QObject *videoReceiver);
    QObject *videoReceiver() const;
    Q_SIGNAL void videoReceiverChanged(QObject *videoReceiver);

    /* This method sets the pipeline state to playing as soon as Qt allows it */
    Q_INVOKABLE void startVideo();

    /* This *actually* enables the video set by startVideo() */
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *);

private:
    VideoReceiver *_videoReceiver;
    QObject *_videoItem;
    GstElement *_pipeline;
    GstElement *_videoSink;
    bool _shouldStartVideo;
};

#endif
