/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 * @file
 *   @brief QGC Video Receiver
 *   @author Gus Grubba <gus@auterion.com>
 */

#pragma once

#include "QGCLoggingCategory.h"
#include <QObject>
#include <QTimer>
#include <QTcpSocket>

#if defined(QGC_GST_STREAMING)
#include <gst/gst.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(VideoReceiverLog)

class VideoSettings;

class VideoReceiver : public QObject
{
    Q_OBJECT
public:

#if defined(QGC_GST_STREAMING)
    Q_PROPERTY(bool             recording           READ    recording           NOTIFY recordingChanged)
#endif
    Q_PROPERTY(bool             videoRunning        READ    videoRunning        NOTIFY  videoRunningChanged)
    Q_PROPERTY(QString          imageFile           READ    imageFile           NOTIFY  imageFileChanged)
    Q_PROPERTY(QString          videoFile           READ    videoFile           NOTIFY  videoFileChanged)
    Q_PROPERTY(QString          imagePath           READ    imagePath           NOTIFY  imagePathChanged)
    Q_PROPERTY(QString          videoPath           READ    videoPath           NOTIFY  videoPathChanged)

    Q_PROPERTY(bool             showFullScreen      READ    showFullScreen      WRITE   setShowFullScreen     NOTIFY showFullScreenChanged)
    Q_PROPERTY(bool             streamEnabled       READ    streamEnabled       WRITE   setStreamEnabled      NOTIFY streamEnabledChanged)
    Q_PROPERTY(bool             streamConfigured    READ    streamConfigured    WRITE   setStreamConfigured   NOTIFY streamConfiguredChanged)
    Q_PROPERTY(bool             storageLimit        READ    storageLimit        WRITE   setStorageLimit       NOTIFY storageLimitChanged)
    Q_PROPERTY(bool             isTaisync           READ    isTaisync           WRITE   setIsTaysinc          NOTIFY isTaisyncChanged)

    Q_PROPERTY(int              maxVideoSize        READ    maxVideoSize        WRITE   setMaxVideoSize       NOTIFY maxVideoSizeChanged)
    Q_PROPERTY(int              recordingFormatId   READ    recordingFormatId   WRITE   setRecordingFormatId  NOTIFY recordingFormatIdChanged)
    Q_PROPERTY(int              rtspTimeout         READ    rtspTimeout         WRITE   setRtspTimeout        NOTIFY rtspTimeoutChanged)

    explicit VideoReceiver(QObject* parent = nullptr);
    ~VideoReceiver();

    bool streamEnabled() const;
    Q_SLOT void setStreamEnabled(bool enabled);
    Q_SIGNAL void streamEnabledChanged();

    bool streamConfigured() const;
    Q_SLOT void setStreamConfigured(bool enabled);
    Q_SIGNAL void streamConfiguredChanged();

    bool storageLimit() const;
    Q_SLOT void setStorageLimit(bool enabled);
    Q_SIGNAL void storageLimitChanged();

    bool isTaisync() const;
    Q_SLOT void setIsTaysinc(bool value);
    Q_SIGNAL void isTaisyncChanged();

    QString videoPath() const;
    Q_SLOT void setVideoPath(const QString& path);
    Q_SIGNAL void videoPathChanged();

    QString imagePath() const;
    Q_SLOT void setImagePath(const QString& path);
    Q_SIGNAL void imagePathChanged();

    int maxVideoSize() const;
    Q_SLOT void setMaxVideoSize(int value);
    Q_SIGNAL void maxVideoSizeChanged();

    int recordingFormatId() const;
    Q_SLOT void setRecordingFormatId(int value);
    Q_SIGNAL void recordingFormatIdChanged();

    int rtspTimeout() const;
    Q_SLOT void setRtspTimeout(int value);
    Q_SIGNAL void rtspTimeoutChanged();

    Q_SIGNAL void restartTimeout();
    Q_SIGNAL void sendMessage(const QString& message);

    void setUnittestMode(bool runUnitTests);
#if defined(QGC_GST_STREAMING)
    virtual bool            recording       () { return _recording; }
#endif

    virtual bool            videoRunning    () { return _videoRunning; }
    virtual QString         imageFile       () { return _imageFile; }
    virtual QString         videoFile       () { return _videoFile; }
    virtual bool            showFullScreen  () { return _showFullScreen; }

    virtual void            grabImage       (QString imageFile);

    virtual void        setShowFullScreen   (bool show) { _showFullScreen = show; emit showFullScreenChanged(); }

#if defined(QGC_GST_STREAMING)
    void                  setVideoSink      (GstElement* videoSink);
#endif

signals:
    void videoRunningChanged                ();
    void imageFileChanged                   ();
    void videoFileChanged                   ();
    void showFullScreenChanged              ();
#if defined(QGC_GST_STREAMING)
    void recordingChanged                   ();
    void msgErrorReceived                   ();
    void msgEOSReceived                     ();
    void msgStateChangedReceived            ();
    void gotFirstRecordingKeyFrame          ();
#endif

public slots:
    virtual void start                      ();
    virtual void stop                       ();
    virtual void setUri                     (const QString& uri);
    virtual void stopRecording              ();
    virtual void startRecording             (const QString& videoFile = QString());

protected slots:
    virtual void _updateTimer               ();
#if defined(QGC_GST_STREAMING)
    GstElement*  _makeSource                (const QString& uri);
    GstElement*  _makeFileSink              (const QString& videoFile, unsigned format);
    virtual void _handleError               ();
    virtual void _handleEOS                 ();
    virtual void _handleStateChanged        ();
#endif

protected:
#if defined(QGC_GST_STREAMING)

    typedef struct
    {
        GstPad*         teepad;
        GstElement*     queue;
        GstElement*     filesink;
        gboolean        removing;
    } Sink;

    bool                _running;
    bool                _recording;
    bool                _streaming;
    bool                _starting;
    bool                _stopping;
    bool                _stop;
    Sink*               _sink;
    GstElement*         _tee;

    void _noteVideoSinkFrame                            ();

    static gboolean             _onBusMessage           (GstBus* bus, GstMessage* message, gpointer user_data);
    static GstPadProbeReturn    _unlinkCallBack         (GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn    _videoSinkProbe         (GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn    _keyframeWatch          (GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

    virtual void                _unlinkRecordingBranch  (GstPadProbeInfo* info);
    virtual void                _shutdownRecordingBranch();
    virtual void                _shutdownPipeline       ();
    virtual void                _cleanupOldVideos       ();

    GstElement*     _pipeline;
    GstElement*     _videoSink;
    guint64         _lastFrameId;
    qint64          _lastFrameTime;

    //-- Wait for Video Server to show up before starting
    QTimer          _frameTimer;
    QTimer          _restart_timer;
    int             _restart_time_ms;

    //-- RTSP UDP reconnect timeout
    uint64_t        _udpReconnect_us;
#endif

    QString         _uri;
    QString         _imageFile;
    QString         _videoFile;
    QString         _videoPath;
    QString         _imagePath;

    bool            _videoRunning;
    bool            _showFullScreen;
    bool            _streamEnabled;
    bool            _streamConfigured;
    bool            _storageLimit;
    bool            _unittTestMode;
    bool            _isTaisync;
    int            _maxVideoSize; // in mbs.
    int            _recordingFormatId; // 0 - 2, defined in VideoReceiver.cc / kVideoExtensions. TODO: use a better representation.
    int            _rtspTimeout;

};

