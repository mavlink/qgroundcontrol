#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtMultimedia/QMediaFormat>
#include <memory>
#include <utility>

#include "VideoRecorder.h"
#include "VideoSourceResolver.h"

#include <gst/gst.h>

Q_DECLARE_LOGGING_CATEGORY(GstNativeRecorderLog)

class GstRemuxPipeline;

/// Lossless recorder for sources already handled by GStreamer transport ingest.
///
/// This is deliberately a recording-side service, not a display receiver. It
/// opens the original source through GstSourceFactory, muxes the encoded video
/// elementary stream, and writes directly to disk while QtMultimedia remains
/// the only display/decode path.
class GstNativeRecorder : public VideoRecorder
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GstNativeRecorder)

public:
    explicit GstNativeRecorder(VideoSourceResolver::SourceDescriptor source, QObject* parent = nullptr);
    ~GstNativeRecorder() override;

    bool start(const QString& path, QMediaFormat::FileFormat format) override;
    void stop() override;
    [[nodiscard]] Capabilities capabilities() const override;

    [[nodiscard]] static bool supportsSource(const VideoSourceResolver::SourceDescriptor& source);

#ifdef QGC_UNITTEST_BUILD
    void handleBusMessageForTest(GstMessage* message);
#endif

private:
    struct MuxerSpec
    {
        const char* factory = nullptr;
        QMediaFormat::FileFormat format = QMediaFormat::UnspecifiedFormat;
    };

    [[nodiscard]] static MuxerSpec _muxerForFormat(QMediaFormat::FileFormat format);
    [[nodiscard]] static bool _factoryAvailable(const char* factoryName);
    [[nodiscard]] static QList<QMediaFormat::FileFormat> _availableFormats();

    [[nodiscard]] bool _startPipeline(QMediaFormat::FileFormat format);
    void _handlePipelineError(VideoReceiver::ErrorCategory category, const QString& message);
    void _handlePipelineEos();
    void _handleVideoLinked();
    void _finalizeStop(bool emitStopped);
    void _failStartOrRecording(const QString& message);
    void _confirmStarted();
    void _removeIncompleteOutput();

    VideoSourceResolver::SourceDescriptor _source;
    std::unique_ptr<GstRemuxPipeline> _pipeline;
    QTimer _startTimeoutTimer;
    QTimer _stopTimeoutTimer;
    bool _videoLinked = false;
    bool _pipelinePlaying = false;
    bool _startedEmitted = false;
    bool _outputPreexisting = false;
};
