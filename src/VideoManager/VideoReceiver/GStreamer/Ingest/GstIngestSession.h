#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtMultimedia/QMediaFormat>

#include "VideoReceiver.h"
#include "VideoSourceResolver.h"

#include <gst/gst.h>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(GstIngestSessionLog)

class GstStreamDevice;
class GstRemuxPipeline;
class QIODevice;

/// GStreamer sidecar for transports that QtMultimedia should display but may
/// need GStreamer-specific ingest. The sidecar normalizes the source into a
/// MPEG-TS QIODevice stream; QtMultimedia/FFmpeg owns decode and presentation.
class GstIngestSession : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GstIngestSession)

public:
    explicit GstIngestSession(QObject* parent = nullptr);
    ~GstIngestSession() override;

    [[nodiscard]] bool start(const VideoSourceResolver::SourceDescriptor& source, bool lowLatency);
    void stop();

    [[nodiscard]] QString playbackUri() const { return _playbackUri; }
    [[nodiscard]] QIODevice* playbackDevice() const;
    [[nodiscard]] QUrl playbackDeviceUrl() const { return _playbackDeviceUrl; }
    [[nodiscard]] bool running() const;
    [[nodiscard]] bool startRecording(const QString& path, QMediaFormat::FileFormat format);
    void stopRecording();
    [[nodiscard]] bool isRecording() const { return _recordingOutputId >= 0; }

#ifdef QGC_UNITTEST_BUILD
    void handleBusMessageForTest(GstMessage* message);
#endif

signals:
    void errorOccurred(VideoReceiver::ErrorCategory category, const QString& message);
    void endOfStream();

private:
    [[nodiscard]] GstFlowReturn _handleNewSample();

    static GstFlowReturn _onNewSample(GstElement* sink, gpointer userData);

    std::unique_ptr<GstRemuxPipeline> _pipeline;
    std::unique_ptr<GstStreamDevice> _device;
    QString _playbackUri;
    QUrl _playbackDeviceUrl;
    int _recordingOutputId = -1;
};
