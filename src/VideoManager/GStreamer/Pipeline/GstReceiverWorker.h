#pragma once

#include <memory>
#include <optional>

#include <QtCore/QObject>
#include <QtCore/QString>

#include "PipelineDotDumper.h"
#include "PipelineSession.h"
#include "RecordingPlan.h"
#include "StreamSupervisor.h"
#include "StreamWatchdog.h"
#include "VideoReceiver.h"
#include "VideoStreamStats.h"
#include "VideoStreamStatsSampler.h"

class QTimer;

/// GstVideoReceiver's worker, on a thread of its own: holds what the receiver was asked for (active, config, video
/// sink, recording) and keeps a PipelineSession in line with it, reconnecting after a failure as its StreamSupervisor
/// decides.
///
/// Reports back through its signals only, so it can outlive a receiver that stopped waiting for it.
class GstReceiverWorker : public QObject, private PipelineSession::Listener
{
    Q_OBJECT

public:
    explicit GstReceiverWorker(QObject* parent = nullptr);
    ~GstReceiverWorker() override;

    void setActive(bool active, const QString& name, const VideoReceiver::Config& config);
    void setConfig(const VideoReceiver::Config& config);
    void restart(const std::shared_ptr<GstElement>& videoSink);
    void setVideoSink(const std::shared_ptr<GstElement>& videoSink);
    void startRecording(const QString& file, VideoReceiver::FileFormat format);
    void stopRecording();
    void dumpPipelineGraph(const QString& tag) const;
    /// Stops everything and quits the thread.
    void shutdown();

    GstElement* pipeline() const { return _session ? _session->pipeline() : nullptr; }

signals:
    void stateChanged(VideoReceiver::State state);
    void decodingChanged(bool decoding);
    void decoderNameChanged(const QString& name);
    void recordingChanged(bool recording);
    void recordingStarted(const QString& file);
    void recordingFinished(const QString& file, const QString& error);
    void errorOccurred(const QString& message);
    /// About once a second while streaming, and an invalid one once the session stops.
    void statsChanged(const VideoStreamStats& stats);

private:
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

    bool _active = false;
    VideoReceiver::Config _config;
    std::shared_ptr<GstElement> _videoSink;
    /// What startRecording() asked for; after a reconnect it continues in the next part.
    std::optional<RecordingPlan> _recording;

    PipelineSession* _session = nullptr;
    int _statsTicks = 0;
    VideoStreamStatsSampler _statsSampler;
};
