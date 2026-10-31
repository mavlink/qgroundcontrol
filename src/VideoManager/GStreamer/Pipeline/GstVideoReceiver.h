#pragma once

#include <functional>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaObject>
#include <QtCore/QString>

#include "GstScoped.h"
#include "VideoReceiver.h"

class GstReceiverWorker;
class GstVideoReceiverTest;
class QGCQVideoSinkController;
class QThread;

/// Also logs the receiver's worker, its PipelineSession with the branches, and PipelineDotDumper.
Q_DECLARE_LOGGING_CATEGORY(GstVideoReceiverLog)

/// The GStreamer VideoReceiver.
///
/// This GUI-thread facade owns the display sink, built from VideoSettings once there is an output to show and rebuilt
/// by restart(), and binds it to the output's QVideoSink through a QGCQVideoSinkController. It forwards every request
/// to a GstReceiverWorker on a thread of its own, which runs the pipeline (one PipelineSession per connection) and
/// steers it towards what was asked for: reconnecting with backoff, replacing a failed decoder, resuming a recording in
/// a new part. The worker reports back through queued signals, so every signal is emitted on the GUI thread.
class GstVideoReceiver : public VideoReceiver
{
    Q_OBJECT
    Q_PROPERTY(QString decoderName READ decoderName NOTIFY decoderNameChanged)

public:
    explicit GstVideoReceiver(QObject* parent = nullptr);
    /// Finalizes a recording and stops the pipeline, waiting at most a few seconds for the worker.
    ~GstVideoReceiver() override;

    /// Factory name of the decoder decodebin3 plugged last, empty before the first one.
    QString decoderName() const { return _decoderName; }

    /// Writes PipelineDotDumper::snapshot() under @p tag; callable from QML.
    Q_INVOKABLE void dumpPipelineGraph(const QString& tag = QStringLiteral("manual"));

signals:
    void decoderNameChanged();

protected:
    void onConfigChanged() override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onVideoOutputChanged() override;
    void onStartRecording(const QString& file, FileFormat format) override;
    void onStopRecording() override;

private:
    friend class GstVideoReceiverTest;

    void _postToWorker(std::function<void(GstReceiverWorker&)> task);
    /// Builds the display sink and its controller, bound to @p videoSink.
    bool _buildDisplaySink(QVideoSink* videoSink);
    void _releaseDisplaySink();
    /// Binds the display sink, built first when there is none, to @p videoSink and follows the output's window.
    void _bindVideoSink(QVideoSink* videoSink);
    void _setDecoderName(const QString& name);
    /// Runs @p task with the current pipeline (or null) on the worker, and waits for it. For tests.
    bool _runOnPipeline(const std::function<void(GstElement* pipeline)>& task);

    QThread* _workerThread = nullptr;
    GstReceiverWorker* _worker = nullptr;
    GStreamer::GstElementPtr _displaySink;
    QGCQVideoSinkController* _sinkController = nullptr;
    QMetaObject::Connection _videoSizeConnection;
    QString _decoderName;
};
