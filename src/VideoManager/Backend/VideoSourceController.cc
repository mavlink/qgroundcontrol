#include "VideoSourceController.h"

#include "MavlinkCameraControlInterface.h"
#include "QGCCameraManager.h"
#include "QGCLoggingCategory.h"
#include "QGCVideoStreamInfo.h"
#include "QmlObjectListModel.h"
#include "VideoSettings.h"

#include <QtCore/QSignalBlocker>

QGC_LOGGING_CATEGORY(VideoSourceControllerLog, "Video.VideoSourceController")

VideoSourceController::VideoSourceController(VideoSettings* settings, QObject* parent)
    : QObject(parent)
    , _settings(settings)
{
}

VideoSourceController::~VideoSourceController()
{
    if (_cameraManager)
        disconnect(_cameraManagerConn);
}

void VideoSourceController::setCameraManager(QGCCameraManager* cameraManager)
{
    if (_cameraManager == cameraManager)
        return;

    if (_cameraManager) {
        disconnect(_cameraManagerConn);
        _cameraManagerConn = {};
    }

    _cameraManager = cameraManager;

    if (_cameraManager) {
        _cameraManagerConn = connect(_cameraManager, &QGCCameraManager::streamChanged,
                                     this, &VideoSourceController::sourceChanged);
    }
}

std::optional<VideoSourceResolver::StreamInfo> VideoSourceController::cameraMetadataForRole(VideoStream::Role role) const
{
    if (!_cameraManager)
        return std::nullopt;

    QGCVideoStreamInfo* info = nullptr;

    switch (role) {
        case VideoStream::Role::Primary:
            info = _cameraManager->currentStreamInstance();
            break;
        case VideoStream::Role::Thermal:
            info = _cameraManager->thermalStreamInstance();
            break;
        case VideoStream::Role::Dynamic:
        case VideoStream::Role::UVC:
            return std::nullopt;
    }

    return VideoSourceResolver::streamInfoFrom(info);
}

VideoSourceController::DynamicStreamInfoMap VideoSourceController::expectedDynamicStreams() const
{
    DynamicStreamInfoMap expected;
    if (!_cameraManager)
        return expected;

    auto* currentCamera = _cameraManager->currentCameraInstance();
    auto* currentInfo = _cameraManager->currentStreamInstance();
    auto* thermalInfo = _cameraManager->thermalStreamInstance();
    if (!currentCamera || !currentCamera->streams())
        return expected;

    auto* list = currentCamera->streams();
    for (int i = 0; i < list->count(); ++i) {
        auto* info = qobject_cast<QGCVideoStreamInfo*>(list->get(i));
        if (!info)
            continue;
        if (info == currentInfo || info == thermalInfo)
            continue;
        std::optional<VideoSourceResolver::StreamInfo> snapshot = VideoSourceResolver::streamInfoFrom(info);
        if (!snapshot || !snapshot->streamID)
            continue;
        expected.insert(*snapshot->streamID, *snapshot);
    }
    return expected;
}

void VideoSourceController::bindCameraMetadata(VideoStream* stream) const
{
    if (!stream)
        return;

    switch (stream->role()) {
        case VideoStream::Role::Primary:
        case VideoStream::Role::Thermal:
            if (const auto metadata = cameraMetadataForRole(stream->role()))
                stream->setSourceMetadata(*metadata);
            else
                stream->clearSourceMetadata();
            break;
        case VideoStream::Role::Dynamic:
        case VideoStream::Role::UVC:
            break;
    }
}

VideoSourceResolver::EffectiveSource VideoSourceController::resolveStreamSource(const VideoStream* stream) const
{
    if (!stream)
        return {};

    return VideoSourceResolver::resolveEffectiveSource(stream->isThermal(), _settings, stream->sourceMetadata());
}

bool VideoSourceController::applyResolvedSource(VideoStream* stream) const
{
    if (!stream)
        return false;

    bool changed = applyLowLatency(stream);
    const VideoSourceResolver::EffectiveSource resolution = resolveStreamSource(stream);
    _writeBackResolvedSource(resolution);
    if (resolution.hasSource)
        changed |= stream->setSourceDescriptor(resolution.source);
    return changed;
}

bool VideoSourceController::isUvcSource() const
{
    return _settings && _settings->currentSourceType() == VideoSettings::SourceType::UVC;
}

bool VideoSourceController::autoStreamConfigured(const VideoStream* primaryStream) const
{
    const std::optional<VideoSourceResolver::StreamInfo> metadata =
        primaryStream ? primaryStream->sourceMetadata() : std::nullopt;
    return metadata && !metadata->thermal && !metadata->uri.isEmpty();
}

bool VideoSourceController::hasVideo(const VideoStream* primaryStream) const
{
    return _settings && _settings->streamEnabled()->rawValue().toBool()
           && (_settings->streamConfigured() || autoStreamConfigured(primaryStream));
}

bool VideoSourceController::applyLowLatency(VideoStream* stream) const
{
    if (!_settings || !stream)
        return false;

    return stream->setLowLatency(_settings->lowLatencyMode()->rawValue().toBool());
}

void VideoSourceController::_writeBackResolvedSource(const VideoSourceResolver::EffectiveSource& resolution) const
{
    if (!_settings)
        return;

    if (!resolution.writeBackSourceName.isEmpty()) {
        const QSignalBlocker sourceBlocker(_settings->videoSource());
        _settings->videoSource()->setRawValue(resolution.writeBackSourceName);
    }
    if (!resolution.writeBackRtspUrl.isEmpty()) {
        const QSignalBlocker rtspBlocker(_settings->rtspUrl());
        _settings->rtspUrl()->setRawValue(resolution.writeBackRtspUrl);
    }
}
