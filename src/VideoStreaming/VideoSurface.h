#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H

#include <QQuickItem>
#include <QObject>

class VideoReceiver;

class VideoSurface : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *videoItem WRITE setVideoItem READ videoItem NOTIFY videoItemChanged)
    Q_PROPERTY(VideoReceiver *videoReceiver WRITE setVideoReceiver READ videoReceiver NOTIFY videoReceiverChanged)
public:
    VideoSurface(QQuickItem *parent = nullptr);
    virtual ~VideoSurface();

   /* This is the Qml GstGLVideoItem  */
    QObject *videoItem() const;
    Q_SLOT void setVideoItem(QObject *videoItem);
    Q_SIGNAL void videoItemChanged(QObject *videoItem);

    /* This is the Video Receiver, managed by the Model */
    VideoReceiver *videoReceiver() const;
    Q_SLOT void setVideoReceiver(VideoReceiver *videoReceiver);
    Q_SIGNAL void videoReceiverChanged(VideoReceiver *videoReceiver);

    /* This method sets the pipeline state to playing as soon as Qt allows it */
    Q_INVOKABLE void startVideo();

    /* This *actually* enables the video set by startVideo() */
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *);

private:
    QObject *_videoItem;
    VideoReceiver *_videoReceiver;
    bool _shouldStartVideo;

};

#endif
