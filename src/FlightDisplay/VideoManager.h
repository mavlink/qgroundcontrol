/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef VideoManager_H
#define VideoManager_H

#include <QObject>
#include <QTimer>
#include <QUrl>

#include "QGCLoggingCategory.h"
#include "VideoSurface.h"
#include "VideoReceiver.h"
#include "QGCToolbox.h"

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

class VideoManager : public QGCTool
{
    Q_OBJECT

public:
    VideoManager    (QGCApplication* app);
    ~VideoManager   ();

    Q_PROPERTY(bool             hasVideo        READ    hasVideo                                    NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isGStreamer     READ    isGStreamer                                 NOTIFY isGStreamerChanged)
    Q_PROPERTY(QString          videoSourceID   READ    videoSourceID                               NOTIFY videoSourceIDChanged)
    Q_PROPERTY(QString          videoSource     READ    videoSource     WRITE setVideoSource        NOTIFY videoSourceChanged)
    Q_PROPERTY(QStringList      videoSourceList READ    videoSourceList                             NOTIFY videoSourceListChanged)
    Q_PROPERTY(bool             videoRunning    READ    videoRunning                                NOTIFY videoRunningChanged)
    Q_PROPERTY(quint16          udpPort         READ    udpPort         WRITE setUdpPort            NOTIFY udpPortChanged)
    Q_PROPERTY(QString          rtspURL         READ    rtspURL         WRITE setRtspURL            NOTIFY rtspURLChanged)
    Q_PROPERTY(QString          videoSavePath   READ    videoSavePath                               NOTIFY videoSavePathChanged)
    Q_PROPERTY(bool             uvcEnabled      READ    uvcEnabled                                  CONSTANT)
    Q_PROPERTY(VideoSurface*    videoSurface    MEMBER  _videoSurface                               CONSTANT)
    Q_PROPERTY(VideoReceiver*   videoReceiver   MEMBER  _videoReceiver                              CONSTANT)

    Q_INVOKABLE void setVideoSavePathByUrl (QUrl url);

    bool        hasVideo            ();
    bool        isGStreamer         ();
    bool        videoRunning        () { return _videoRunning; }
    QString     videoSourceID       () { return _videoSourceID; }
    QString     videoSource         () { return _videoSource; }
    QStringList videoSourceList     ();
    quint16     udpPort             () { return _udpPort; }
    QString     rtspURL             () { return _rtspURL; }
    QString     videoSavePath       () { return _videoSavePath; }

#if defined(QGC_DISABLE_UVC)
    bool        uvcEnabled          () { return false; }
#else
    bool        uvcEnabled          ();
#endif

    void        setVideoSource          (QString vSource);
    void        setUdpPort              (quint16 port);
    void        setRtspURL              (QString url);
    void        setVideoSavePath        (QString path);

    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

signals:
    void hasVideoChanged        ();
    void videoRunningChanged    ();
    void videoSourceChanged     ();
    void videoSourceListChanged ();
    void isGStreamerChanged     ();
    void videoSourceIDChanged   ();
    void udpPortChanged         ();
    void rtspURLChanged         ();
    void videoSavePathChanged   ();

private:
    void _updateTimer           ();
    void _updateVideo           ();

private:
    VideoSurface*       _videoSurface;
    VideoReceiver*      _videoReceiver;
    bool                _videoRunning;
#if defined(QGC_GST_STREAMING)
    QTimer              _frameTimer;
#endif
    QString             _videoSource;
    QString             _videoSourceID;
    QStringList         _videoSourceList;
    quint16             _udpPort;
    QString             _rtspURL;
    QString             _videoSavePath;
    bool                _init;
};

#endif
