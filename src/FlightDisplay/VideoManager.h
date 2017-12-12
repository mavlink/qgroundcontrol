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
    Q_PROPERTY(bool             uvcEnabled          READ    uvcEnabled                                  CONSTANT)
    Q_PROPERTY(bool             fullScreen          READ    fullScreen      WRITE   setfullScreen       NOTIFY fullScreenChanged)
    Q_PROPERTY(VideoReceiver*   videoReceiver       READ    videoReceiver                               CONSTANT)

    bool        hasVideo            ();
    bool        isGStreamer         ();
    bool        fullScreen          () { return _fullScreen; }
    QString     videoSourceID       () { return _videoSourceID; }

    VideoReceiver*  videoReceiver   () { return _videoReceiver; }

#if defined(QGC_DISABLE_UVC)
    bool        uvcEnabled          () { return false; }
#else
    bool        uvcEnabled          ();
#endif

    void        setfullScreen       (bool f) { _fullScreen = f; emit fullScreenChanged(); }

    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

    Q_INVOKABLE void startVideo() {_videoReceiver->stop();};
    Q_INVOKABLE void stopVideo() {_videoReceiver->stop();};

signals:
    void hasVideoChanged            ();
    void isGStreamerChanged         ();
    void videoSourceIDChanged       ();
    void fullScreenChanged          ();

private slots:
    void _videoSourceChanged        ();
    void _udpPortChanged            ();
    void _rtspUrlChanged            ();
    void _tcpUrlChanged             ();

private:
    void _updateSettings            ();
    void _restartVideo              ();

    VideoReceiver*  _videoReceiver;
    VideoSettings*  _videoSettings;
    QString         _videoSourceID;
    bool            _fullScreen;
};

#endif
