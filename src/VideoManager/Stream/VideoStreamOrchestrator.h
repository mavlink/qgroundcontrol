#pragma once

#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>

#include <functional>

#include "VideoStream.h"  // Role enum required for templated iterators

Q_DECLARE_LOGGING_CATEGORY(VideoStreamOrchestratorLog)

class QGCCameraManager;
class VideoSettings;
class VideoSettingsBridge;
class VideoStreamModel;
class VideoManagerIntegrationTest;

/// Owns the video stream collection extracted from VideoManager. Responsible
/// for stream creation/teardown, UVC activation, settings→stream propagation,
/// aggregate lifecycle state, and per-role lookup.
///
/// Consumers (VideoManager as the QML facade; tests) interact via this
/// object's public verbs and observe transitions through the emitted signals.
/// `_onStreamInfoUpdated` geometry caching stays in the facade because its
/// shape is dictated by VideoManager's Q_PROPERTY layout; the orchestrator
/// emits a single coarse `streamInfoUpdated()` signal for that purpose.
class VideoStreamOrchestrator : public QObject
{
    Q_OBJECT

    friend class VideoManagerIntegrationTest;

public:
    using StreamCreationHook = std::function<void()>;

    /// `settings` must outlive this object. `bindToSettings()` wires the
    /// Fact signal subscriptions after construction.
    explicit VideoStreamOrchestrator(VideoSettings* settings, QObject* parent = nullptr);
    ~VideoStreamOrchestrator() override;

    VideoStreamModel* streamModel() const { return _streamModel; }

    void createStreams();

    /// Test hook — when set, `createStreams()` calls it instead of building
    /// the production stream list. Production code never sets this.
    void setCreateStreamsForTest(StreamCreationHook hook) { _createStreamsForTest = std::move(hook); }

    /// Override the factory used by `_reconcileDynamicStreams` when spawning
    /// new `Role::Dynamic` VideoStreams. Defaults to
    /// `VideoReceiverFactory::gstOrQtMultimedia` in production. Tests inject a
    /// FakeVideoReceiver factory here to keep reconcile hermetic.
    void setDynamicReceiverFactoryForTest(VideoStream::ReceiverFactory factory) {
        _dynamicFactory = std::move(factory);
    }

    void cleanup();

    /// Wires orchestrator reactions to VideoSettings Fact changes. Call once
    /// after construction; disconnection happens in `cleanup()` / dtor.
    void bindToSettings();

    /// Sets the camera manager backing per-stream info. Pass nullptr to
    /// clear (no active vehicle or vehicle without cameras).
    void setCameraManager(QGCCameraManager* cameraManager);

    void startVideo();
    void stopVideo();

    /// Forwarder used by facades that want to re-trigger source resolution
    /// outside of Fact signal deliveries (e.g. after `autoStreamConfigured`
    /// changes). Equivalent to an internal source-change notification.
    void refreshFromSettings();

    [[nodiscard]] bool streaming() const;
    [[nodiscard]] bool decoding() const;
    [[nodiscard]] bool recording() const;

    [[nodiscard]] bool hasVideo() const;
    [[nodiscard]] QSize videoSize() const { return _videoSize; }

    [[nodiscard]] VideoStream* streamByRole(VideoStream::Role role) const;

    [[nodiscard]] VideoStream* primaryStream() const { return streamByRole(VideoStream::Role::Primary); }

    [[nodiscard]] VideoStream* thermalStream() const { return streamByRole(VideoStream::Role::Thermal); }

    [[nodiscard]] VideoStream* uvcStream() const { return streamByRole(VideoStream::Role::UVC); }

    /// All streams currently created for `Role::Dynamic` — vehicle-reported
    /// streams beyond the primary/thermal slots. Order matches insertion.
    [[nodiscard]] QList<VideoStream*> dynamicStreams() const;

    template <typename Func>
    void forEachStream(Func&& fn) const
    {
        for (const auto& stream : std::as_const(_streams))
            fn(stream);
    }

    template <typename Func>
    void forEachNetworkStream(Func&& fn) const
    {
        for (const auto& stream : std::as_const(_streams)) {
            if (stream->role() == VideoStream::Role::UVC)
                continue;
            fn(stream);
        }
    }

    template <typename Func>
    void forEachRecordableStream(Func&& fn) const
    {
        for (const auto& stream : std::as_const(_streams)) {
            if (!stream->started())
                continue;
            if (!stream->recorder())
                continue;
            fn(stream);
        }
    }

signals:
    /// Aggregate transitions — emitted only when the computed aggregate
    /// actually flips, not on every per-stream signal. Used by the facade
    /// to coalesce camera-state pushes; QML reads per-stream state via
    /// `streamModel`, not these signals.
    void streamingChanged();
    void decodingChanged();
    void recordingChanged(bool recording);

    /// Source/configuration transitions fired from `_onSourceChanged`.
    void hasVideoChanged();
    void isStreamSourceChanged();

    /// The primary stream's bridge pointer changed (receiver recreated).
    /// Facade queries `primaryStream()->bridge()` to extract the new sink.
    void primaryBridgeChanged();

    /// Per-stream recording error (already formatted user-facing message).
    void recordingError(const QString& errorMsg);

    /// Fired once after `createStreams()` finishes successfully.
    void streamsCreated();

private slots:
    void _onSourceChanged();
    void _recomputeAggregate();

private:
    void _wireStreamSignals(VideoStream* stream);
    void _activateUvcStream();
    void _deactivateUvcStream();
    [[nodiscard]] bool _isUvcSource() const;

    /// Reconcile `Role::Dynamic` streams against the active camera's MAVLink
    /// stream list. Adds streams for newly reported stream IDs, destroys
    /// streams whose IDs have vanished. Primary/Thermal slots are not touched.
    /// Must be called from `_onSourceChanged` after primary/thermal binding.
    /// Returns true if the dynamic set changed.
    bool _reconcileDynamicStreams();

    /// Test seam: reconcile against an explicitly provided set of
    /// `{streamID → QGCVideoStreamInfo*}`. Production uses the arg-less
    /// overload which pulls from `_cameraManager`. Exposed to
    /// `VideoManagerIntegrationTest` via friend access.
    bool _reconcileDynamicStreams(const QHash<quint8, class QGCVideoStreamInfo*>& expected);

    VideoSettings* _settings = nullptr;
    VideoSettingsBridge* _settingsBridge = nullptr;
    QPointer<QGCCameraManager> _cameraManager;
    QMetaObject::Connection _cameraManagerConn;

    QList<VideoStream*> _streams;
    VideoStreamModel* _streamModel = nullptr;
    StreamCreationHook _createStreamsForTest;

    /// Factory for Role::Dynamic receivers. Defaults to
    /// `VideoReceiverFactory::gstOrQtMultimedia`; tests override via
    /// `setDynamicReceiverFactoryForTest`.
    VideoStream::ReceiverFactory _dynamicFactory;

    /// Previous aggregate values — used to detect real transitions before
    /// emitting NOTIFY signals in `_recomputeAggregate()`.
    struct AggregateState
    {
        bool streaming = false;
        bool decoding = false;
        bool recording = false;
    };

    AggregateState _agg;

    QSize _videoSize;
};
