/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QtCore/QSize>
#include <QtCore/QRunnable>
#include <QtCore/QLoggingCategory>

#include "QGCToolbox.h"

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

#define MAX_VIDEO_RECEIVERS 2

class VideoSettings;
class Vehicle;
class Joystick;
class VideoReceiver;
class SubtitleWriter;

class VideoManager : public QGCTool
{
    Q_OBJECT

    Q_PROPERTY(bool             hasVideo                READ    hasVideo                                    NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isStreamSource          READ    isStreamSource                              NOTIFY isStreamSourceChanged)
    Q_PROPERTY(bool             gstreamerEnabled        READ    gstreamerEnabled                            CONSTANT)
    Q_PROPERTY(bool             isUvc                   READ    isUvc                                       NOTIFY isUvcChanged)
    Q_PROPERTY(QString          uvcVideoSourceID        READ    uvcVideoSourceID                            NOTIFY uvcVideoSourceIDChanged)
    Q_PROPERTY(bool             uvcEnabled              READ    uvcEnabled                                  CONSTANT)
    Q_PROPERTY(bool             qtmultimediaEnabled     READ    qtmultimediaEnabled                         CONSTANT)
    Q_PROPERTY(bool             fullScreen              READ    fullScreen      WRITE   setfullScreen       NOTIFY fullScreenChanged)
    Q_PROPERTY(double           aspectRatio             READ    aspectRatio                                 NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           thermalAspectRatio      READ    thermalAspectRatio                          NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           hfov                    READ    hfov                                        NOTIFY aspectRatioChanged)
    Q_PROPERTY(double           thermalHfov             READ    thermalHfov                                 NOTIFY aspectRatioChanged)
    Q_PROPERTY(bool             autoStreamConfigured    READ    autoStreamConfigured                        NOTIFY autoStreamConfiguredChanged)
    Q_PROPERTY(bool             hasThermal              READ    hasThermal                                  NOTIFY decodingChanged)
    Q_PROPERTY(QString          imageFile               READ    imageFile                                   NOTIFY imageFileChanged)
    Q_PROPERTY(bool             streaming               READ    streaming                                   NOTIFY streamingChanged)
    Q_PROPERTY(bool             decoding                READ    decoding                                    NOTIFY decodingChanged)
    Q_PROPERTY(bool             recording               READ    recording                                   NOTIFY recordingChanged)
    Q_PROPERTY(QSize            videoSize               READ    videoSize                                   NOTIFY videoSizeChanged)

public:
    VideoManager(QGCApplication* app, QGCToolbox* toolbox);
    virtual ~VideoManager();

    void setToolbox(QGCToolbox *toolbox) override;

    virtual bool        hasVideo            () const;
    virtual bool        isStreamSource      () const;
    virtual bool        isUvc               () const;
    virtual bool        fullScreen          () const { return _fullScreen; }
    virtual QString     uvcVideoSourceID    () const { return _uvcVideoSourceID; }
    virtual double      aspectRatio         () const;
    virtual double      thermalAspectRatio  () const;
    virtual double      hfov                () const;
    virtual double      thermalHfov         () const;
    virtual bool        autoStreamConfigured() const;
    virtual bool        hasThermal          () const;
    virtual QString     imageFile           () const { return _imageFile; }

    bool streaming() const { return _streaming; }
    bool decoding() const { return _decoding; }
    bool recording() const { return _recording; }
    QSize videoSize() const { return QSize((_videoSize >> 16) & 0xFFFF, _videoSize & 0xFFFF); }

    virtual bool gstreamerEnabled() const;
    virtual bool uvcEnabled() const;
    virtual bool qtmultimediaEnabled() const;

    virtual void setfullScreen(bool on);

    Q_INVOKABLE void startVideo();
    Q_INVOKABLE void stopVideo();

    Q_INVOKABLE void startRecording(const QString& videoFile = QString());
    Q_INVOKABLE void stopRecording();

    Q_INVOKABLE void grabImage(const QString& imageFile = QString());

signals:
    void hasVideoChanged            ();
    void isStreamSourceChanged      ();
    void isUvcChanged               ();
    void uvcVideoSourceIDChanged    ();
    void fullScreenChanged          ();
    void isAutoStreamChanged        ();
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
    bool _updateUVC                 ();
    void _setActiveVehicle          (Vehicle* vehicle);
    void _communicationLostChanged  (bool communicationLost);

protected:
    friend class FinishVideoInitialization;

    void _initVideo       ();
    bool _updateSettings  (unsigned id);
    bool _updateVideoUri  (unsigned id, const QString& uri);
    void _cleanupOldVideos();
    void _restartAllVideos();
    void _restartVideo    (unsigned id);
    void _startReceiver   (unsigned id);
    void _stopReceiver    (unsigned id);

    QString                 _videoFile;
    QString                 _imageFile;
    SubtitleWriter*         _subtitleWriter = nullptr;

    struct VideoReceiverData {
        VideoReceiver* receiver = nullptr;
        void* sink = nullptr;
        QString uri;
        bool started = false;
        bool lowLatencyStreaming = false;
        size_t index = 0;
    };
    QList<VideoReceiverData>       _videoReceiverData = QList<VideoReceiverData>(MAX_VIDEO_RECEIVERS);
    // FIXME: AV: _videoStarted seems to be access from 3 different threads, from time to time
    // 1) Video Receiver thread
    // 2) Video Manager/main app thread
    // 3) Qt rendering thread (during video sink creation process which should happen in this thread)
    // It works for now but...

    QAtomicInteger<bool>    _streaming              = false;
    QAtomicInteger<bool>    _decoding               = false;
    QAtomicInteger<bool>    _recording              = false;
    QAtomicInteger<quint32> _videoSize              = 0;
    VideoSettings*          _videoSettings          = nullptr;
    QString                 _uvcVideoSourceID;
    bool                    _fullScreen             = false;
    Vehicle*                _activeVehicle          = nullptr;
};

class FinishVideoInitialization : public QRunnable
{
public:
    explicit FinishVideoInitialization(VideoManager* manager)
        : _manager(manager)
    {}

    void run()
    {
        _manager->_initVideo();
    }

private:
    VideoManager* _manager = nullptr;
};
