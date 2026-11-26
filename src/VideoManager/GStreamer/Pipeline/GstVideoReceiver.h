#pragma once

#include <chrono>
#include <functional>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaObject>
#include <QtCore/QString>

#include "GstReceiverRequest.h"
#include "GstReceiverTimings.h"
#include "GstScoped.h"
#include "VideoReceiver.h"

class GstReceiverWorker;
class GstVideoReceiverTestBase;
class QGCQVideoSinkController;
class QThread;
class QTimer;

/// Also logs the receiver's worker, its PipelineSession with the branches, and PipelineDotDumper.
Q_DECLARE_LOGGING_CATEGORY(GstVideoReceiverLog)

/// The GStreamer VideoReceiver.
///
/// This GUI-thread facade owns the display sink, built from VideoSettings once there is an output to show and rebuilt
/// by restart(), and binds it to the output's QVideoSink through a QGCQVideoSinkController. After every request it
/// hands the whole GstReceiverRequest to a GstReceiverWorker on a thread of its own, which runs the pipeline
/// (one PipelineSession per connection) and steers it towards what was asked for: reconnecting with backoff, replacing
/// a failed decoder, resuming a recording in a new part. The worker reports its GstReceiverStatus and events back
/// through queued signals, so every signal is emitted on the GUI thread.
///
/// A worker that waits for a streaming thread that never returns (a decoder deadlocked in its library, a recorder
/// stuck in the kernel) serves no request any more, and nothing can stop its pipeline. The receiver notices that the
/// worker runs none of the tasks it posts to it, which are its requests and, between them, a heartbeat: it then tells
/// the user, once, through errorOccurred(), and reports State::Stopped without decoding unless frames still reach the
/// output. It follows the worker again when that runs a task after all.
class GstVideoReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    /// How often the receiver checks that its worker runs what it is posted, and posts it a heartbeat when that is
    /// nothing else.
    // Above the worker's longest waits in a row: a recording's end (3 s), RTSP TEARDOWN (1 s), a TCP connect (3 s).
    static constexpr std::chrono::seconds kHeartbeatInterval{10};

    explicit GstVideoReceiver(QObject* parent = nullptr, const GstReceiverTimings& timings = {});
    /// Finalizes a recording and stops the pipeline, waiting at most a few seconds for the worker.
    ~GstVideoReceiver() override;

    /// Factory name of the decoder decodebin3 plugged last; empty while the stream has no decoder: before it streams,
    /// without a video output, and while a decoder is being replaced. For tests.
    QString decoderName() const { return _decoderName; }

    /// Checks the worker every @p interval from now on, instead of kHeartbeatInterval. For tests, which can't wait
    /// that long.
    void setHeartbeatInterval(std::chrono::milliseconds interval);

protected:
    void onConfigChanged() override;
    void onActiveChanged() override;
    void onRestartRequested() override;
    void onReconnectRequested() override;
    void onVideoOutputChanged() override;
    void onStartRecording(const QString& file, FileFormat format, const RecordingEncoding& encoding) override;
    void onStopRecording() override;

private:
    friend class GstVideoReceiverTestBase;

    /// Queues @p task for the worker's thread, which reports each one it has run.
    void _postToWorker(std::function<void(GstReceiverWorker&)> task);
    /// Hands _request, brought up to date, to the worker.
    void _postRequest();
    void _onWorkerStatus(const GstReceiverStatus& status);
    void _applyStatus(const GstReceiverStatus& status);
    /// Counts the checks at which the worker has run none of its tasks since the check before.
    void _checkWorker();
    void _onWorkerTaskDone();
    /// The status of a stream whose worker is stuck. @p frameShown: a frame reached the output since the last check.
    void _applyStuckStatus(bool frameShown);
    /// Builds the display sink and its controller, bound to @p videoSink.
    bool _buildDisplaySink(QVideoSink* videoSink);
    void _releaseDisplaySink();
    /// Binds the display sink, built first when there is none, to @p videoSink and follows the output's window.
    void _bindVideoSink(QVideoSink* videoSink);

    QThread* _workerThread = nullptr;
    GstReceiverWorker* _worker = nullptr;
    /// What was asked of the worker last.
    GstReceiverRequest _request;
    GStreamer::GstElementPtr _displaySink;
    QGCQVideoSinkController* _sinkController = nullptr;
    QMetaObject::Connection _videoSizeConnection;
    QString _decoderName;

    QTimer* const _heartbeatTimer;
    quint64 _tasksPosted = 0;
    quint64 _tasksDone = 0;
    /// _tasksDone at the last check of the worker.
    quint64 _tasksDoneAtCheck = 0;
    /// Checks in a row at which the worker had tasks left and had run none since the check before.
    int _stalledChecks = 0;
    bool _workerStuck = false;
    /// What the worker reported last, applied again once a stuck worker goes on.
    GstReceiverStatus _workerStatus;
    /// A frame reached the output since the last check of the worker.
    bool _frameShown = false;
};
