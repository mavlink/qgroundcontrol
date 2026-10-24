#pragma once

#include <functional>
#include <memory>

#include <QtCore/QMetaObject>
#include <QtCore/QString>

#include "GstScoped.h"
#include "VideoReceiver.h"

class GstReceiverLink;
class GstReceiverWorker;
class GstVideoReceiverTest;
class QGCQVideoSinkController;
class QThread;

/// The GStreamer VideoReceiver.
///
/// This GUI-thread facade owns the display sink, built from VideoSettings once there is an output to show and rebuilt
/// by restart(), and binds it to the output's QVideoSink through a QGCQVideoSinkController. It forwards every request
/// to a worker thread that runs the pipeline (one PipelineSession per connection) and steers it towards what was asked
/// for: reconnecting with backoff, replacing a failed decoder, resuming a recording in a new part. The worker reports
/// back through queued calls, so every signal is emitted on the GUI thread.
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

    /// The size @p frame is presented at: its format's viewport (the picture without decoder padding or crop), else the
    /// whole frame, turned by the rotation Qt presents it with. Invalid for an invalid frame.
    static QSize presentedSize(const QVideoFrame& frame);

    /// Writes PipelineDotDumper::snapshot() under @p tag; callable from QML.
    Q_INVOKABLE void dumpPipelineGraph(const QString& tag = QStringLiteral("manual"));

signals:
    void decoderNameChanged();

protected:
    void onConfigChanged(const Config& previous) override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onVideoOutputChanged() override;
    void onStartRecording(const QString& file, FileFormat format) override;
    void onStopRecording() override;

private:
    friend class GstReceiverWorker;
    friend class GstVideoReceiverTest;

    void _postToWorker(std::function<void(GstReceiverWorker&)> task);
    /// Hands the worker the display sink, or none without an output, once the current call stack is done.
    void _scheduleVideoSinkUpdate();
    /// Builds the display sink and its controller for @p videoSink.
    bool _buildDisplaySink(QVideoSink* videoSink);
    void _releaseDisplaySink();
    void _bindVideoSink(QVideoSink* videoSink);
    void _setDecoderName(const QString& name);
    void _releaseDisplayedFrame();
    /// Runs @p task with the current pipeline (or null) on the worker, and waits for it. For tests.
    bool _runOnPipeline(const std::function<void(GstElement* pipeline)>& task);

    std::shared_ptr<GstReceiverLink> _link;
    QThread* _workerThread = nullptr;
    GstReceiverWorker* _worker = nullptr;
    GStreamer::GstElementPtr _displaySink;
    QGCQVideoSinkController* _sinkController = nullptr;
    QMetaObject::Connection _videoSizeConnection;
    bool _videoSinkUpdatePending = false;
    QString _decoderName;
};
