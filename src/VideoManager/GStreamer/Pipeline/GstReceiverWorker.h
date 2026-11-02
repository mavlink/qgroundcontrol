#pragma once

#include <optional>

#include <QtCore/QObject>
#include <QtCore/QString>

#include "GstReceiverRequest.h"
#include "PipelineDotDumper.h"
#include "PipelineSession.h"
#include "RecordingPlan.h"
#include "StreamSupervisor.h"
#include "StreamWatchdog.h"
#include "VideoReceiver.h"
#include "VideoStreamStats.h"
#include "VideoStreamStatsSampler.h"

class QTimer;

/// GstVideoReceiver's worker, on a thread of its own: steers a PipelineSession towards the receiver's latest
/// GstReceiverRequest (active, config, video sink, restarts, recording), reconnecting after a failure as its
/// StreamSupervisor decides.
///
/// Reports back through its signals only, so it can outlive a receiver that stopped waiting for it.
class GstReceiverWorker : public QObject, private PipelineSession::Listener
{
    Q_OBJECT

public:
    using Request = GstReceiverRequest;
    using RecordingRequest = GstRecordingRequest;
    using Status = GstReceiverStatus;

    explicit GstReceiverWorker(QObject* parent = nullptr);
    ~GstReceiverWorker() override;

    /// Steers the stream towards @p request, acting on what differs from the previous one.
    void apply(const Request& request);
    void dumpPipelineGraph(const QString& tag) const;
    /// Stops everything and quits the thread.
    void shutdown();

    GstElement* pipeline() const { return _session ? _session->pipeline() : nullptr; }

signals:
    void statusChanged(const GstReceiverStatus& status);
    void recordingStarted(const QString& file);
    void recordingFinished(const QString& file, const QString& error);
    void errorOccurred(const QString& message);

private:
    void _onActiveChanged();
    void _onConfigChanged(const VideoReceiver::Config& previous);
    void _startRecording(const RecordingRequest& request);
    void _stopRecording();

    template <typename T>
    void _updateStatus(T Status::* field, const T& value)
    {
        if (_status.*field != value) {
            _status.*field = value;
            emit statusChanged(_status);
        }
    }

    void _reconcile();
    /// Stops the stream and connects again now.
    void _restartStream();
    void _startSession();
    void _stopSession();
    /// After a failure: reconnects with backoff, or gives up.
    void _recover(const PipelineSession::Failure& failure);
    /// The supervisor halted the stream after @p message.
    void _onHalted(const QString& message);
    void _endRecording();
    std::optional<RecordingBranch::Request> _recordingRequest() const;
    void _onWatchdogTick();
    void _sampleStats();
    QString _redactedUri() const;

    void onSessionStreaming() override;
    void onSessionFailed(const PipelineSession::Failure& failure) override;
    void onDecodingChanged(bool decoding) override;
    void onDecoderNameChanged(const QString& name) override;
    void onDecoderFailing() override;
    void onRecordingFileOpened(const QString& file) override;
    void onRecordingFileClosed(const QString& file, const QString& error) override;
    void onRecordingFailed(const QString& file, const QString& error, bool fileOpened) override;

    QTimer* const _watchdogTimer;
    StreamSupervisor* const _supervisor;
    StreamWatchdog _watchdog;
    PipelineDotDumper _dotDumper;

    Request _requested;
    Status _status;
    /// The recording asked for, until it stops or fails; after a reconnect it continues in the next part.
    std::optional<RecordingPlan> _recording;

    PipelineSession* _session = nullptr;
    int _statsTicks = 0;
    VideoStreamStatsSampler _statsSampler;
};
