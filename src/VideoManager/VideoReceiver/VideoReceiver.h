/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QTimer>

class QGCVideoStreamInfo;
class QQuickItem;

class VideoReceiver : public QObject
{
    Q_OBJECT

public:
    explicit VideoReceiver(QObject *parent = nullptr)
        : QObject(parent)
    {}

    bool isThermal() const { return (_name == QStringLiteral("thermalVideo")); }

    void *sink() { return _sink; }
    QQuickItem *widget() { return _widget; }
    QString name() const { return _name; }
    QString uri() const { return _uri; }
    bool started() const { return _started; }
    bool lowLatency() const { return _lowLatency; }
    QGCVideoStreamInfo *videoStreamInfo() { return _videoStreamInfo; }
    QString recordingOutput() const { return _recordingOutput; }

    virtual void setSink(void *sink) { if (sink != _sink) { _sink = sink; emit sinkChanged(_sink); } }
    virtual void setWidget(QQuickItem *widget) { if (widget != _widget) { _widget = widget; emit widgetChanged(_widget); } }
    void setName(const QString &name) { if (name != _name) { _name = name; emit nameChanged(_name); } }
    void setUri(const QString &uri) { if (uri != _uri) { _uri = uri; emit uriChanged(_uri); } }
    void setStarted(bool started) { if (started != _started) { _started = started; emit startedChanged(_started); } }
    void setLowLatency(bool lowLatency) { if (lowLatency != _lowLatency) { _lowLatency = lowLatency; emit lowLatencyChanged(_lowLatency); } }
    void setVideoStreamInfo(QGCVideoStreamInfo *videoStreamInfo) { if (videoStreamInfo != _videoStreamInfo) { _videoStreamInfo = videoStreamInfo; emit videoStreamInfoChanged(); } }

    // QMediaFormat::FileFormat
    enum FILE_FORMAT {
        FILE_FORMAT_MIN = 0,
        FILE_FORMAT_MKV = FILE_FORMAT_MIN,
        FILE_FORMAT_MOV,
        FILE_FORMAT_MP4,
        FILE_FORMAT_MAX = FILE_FORMAT_MP4
    };
    Q_ENUM(FILE_FORMAT)
    static bool isValidFileFormat(FILE_FORMAT format) { return ((format >= FILE_FORMAT_MIN) && (format <= FILE_FORMAT_MAX)); }

    enum STATUS {
        STATUS_MIN = 0,
        STATUS_OK = STATUS_MIN,
        STATUS_FAIL,
        STATUS_INVALID_STATE,
        STATUS_INVALID_URL,
        STATUS_NOT_IMPLEMENTED,
        STATUS_MAX = STATUS_NOT_IMPLEMENTED
    };
    Q_ENUM(STATUS)
    static bool isValidStatus(STATUS status) { return ((status >= STATUS_MIN) && (status <= STATUS_MAX)); }

signals:
    void timeout();
    void streamingChanged(bool active);
    void decodingChanged(bool active);
    void recordingChanged(bool active);
    void recordingStarted(const QString &filename);
    void videoSizeChanged(QSize size);

    void sinkChanged(void *sink);
    void nameChanged(const QString &name);
    void uriChanged(const QString &uri);
    void startedChanged(bool started);
    void lowLatencyChanged(bool lowLatency);
    void videoStreamInfoChanged();
    void widgetChanged(QQuickItem *widget);

    void onStartComplete(STATUS status);
    void onStopComplete(STATUS status);
    void onStartDecodingComplete(STATUS status);
    void onStopDecodingComplete(STATUS status);
    void onStartRecordingComplete(STATUS status);
    void onStopRecordingComplete(STATUS status);
    void onTakeScreenshotComplete(STATUS status);

public slots:
    virtual void start(uint32_t timeout) = 0;
    virtual void stop() = 0;
    virtual void startDecoding(void *sink) = 0;
    virtual void stopDecoding() = 0;
    virtual void startRecording(const QString &videoFile, FILE_FORMAT format) = 0;
    virtual void stopRecording() = 0;
    virtual void takeScreenshot(const QString &imageFile) = 0;

protected:
    void *_sink = nullptr;
    QQuickItem *_widget = nullptr;
    QGCVideoStreamInfo *_videoStreamInfo = nullptr;
    QString _name;
    QString _uri;
    bool _started = false;
    bool _decoding = false;
    bool _recording = false;
    bool _streaming = false;
    bool _lowLatency = false;
    bool _resetVideoSink = false;
    bool _endOfStream = false;
    bool _removingDecoder = false;
    bool _removingRecorder = false;
    // buffer:
    //      -1 - disable buffer and video sync
    //      0 - default buffer length
    //      N - buffer length, ms
    int _buffer = 0;
    qint64 _lastSourceFrameTime = 0;
    qint64 _lastVideoFrameTime = 0;
    QTimer _watchdogTimer;
    uint32_t _signalDepth = 0;
    uint32_t _timeout = 0;
    QString _recordingOutput;

    // bool _initialized = false;
    // bool _fullScreen = false;
    // QSize _videoSize;
    // QString _imageFile;
};
