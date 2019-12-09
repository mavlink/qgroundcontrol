#ifndef VIDEOSURFACE_H
#define VIDEOSURFACE_H

#include <QQuickItem>
#include <QObject>

class VideoReceiver;

/* This is a fake surface that controls the video.
 *
 * Because of the way that OpenGL works, and because how GStreamer
 * is created, we need to play / pause the pipeline in the rendering
 * thread, that means that this element is the actual responsible
 * of starting the video. Other classes can request the video to start,
 * but never directly.
 *
 * The 'VideoItem' element is a GstGlVideoItem, that's the real paint surface.
 * This class plug things together and controlls playing / pausing / screenshooting, etc.
 */
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

    /* This is the Video Receiver, that controls the creation of the pipeline and a few other helper functions */
    VideoReceiver *videoReceiver() const;
    Q_SLOT void setVideoReceiver(VideoReceiver *videoReceiver);
    Q_SIGNAL void videoReceiverChanged(VideoReceiver *videoReceiver);

    /* This method sets the pipeline state to playing as soon as Qt allows it */
    Q_INVOKABLE void startVideo();
    Q_INVOKABLE void pauseVideo();

    /* update paint node usually is a method that should trigger a painting on the OpenGL surface
    in our case, it's where we set the gstreamer pipeline state to playing / stop / pause.
    */
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *);

private:
    QObject *_videoItem;
    VideoReceiver *_videoReceiver;
    bool _shouldStartVideo;
    bool _shouldPauseVideo;
};

#endif
