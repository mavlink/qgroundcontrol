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

class VideoSettings;

class VideoManager : public QGCTool
{
    Q_OBJECT

public:
    VideoManager    (QGCApplication* app);
    ~VideoManager   ();

    Q_PROPERTY(bool             hasVideo            READ    hasVideo                                    NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isGStreamer         READ    isGStreamer                                 NOTIFY isGStreamerChanged)
    Q_PROPERTY(QString          videoSourceID       READ    videoSourceID                               NOTIFY videoSourceIDChanged)
    Q_PROPERTY(bool             videoRunning        READ    videoRunning                                NOTIFY videoRunningChanged)
    Q_PROPERTY(bool             uvcEnabled          READ    uvcEnabled                                  CONSTANT)
    Q_PROPERTY(VideoSurface*    videoSurface        MEMBER  _videoSurface                               CONSTANT)
    Q_PROPERTY(VideoReceiver*   videoReceiver       MEMBER  _videoReceiver                              CONSTANT)
    Q_PROPERTY(bool             recordingEnabled    READ    recordingEnabled                            CONSTANT)

    bool        hasVideo            ();
    bool        isGStreamer         ();
    bool        videoRunning        () { return _videoRunning; }
    QString     videoSourceID       () { return _videoSourceID; }

#if defined(QGC_DISABLE_UVC)
    bool        uvcEnabled          () { return false; }
#else
    bool        uvcEnabled          ();
#endif

#if defined(QGC_GST_STREAMING) && defined(QGC_ENABLE_VIDEORECORDING)
    bool        recordingEnabled    () { return true; }
#else
    bool        recordingEnabled    () { return false; }
#endif

    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

signals:
    void hasVideoChanged        ();
    void videoRunningChanged    ();
    void isGStreamerChanged     ();
    void videoSourceIDChanged   ();

private:
    void _updateTimer           ();
    void _updateVideo           ();

private:
    VideoSurface*   _videoSurface;
    VideoReceiver*  _videoReceiver;
    bool            _videoRunning;
#if defined(QGC_GST_STREAMING)
    QTimer          _frameTimer;
#endif
    QString         _videoSourceID;
    bool            _init;
    VideoSettings*  _videoSettings;
};

#endif
