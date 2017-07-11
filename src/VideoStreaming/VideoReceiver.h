/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Video Receiver
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include "QGCLoggingCategory.h"
#include <QObject>
#include <QTimer>
#include <QTcpSocket>

#include "VideoSurface.h"

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(VideoReceiverLog)

class VideoReceiver : public QObject
{
    Q_OBJECT
public:
#if defined(QGC_GST_STREAMING)
    Q_PROPERTY(bool             recording           READ    recording           NOTIFY recordingChanged)
#endif
    Q_PROPERTY(VideoSurface*    videoSurface        READ    videoSurface        CONSTANT)
    Q_PROPERTY(bool             videoRunning        READ    videoRunning        NOTIFY videoRunningChanged)
    Q_PROPERTY(QString          imageFile           READ    imageFile           NOTIFY imageFileChanged)
    Q_PROPERTY(bool             showFullScreen      READ    showFullScreen      WRITE setShowFullScreen     NOTIFY showFullScreenChanged)

    explicit VideoReceiver(QObject* parent = 0);
    ~VideoReceiver();

#if defined(QGC_GST_STREAMING)
    bool            running         () { return _running;   }
    bool            recording       () { return _recording; }
    bool            streaming       () { return _streaming; }
    bool            starting        () { return _starting;  }
    bool            stopping        () { return _stopping;  }
#endif

    VideoSurface*   videoSurface    () { return _videoSurface; }
    bool            videoRunning    () { return _videoRunning; }
    QString         imageFile       () { return _imageFile; }
    bool            showFullScreen  () { return _showFullScreen; }
    void            grabImage       (QString imageFile);

    void        setShowFullScreen   (bool show) { _showFullScreen = show; emit showFullScreenChanged(); }

signals:
    void videoRunningChanged        ();
    void imageFileChanged           ();
    void showFullScreenChanged      ();
#if defined(QGC_GST_STREAMING)
    void recordingChanged           ();
    void msgErrorReceived           ();
    void msgEOSReceived             ();
    void msgStateChangedReceived    ();
#endif

public slots:
    void start                      ();
    void stop                       ();
    void setUri                     (const QString& uri);
    void stopRecording              ();
    void startRecording             ();

private slots:
    void _updateTimer               ();
#if defined(QGC_GST_STREAMING)
    void _timeout                   ();
    void _connected                 ();
    void _socketError               (QAbstractSocket::SocketError socketError);
    void _handleError               ();
    void _handleEOS                 ();
    void _handleStateChanged        ();
#endif

private:
#if defined(QGC_GST_STREAMING)

    typedef struct
    {
        GstPad*         teepad;
        GstElement*     queue;
        GstElement*     mux;
        GstElement*     filesink;
        GstElement*     parse;
        gboolean        removing;
    } Sink;

    bool                _running;
    bool                _recording;
    bool                _streaming;
    bool                _starting;
    bool                _stopping;
    Sink*               _sink;
    GstElement*         _tee;

    static gboolean             _onBusMessage           (GstBus* bus, GstMessage* message, gpointer user_data);
    static GstPadProbeReturn    _unlinkCallBack         (GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    void                        _detachRecordingBranch  (GstPadProbeInfo* info);
    void                        _shutdownRecordingBranch();
    void                        _shutdownPipeline       ();
    void                        _cleanupOldVideos       ();
    void                        _setVideoSink           (GstElement* sink);

    GstElement*     _pipeline;
    GstElement*     _pipelineStopRec;
    GstElement*     _videoSink;

    //-- Wait for Video Server to show up before starting
    QTimer          _frameTimer;
    QTimer          _timer;
    QTcpSocket*     _socket;
    bool            _serverPresent;

#endif

    QString         _uri;
    QString         _imageFile;
    VideoSurface*   _videoSurface;
    bool            _videoRunning;
    bool            _showFullScreen;

};

#endif // VIDEORECEIVER_H
