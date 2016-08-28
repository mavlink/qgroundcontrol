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

    Q_PROPERTY(bool             hasVideo        READ    hasVideo                                NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isGStreamer     READ    isGStreamer                             NOTIFY isGStreamerChanged)
    Q_PROPERTY(QString          videoSourceID   READ    videoSource                             NOTIFY videoSourceIDChanged)
    Q_PROPERTY(QString          videoSource     READ    videoSource     WRITE setVideoSource    NOTIFY videoSourceChanged)
    Q_PROPERTY(QStringList      videoSourceList READ    videoSourceList                         NOTIFY videoSourceListChanged)
    Q_PROPERTY(bool             videoRunning    READ    videoRunning                            NOTIFY videoRunningChanged)
    Q_PROPERTY(VideoSurface*    videoSurface    MEMBER  _videoSurface                           CONSTANT)
    Q_PROPERTY(VideoReceiver*   videoReceiver   MEMBER  _videoReceiver                          CONSTANT)

    bool        hasVideo            ();
    bool        isGStreamer         ();
    bool        videoRunning        () { return _videoRunning; }
    QString     videoSourceID       () { return _videoSourceID; }
    QString     videoSource         () { return _videoSource; }
    QStringList videoSourceList     ();
    void        setVideoSource      (QString vSource);

    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

signals:
    void hasVideoChanged        ();
    void videoRunningChanged    ();
    void videoSourceChanged     ();
    void videoSourceListChanged ();
    void isGStreamerChanged     ();
    void videoSourceIDChanged   ();

private:
    void _updateTimer(void);

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
};

#endif
