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
#include <QTime>
#include <QUrl>

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "VideoReceiver.h"
#include "QGCToolbox.h"

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

class VideoSettings;
class Vehicle;
class Joystick;

class VideoManager : public QGCTool
{
    Q_OBJECT

public:
    VideoManager    (QGCApplication* app, QGCToolbox* toolbox);
    ~VideoManager   ();

    Q_PROPERTY(bool             hasVideo            READ    hasVideo                                    NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isGStreamer         READ    isGStreamer                                 NOTIFY isGStreamerChanged)
    Q_PROPERTY(bool             isAutoStream        READ    isAutoStream                                NOTIFY isAutoStreamChanged)
    Q_PROPERTY(bool             isTaisync           READ    isTaisync       WRITE   setIsTaisync        NOTIFY isTaisyncChanged)
    Q_PROPERTY(QString          videoSourceID       READ    videoSourceID                               NOTIFY videoSourceIDChanged)
    Q_PROPERTY(bool             uvcEnabled          READ    uvcEnabled                                  CONSTANT)
    Q_PROPERTY(bool             fullScreen          READ    fullScreen      WRITE   setfullScreen       NOTIFY fullScreenChanged)
    Q_PROPERTY(VideoReceiver*   videoReceiver       READ    videoReceiver                               CONSTANT)
    Q_PROPERTY(double           aspectRatio         READ    aspectRatio                                 NOTIFY streamInfoChanged)
    Q_PROPERTY(bool             hasZoom             READ    hasZoom                                     NOTIFY streamInfoChanged)
    Q_PROPERTY(int              streamCount         READ    streamCount                                 NOTIFY streamCountChanged)
    Q_PROPERTY(int              currentStream       READ    currentStream   WRITE setCurrentStream      NOTIFY currentStreamChanged)

    bool        hasVideo            ();
    bool        isGStreamer         ();
    bool        isAutoStream        ();
    bool        isTaisync           () { return _isTaisync; }
    bool        fullScreen          () { return _fullScreen; }
    QString     videoSourceID       () { return _videoSourceID; }
    QString     autoURL             () { return QString(_streamInfo.uri); }
    double      aspectRatio         ();
    bool        hasZoom             () { return _streamInfo.flags & VIDEO_STREAM_HAS_BASIC_ZOOM; }
    int         currentStream       () { return _currentStream; }
    int         streamCount         () { return _streamInfo.count; }

    VideoReceiver*  videoReceiver   () { return _videoReceiver; }

#if defined(QGC_DISABLE_UVC)
    bool        uvcEnabled          () { return false; }
#else
    bool        uvcEnabled          ();
#endif

    void        setfullScreen       (bool f) { _fullScreen = f; emit fullScreenChanged(); }
    void        setIsTaisync        (bool t) { _isTaisync = t;  emit isTaisyncChanged(); }
    void        setCurrentStream    (int stream);

    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

    Q_INVOKABLE void startVideo     () { _videoReceiver->start(); }
    Q_INVOKABLE void stopVideo      () { _videoReceiver->stop();  }
    Q_INVOKABLE void stepZoom       (int direction);
    Q_INVOKABLE void stepStream     (int direction);

signals:
    void hasVideoChanged            ();
    void isGStreamerChanged         ();
    void videoSourceIDChanged       ();
    void fullScreenChanged          ();
    void isAutoStreamChanged        ();
    void streamInfoChanged          ();
    void isTaisyncChanged           ();
    void currentStreamChanged       ();
    void streamCountChanged         ();

private slots:
    void _videoSourceChanged        ();
    void _udpPortChanged            ();
    void _rtspUrlChanged            ();
    void _tcpUrlChanged             ();
    void _updateUVC                 ();
    void _streamInfoChanged         ();
    void _setActiveVehicle          (Vehicle* vehicle);
    void _activeJoystickChanged     (Joystick* joystick);
    void _vehicleMessageReceived    (const mavlink_message_t& message);

private:
    void _updateSettings            ();
    void _restartVideo              ();

private:
    bool            _isTaisync          = false;
    VideoReceiver*  _videoReceiver      = nullptr;
    VideoSettings*  _videoSettings      = nullptr;
    QString         _videoSourceID;
    bool            _fullScreen         = false;
    Vehicle*        _activeVehicle      = nullptr;
    Joystick*       _activeJoystick     = nullptr;
    mavlink_video_stream_information_t _streamInfo;
    bool            _hasAutoStream      = false;
    uint8_t         _videoStreamCompID  = 0;
    int             _currentStream      = 1;
    QTime           _lastZoomChange;
    QTime           _lastStreamChange;
};

#endif
