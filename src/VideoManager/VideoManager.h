/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QSize>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

#define MAX_VIDEO_RECEIVERS 2

class FinishVideoInitialization;
class SubtitleWriter;
class Vehicle;
class VideoReceiver;
class VideoSettings;

class VideoManager : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(bool     gstreamerEnabled        READ gstreamerEnabled                           CONSTANT)
    Q_PROPERTY(bool     qtmultimediaEnabled     READ qtmultimediaEnabled                        CONSTANT)
    Q_PROPERTY(bool     uvcEnabled              READ uvcEnabled                                 CONSTANT)
    Q_PROPERTY(bool     autoStreamConfigured    READ autoStreamConfigured                       NOTIFY autoStreamConfiguredChanged)
    Q_PROPERTY(bool     decoding                READ decoding                                   NOTIFY decodingChanged)
    Q_PROPERTY(bool     fullScreen              READ fullScreen             WRITE setfullScreen NOTIFY fullScreenChanged)
    Q_PROPERTY(bool     hasThermal              READ hasThermal                                 NOTIFY decodingChanged)
    Q_PROPERTY(bool     hasVideo                READ hasVideo                                   NOTIFY hasVideoChanged)
    Q_PROPERTY(bool     isStreamSource          READ isStreamSource                             NOTIFY isStreamSourceChanged)
    Q_PROPERTY(bool     isUvc                   READ isUvc                                      NOTIFY isUvcChanged)
    Q_PROPERTY(bool     recording               READ recording                                  NOTIFY recordingChanged)
    Q_PROPERTY(bool     streaming               READ streaming                                  NOTIFY streamingChanged)
    Q_PROPERTY(double   aspectRatio             READ aspectRatio                                NOTIFY aspectRatioChanged)
    Q_PROPERTY(double   hfov                    READ hfov                                       NOTIFY aspectRatioChanged)
    Q_PROPERTY(double   thermalAspectRatio      READ thermalAspectRatio                         NOTIFY aspectRatioChanged)
    Q_PROPERTY(double   thermalHfov             READ thermalHfov                                NOTIFY aspectRatioChanged)
    Q_PROPERTY(QSize    videoSize               READ videoSize                                  NOTIFY videoSizeChanged)
    Q_PROPERTY(QString  imageFile               READ imageFile                                  NOTIFY imageFileChanged)
    Q_PROPERTY(QString  uvcVideoSourceID        READ uvcVideoSourceID                           NOTIFY uvcVideoSourceIDChanged)

    friend class FinishVideoInitialization;

public:
    explicit VideoManager(QObject *parent = nullptr);
    ~VideoManager();

    /// Gets the singleton instance of VideoManager.
    ///     @return The singleton instance.
    static VideoManager *instance();
    static void registerQmlTypes();

    Q_INVOKABLE void grabImage(const QString &imageFile = QString());
    Q_INVOKABLE void startRecording(const QString &videoFile = QString());
    Q_INVOKABLE void startVideo();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void stopVideo();

    void init();
    void cleanup();
    bool autoStreamConfigured() const;
    bool decoding() const { return _decoding; }
    bool fullScreen() const { return _fullScreen; }
    bool gstreamerEnabled() const;
    bool hasThermal() const;
    bool hasVideo() const;
    bool isStreamSource() const;
    bool isUvc() const;
    bool qtmultimediaEnabled() const;
    bool recording() const { return _recording; }
    bool streaming() const { return _streaming; }
    bool uvcEnabled() const;
    double aspectRatio() const;
    double hfov() const;
    double thermalAspectRatio() const;
    double thermalHfov() const;
    QSize videoSize() const { return QSize((_videoSize >> 16) & 0xFFFF, _videoSize & 0xFFFF); }
    QString imageFile() const { return _imageFile; }
    QString uvcVideoSourceID() const { return _uvcVideoSourceID; }
    void setfullScreen(bool on);

signals:
    void aspectRatioChanged();
    void autoStreamConfiguredChanged();
    void decodingChanged();
    void fullScreenChanged();
    void hasVideoChanged();
    void imageFileChanged();
    void isAutoStreamChanged();
    void isStreamSourceChanged();
    void isUvcChanged();
    void recordingChanged();
    void recordingStarted();
    void streamingChanged();
    void uvcVideoSourceIDChanged();
    void videoSizeChanged();

private slots:
    bool _updateUVC();
    void _communicationLostChanged(bool communicationLost);
    void _lowLatencyModeChanged() { _restartAllVideos(); }
    void _setActiveVehicle(Vehicle *vehicle);
    void _videoSourceChanged();

private:
    bool _updateAutoStream(unsigned id);
    bool _updateSettings(unsigned id);
    bool _updateVideoUri(unsigned id, const QString &uri);
    void _initVideo();
    void _restartAllVideos();
    void _restartVideo(unsigned id);
    void _startReceiver(unsigned id);
    void _stopReceiver(unsigned id);
    static void _cleanupOldVideos();

    struct VideoReceiverData {
        VideoReceiver *receiver = nullptr;
        void *sink = nullptr;
        QString uri;
        bool started = false;
        bool lowLatencyStreaming = false;
        size_t index = 0;
        QString name;
    };
    QList<VideoReceiverData> _videoReceiverData = QList<VideoReceiverData>(MAX_VIDEO_RECEIVERS);

    SubtitleWriter *_subtitleWriter = nullptr;

    bool _initialized = false;
    bool _fullScreen = false;
    QAtomicInteger<bool> _decoding = false;
    QAtomicInteger<bool> _recording = false;
    QAtomicInteger<bool> _streaming = false;
    QAtomicInteger<quint32> _videoSize = 0;
    QString _imageFile;
    QString _uvcVideoSourceID;
    QString _videoFile;
    Vehicle *_activeVehicle = nullptr;
    VideoSettings *_videoSettings = nullptr;
};

/*===========================================================================*/

class FinishVideoInitialization : public QRunnable
{
public:
    explicit FinishVideoInitialization();
    ~FinishVideoInitialization();

    void run() final;
};
