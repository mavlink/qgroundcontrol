/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "SubtitleWriter.h"

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

class VideoSettings;
class Vehicle;
class Joystick;

class VideoManager : public QGCTool
{
    Q_OBJECT

public:
    VideoManager    (QGCApplication* app, QGCToolbox* toolbox);
    virtual ~VideoManager   ();

    Q_PROPERTY(bool             hasVideo                READ    hasVideo                                    NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isGStreamer             READ    isGStreamer                                 NOTIFY isGStreamerChanged)
    Q_PROPERTY(bool             isTaisync               READ    isTaisync       WRITE   setIsTaisync        NOTIFY isTaisyncChanged)
    Q_PROPERTY(QString          videoSourceID           READ    videoSourceID                               NOTIFY videoSourceIDChanged)
    Q_PROPERTY(bool             uvcEnabled              READ    uvcEnabled                                  CONSTANT)
    Q_PROPERTY(bool             fullScreen              READ    fullScreen      WRITE   setfullScreen       NOTIFY fullScreenChanged)
    Q_PROPERTY(VideoReceiver*   videoReceiver           READ    videoReceiver                               CONSTANT)
    Q_PROPERTY(VideoReceiver*   thermalVideoReceiver    READ    thermalVideoReceiver                        CONSTANT)
    Q_PROPERTY(double           aspectRatio             READ    aspectRatio                                 NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           thermalAspectRatio      READ    thermalAspectRatio                          NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           hfov                    READ    hfov                                        NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           thermalHfov             READ    thermalHfov                                 NOTIFY aspectRatioChanged)
    Q_PROPERTY(bool             autoStreamConfigured    READ    autoStreamConfigured                        NOTIFY autoStreamConfiguredChanged)
    Q_PROPERTY(bool             hasThermal              READ    hasThermal                                  NOTIFY aspectRatioChanged)
    Q_PROPERTY(QString          imageFile               READ    imageFile                                   NOTIFY imageFileChanged)
    Q_PROPERTY(bool             streaming               READ    streaming                                   NOTIFY streamingChanged)
    Q_PROPERTY(bool             decoding                READ    decoding                                    NOTIFY decodingChanged)
    Q_PROPERTY(bool             recording               READ    recording                                   NOTIFY recordingChanged)
    Q_PROPERTY(QSize            videoSize               READ    videoSize                                   NOTIFY videoSizeChanged)

    virtual bool        hasVideo            ();
    virtual bool        isGStreamer         ();
    virtual bool        isTaisync           () { return _isTaisync; }
    virtual bool        fullScreen          () { return _fullScreen; }
    virtual QString     videoSourceID       () { return _videoSourceID; }
    virtual double      aspectRatio         ();
    virtual double      thermalAspectRatio  ();
    virtual double      hfov                ();
    virtual double      thermalHfov         ();
    virtual bool        autoStreamConfigured();
    virtual bool        hasThermal          ();
    virtual QString     imageFile           ();

    bool streaming(void) {
        return _streaming;
    }

    bool decoding(void) {
        return _decoding;
    }

    bool recording(void) {
        return _recording;
    }

    QSize videoSize(void) {
        const quint32 size = _videoSize;
        return QSize((size >> 16) & 0xFFFF, size & 0xFFFF);
    }

// FIXME: AV: they should be removed after finishing multiple video stream support
// new arcitecture does not assume direct access to video receiver from QML side, even if it works for now
    virtual VideoReceiver*  videoReceiver           () { return _videoReceiver; }
    virtual VideoReceiver*  thermalVideoReceiver    () { return _thermalVideoReceiver; }

#if defined(QGC_DISABLE_UVC)
    virtual bool        uvcEnabled          () { return false; }
#else
    virtual bool        uvcEnabled          ();
#endif

    virtual void        setfullScreen       (bool f);
    virtual void        setIsTaisync        (bool t) { _isTaisync = t;  emit isTaisyncChanged(); }

    // Override from QGCTool
    virtual void        setToolbox          (QGCToolbox *toolbox);

    Q_INVOKABLE void startVideo     ();
    Q_INVOKABLE void stopVideo      ();

    Q_INVOKABLE void startRecording (const QString& videoFile = QString());
    Q_INVOKABLE void stopRecording  ();

    Q_INVOKABLE void grabImage(const QString& imageFile);

signals:
    void hasVideoChanged            ();
    void isGStreamerChanged         ();
    void videoSourceIDChanged       ();
    void fullScreenChanged          ();
    void isAutoStreamChanged        ();
    void isTaisyncChanged           ();
    void aspectRatioChanged         ();
    void autoStreamConfiguredChanged();
    void imageFileChanged           ();
    void streamingChanged           ();
    void decodingChanged            ();
    void recordingChanged           ();
    void recordingStarted           ();
    void videoSizeChanged           ();

protected slots:
    void _videoSourceChanged        ();
    void _udpPortChanged            ();
    void _rtspUrlChanged            ();
    void _tcpUrlChanged             ();
    void _lowLatencyModeChanged     ();
    void _updateUVC                 ();
    void _setActiveVehicle          (Vehicle* vehicle);
    void _aspectRatioChanged        ();
    void _connectionLostChanged     (bool connectionLost);

protected:
    friend class FinishVideoInitialization;

    void _initVideo                 ();
    bool _updateSettings            ();
    bool _updateVideoUri            (const QString& uri);
    bool _updateThermalVideoUri     (const QString& uri);
    void _cleanupOldVideos          ();
    void _restartVideo              ();
    void _startReceiver             (unsigned id);
    void _stopReceiver              (unsigned id);

protected:
    QString                 _videoFile;
    QString                 _imageFile;
    SubtitleWriter          _subtitleWriter;
    bool                    _isTaisync              = false;
    VideoReceiver*          _videoReceiver          = nullptr;
    QAtomicInteger<bool>    _streaming              = false;
    QAtomicInteger<bool>    _decoding               = false;
    QAtomicInteger<bool>    _recording              = false;
    QAtomicInteger<quint32> _videoSize              = 0;
    VideoReceiver*          _thermalVideoReceiver   = nullptr;
    bool                    _enableVideoRestart     = false;
    void*                   _videoSink              = nullptr;
    void*                   _thermalVideoSink       = nullptr;
    VideoSettings*          _videoSettings          = nullptr;
    QString                 _videoUri;
    QString                 _thermalVideoUri;
    QString                 _videoSourceID;
    bool                    _fullScreen             = false;
    Vehicle*                _activeVehicle          = nullptr;
};

#endif
