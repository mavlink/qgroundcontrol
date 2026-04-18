#pragma once

#include <QtCore/QLoggingCategory>

#include "VideoRecorder.h"

Q_DECLARE_LOGGING_CATEGORY(GstTeeRecorderLog)

#ifdef QGC_GST_STREAMING

class GstVideoReceiver;

/// VideoRecorder implementation backed by GStreamer's tee-based recording branch.
///
/// Attaches a filesink sub-pipeline to the tee element in GstVideoReceiver's
/// pipeline. Recording is bit-accurate (no re-encode) — it muxes the elementary
/// stream (H.264/H.265/AV1/VP8/VP9) directly to a container file.
///
/// Holds a non-owning GstVideoReceiver*. The receiver owns the actual
/// GstRecordingBranch on its worker thread; this object only controls recording
/// intent and translates branch signals into the VideoRecorder interface.
class GstTeeRecorder : public VideoRecorder
{
    Q_OBJECT

public:
    explicit GstTeeRecorder(QObject* parent = nullptr);
    ~GstTeeRecorder() override;

    bool start(const QString& path, QMediaFormat::FileFormat format) override;
    void stop() override;
    [[nodiscard]] Capabilities capabilities() const override;

    /// Bind to a GstVideoReceiver. Must be called before start().
    /// Does NOT transfer ownership. The receiver must outlive this recorder.
    void setReceiver(GstVideoReceiver* receiver);

private slots:
    /// Invoked when GstVideoReceiver reports the first recorded keyframe.
    void _onBranchStarted(const QString& path);
    /// Invoked when GstVideoReceiver signals the recording branch EOS is drained.
    void _onBranchEosDrained(bool success);
    /// Invoked when GstVideoReceiver is about to tear down its pipeline.
    void _onPipelineStopping();

private:
    GstVideoReceiver* _receiver = nullptr;
    QMetaObject::Connection _eosDrainedConn;
    QMetaObject::Connection _startedConn;
    QMetaObject::Connection _pipelineStoppingConn;
};

#else  // QGC_GST_STREAMING not defined

class GstTeeRecorder : public VideoRecorder
{
    Q_OBJECT
public:
    explicit GstTeeRecorder(QObject* parent = nullptr);
    ~GstTeeRecorder() override = default;
    bool start(const QString& path, QMediaFormat::FileFormat format) override;
    void stop() override;
    [[nodiscard]] Capabilities capabilities() const override;
};

#endif  // QGC_GST_STREAMING
