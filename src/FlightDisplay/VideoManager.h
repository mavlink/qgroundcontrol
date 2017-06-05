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
    VideoManager    (QGCApplication* app, QGCToolbox* toolbox);
    ~VideoManager   ();

    Q_PROPERTY(bool             hasVideo            READ    hasVideo                                    NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isGStreamer         READ    isGStreamer                                 NOTIFY isGStreamerChanged)
    Q_PROPERTY(QString          videoSourceID       READ    videoSourceID                               NOTIFY videoSourceIDChanged)
    Q_PROPERTY(bool             videoRunning        READ    videoRunning                                NOTIFY videoRunningChanged)
    Q_PROPERTY(bool             uvcEnabled          READ    uvcEnabled                                  CONSTANT)
    Q_PROPERTY(VideoSurface*    videoSurface        MEMBER  _videoSurface                               CONSTANT)
    Q_PROPERTY(VideoReceiver*   videoReceiver       MEMBER  _videoReceiver                              CONSTANT)
    Q_PROPERTY(QString          imageFile           READ    imageFile                                   NOTIFY imageFileChanged)
    Q_PROPERTY(bool             showFullScreen      READ    showFullScreen  WRITE setShowFullScreen     NOTIFY showFullScreenChanged)

    bool        hasVideo            ();
    bool        isGStreamer         ();
    bool        videoRunning        () { return _videoRunning; }
    QString     videoSourceID       () { return _videoSourceID; }
    QString     imageFile           () { return _imageFile; }
    bool        showFullScreen      () { return _showFullScreen; }

#if defined(QGC_DISABLE_UVC)
    bool        uvcEnabled          () { return false; }
#else
    bool        uvcEnabled          ();
#endif

    void        grabImage           (QString imageFile);
    void        setShowFullScreen   (bool show) { _showFullScreen = show; emit showFullScreenChanged(); }

    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

signals:
    void hasVideoChanged        ();
    void videoRunningChanged    ();
    void isGStreamerChanged     ();
    void videoSourceIDChanged   ();
    void imageFileChanged       ();
    void showFullScreenChanged  ();

private slots:
    void _videoSourceChanged(void);
    void _udpPortChanged(void);
    void _rtspUrlChanged(void);

private:
    void _updateTimer           ();
    void _updateSettings        ();
    void _updateVideo           ();
    void _restartVideo          ();


    VideoSurface*   _videoSurface;
    VideoReceiver*  _videoReceiver;
    bool            _videoRunning;
#if defined(QGC_GST_STREAMING)
    QTimer          _frameTimer;
#endif
    QString         _videoSourceID;
    bool            _init;
    VideoSettings*  _videoSettings;
    QString         _imageFile;
    bool            _showFullScreen;
};

#endif
