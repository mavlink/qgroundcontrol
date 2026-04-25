#pragma once

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <optional>

#include "VideoSourceResolver.h"
#include "VideoStream.h"

class QGCCameraManager;
class VideoSettings;

Q_DECLARE_LOGGING_CATEGORY(VideoSourceControllerLog)

/// Owns source selection for video streams: camera metadata binding,
/// settings/MAVLink source resolution, settings writeback, and dynamic stream
/// expected-set construction. VideoStreamOrchestrator owns stream lifetime;
/// this object decides what source each stream should use.
class VideoSourceController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(VideoSourceController)

public:
    explicit VideoSourceController(VideoSettings* settings, QObject* parent = nullptr);
    ~VideoSourceController() override;

    void setCameraManager(QGCCameraManager* cameraManager);

    using DynamicStreamInfoMap = QHash<quint8, VideoSourceResolver::StreamInfo>;

    [[nodiscard]] std::optional<VideoSourceResolver::StreamInfo> cameraMetadataForRole(VideoStream::Role role) const;
    [[nodiscard]] DynamicStreamInfoMap expectedDynamicStreams() const;

    void bindCameraMetadata(VideoStream* stream) const;
    [[nodiscard]] VideoSourceResolver::EffectiveSource resolveStreamSource(const VideoStream* stream) const;
    [[nodiscard]] bool applyResolvedSource(VideoStream* stream) const;

    [[nodiscard]] bool isUvcSource() const;
    [[nodiscard]] bool autoStreamConfigured(const VideoStream* primaryStream) const;
    [[nodiscard]] bool hasVideo(const VideoStream* primaryStream) const;
    [[nodiscard]] bool applyLowLatency(VideoStream* stream) const;

signals:
    void sourceChanged();

private:
    void _writeBackResolvedSource(const VideoSourceResolver::EffectiveSource& resolution) const;

    VideoSettings* _settings = nullptr;
    QPointer<QGCCameraManager> _cameraManager;
    QMetaObject::Connection _cameraManagerConn;
};
