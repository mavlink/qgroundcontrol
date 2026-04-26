#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

class VideoSettings;

Q_DECLARE_LOGGING_CATEGORY(VideoSettingsBridgeLog)

/// Collapses VideoSettings Fact-value changes into two coarse transitions:
/// `sourceChanged` (any change that affects URI resolution) and
/// `lowLatencyChanged` (restart-only change). Keeps the per-Fact subscription
/// list in one place so consumers don't need to know which Facts participate
/// in a "source" change.
class VideoSettingsBridge : public QObject
{
    Q_OBJECT

public:
    /// `settings` must outlive this object.
    explicit VideoSettingsBridge(VideoSettings* settings, QObject* parent = nullptr);
    ~VideoSettingsBridge() override;

    /// Connects to the underlying Fact change signals. Idempotent.
    void subscribe();

signals:
    /// Fired for any of: source, udpUrl, rtspUrl, tcpUrl, or streamConfigured.
    /// Consumers re-resolve effective URI + source type.
    void sourceChanged();

    /// Low-latency flag flipped. Consumers typically restart streams.
    void lowLatencyChanged();

private:
    VideoSettings* _settings = nullptr;
    bool _subscribed = false;
};
