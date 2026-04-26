#pragma once

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSize>

#include <functional>
#include <optional>

#include "VideoStream.h"  // Role enum required for templated iterators
#include "VideoSourceResolver.h"

Q_DECLARE_LOGGING_CATEGORY(VideoStreamOrchestratorLog)

class QGCCameraManager;
class QMediaDevices;
class VideoSourceController;
class VideoSettings;
class VideoSettingsBridge;
class VideoStreamAggregateMonitor;
class VideoStreamModel;
class VideoUvcController;

/// Owns the video stream collection: stream creation/teardown, UVC
/// activation, settings-to-stream propagation, aggregate state, and per-role
/// lookup.
class VideoStreamOrchestrator : public QObject
{
    Q_OBJECT

public:
    using StreamCreationHook = std::function<void()>;
    using LocalCameraAvailableFn = std::function<bool()>;

    /// `settings` must outlive this object. `bindToSettings()` wires the
    /// Fact signal subscriptions after construction.
    explicit VideoStreamOrchestrator(VideoSettings* settings, QObject* parent = nullptr);
    ~VideoStreamOrchestrator() override;

    VideoStreamModel* streamModel() const { return _streamModel; }

    void createStreams();

#ifdef QGC_UNITTEST_BUILD
    void setCreateStreamsForTest(StreamCreationHook hook) { _createStreamsForTest = std::move(hook); }
    void setLocalCameraAvailableForTest(LocalCameraAvailableFn fn);
    void registerStreamForTest(VideoStream* stream);
    bool reconcileDynamicStreamsForTest(const QHash<quint8, VideoSourceResolver::StreamInfo>& expected);

    void setDynamicReceiverFactoryForTest(VideoStream::ReceiverFactory factory) {
        _dynamicFactory = std::move(factory);
    }
#endif

    void cleanup();

    /// Wires orchestrator reactions to VideoSettings Fact changes. Call once
    /// after construction; disconnection happens in `cleanup()` / dtor.
    void bindToSettings();

    /// Sets the camera manager backing per-stream info. Pass nullptr to
    /// clear (no active vehicle or vehicle without cameras).
    void setCameraManager(QGCCameraManager* cameraManager);

    void startVideo();
    void stopVideo();

    /// Re-trigger source resolution outside of Fact signal deliveries.
    void refreshFromSettings();

    [[nodiscard]] bool decoding() const;
    [[nodiscard]] bool recording() const;

    [[nodiscard]] bool hasVideo() const;
    [[nodiscard]] bool autoStreamConfigured() const;
    [[nodiscard]] static bool isAutoStreamConfigured(const std::optional<VideoSourceResolver::StreamInfo>& metadata);
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
    /// Aggregate transitions — emitted only when the computed aggregate flips.
    void decodingChanged();
    void recordingChanged(bool recording);

    /// Source/configuration transitions fired from `_onSourceChanged`.
    void hasVideoChanged();
    void isStreamSourceChanged();

    /// The primary stream's frame-delivery pointer changed.
    void primaryFrameDeliveryChanged();

    /// Per-stream recording error (already formatted user-facing message).
    void recordingError(const QString& errorMsg);

    /// Fired once after `createStreams()` finishes successfully.
    void streamsCreated();

private slots:
    void _onSourceChanged();

private:
    void _wireStreamSignals(VideoStream* stream);
    VideoStream* _createStream(VideoStream::Role role, VideoStream::ReceiverFactory factory);
    VideoStream* _createStream(VideoStream::Role role, const QString& name, VideoStream::ReceiverFactory factory);
    void _addStream(VideoStream* stream);
    void _removeStream(VideoStream* stream);
    void _activateUvcStream();
    void _deactivateUvcStream();
    [[nodiscard]] VideoStream* _ensureUvcStream();
    [[nodiscard]] bool _isUvcSource() const;
    [[nodiscard]] bool _applyResolvedSource(VideoStream* stream);

    /// Reconcile `Role::Dynamic` streams against the active camera's MAVLink
    /// stream list. Adds streams for newly reported stream IDs, destroys
    /// streams whose IDs have vanished. Primary/Thermal slots are not touched.
    /// Must be called from `_onSourceChanged` after primary/thermal binding.
    /// Returns true if the dynamic set changed.
    bool _reconcileDynamicStreams();

    /// Reconcile against an explicitly provided set of `{streamID -> StreamInfo}` snapshots.
    bool _reconcileDynamicStreams(const QHash<quint8, VideoSourceResolver::StreamInfo>& expected);

    VideoSettingsBridge* _settingsBridge = nullptr;
    VideoSourceController* _sourceController = nullptr;
    QMediaDevices* _mediaDevices = nullptr;
    VideoStreamModel* _streamModel = nullptr;
    VideoStreamAggregateMonitor* _aggregateMonitor = nullptr;
    VideoUvcController* _uvcController = nullptr;

    QList<VideoStream*> _streams;
    StreamCreationHook _createStreamsForTest;

    VideoStream::ReceiverFactory _dynamicFactory;

    QSize _videoSize;
};
