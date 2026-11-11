#include "VideoSettings.h"

#include <QtCore/QSettings>
#include <QtCore/QVariantList>
#include <QtMultimedia/QMediaDevices>

#include "UVCDevices.h"
#include "VideoBackend.h"

namespace {

bool hasStreaming()
{
    return VideoBackend::instance().capabilities().streaming;
}

bool hasPipelineSettings()
{
    return VideoBackend::instance().capabilities().pipelineSettings;
}

}  // namespace

DECLARE_SETTINGGROUP(Video, "Video")
{
    _updateVideoSources();
    (void) connect(new QMediaDevices(this), &QMediaDevices::videoInputsChanged, this,
                   &VideoSettings::_updateVideoSources);

    _setForceVideoDecodeList();

    // Migrate legacy gpuZeroCopyEnabled (pre-rename) into the new force-CPU semantics.
    {
        QSettings settings;
        settings.beginGroup(settingsGroup);
        const bool hasLegacy = settings.contains(QStringLiteral("gpuZeroCopyEnabled"));
        const bool hasNew    = settings.contains(forceCpuVideoPathName);
        if (hasLegacy) {
            if (!hasNew) {
                const bool gpuZeroCopy = settings.value(QStringLiteral("gpuZeroCopyEnabled")).toBool();
                forceCpuVideoPath()->setRawValue(!gpuZeroCopy);
            }
            settings.remove(QStringLiteral("gpuZeroCopyEnabled"));
        }
        settings.endGroup();
    }

    // Set default value for videoSource
    _setDefaults();
}

void VideoSettings::_setDefaults()
{
    _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoDisabled);
}

void VideoSettings::_updateVideoSources()
{
    static constexpr const char* fixedSources[] = {
        videoDisabled,
        videoSourceRTSP,
        videoSourceUDPH264,
        videoSourceUDPH265,
        videoSourceTCP,
        videoSourceMPEGTS,
        videoSource3DRSolo,
        videoSourceParrotDiscovery,
        videoSourceYuneecMantisG,
#ifdef QGC_HERELINK_AIRUNIT_VIDEO
        videoSourceHerelinkAirUnit,
#else
        videoSourceHerelinkHotspot,
#endif
    };

    const QStringList cameras = _cameraNames ? _cameraNames() : UVCDevices::deviceNames();

    if (FactMetaData* const metaData = _nameToMetaDataMap.value(videoSourceName)) {
        QVariantList sources;
        QStringList labels;
        for (const char* const source : fixedSources) {
            sources.append(QString::fromLatin1(source));
            labels.append(tr(source));
        }
        // A settings override file or a custom build that replaced the sources, as the Fact was created, keeps its
        // list.
        const QVariantList listed = metaData->enumValues();
        if (listed.isEmpty() || (listed.mid(0, sources.size()) == sources)) {
            _offerCameras(metaData, _videoSourceFact, sources, labels, cameras);
        }
    }

    if (FactMetaData* const metaData = _nameToMetaDataMap.value(uvcTileCameraName)) {
        _offerCameras(metaData, _uvcTileCameraFact, {QString()}, {tr("None")}, cameras);
    }
}

void VideoSettings::_offerCameras(FactMetaData* metaData, Fact* fact, QVariantList values, QStringList labels,
                                  const QStringList& cameras)
{
    for (const QString& camera : cameras) {
        values.append(camera);
        labels.append(camera);
    }
    const QString selected = fact ? fact->rawValue().toString() : QString();
    if (!selected.isEmpty() && !values.contains(selected)) {
        values.append(selected);
        labels.append(tr("%1 (not connected)").arg(selected));
    }

    if ((values == metaData->enumValues()) && (labels == metaData->enumStrings())) {
        return;
    }
    if (fact) {
        // Tells the settings page's combo box as well.
        fact->setEnumInfo(labels, values);
    } else {
        metaData->setEnumInfo(labels, values);
    }
}

DECLARE_SETTINGSFACT(VideoSettings, aspectRatio)
DECLARE_SETTINGSFACT(VideoSettings, videoFit)
DECLARE_SETTINGSFACT(VideoSettings, gridLines)
DECLARE_SETTINGSFACT(VideoSettings, showTelemetryOverlay)
DECLARE_SETTINGSFACT(VideoSettings, showRecControl)
DECLARE_SETTINGSFACT(VideoSettings, recordingFormat)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoSize)
DECLARE_SETTINGSFACT(VideoSettings, enableStorageLimit)
DECLARE_SETTINGSFACT(VideoSettings, disableWhenDisarmed)
DECLARE_SETTINGSFACT(VideoSettings, udpUrl)
DECLARE_SETTINGSFACT(VideoSettings, rtspUrl)
DECLARE_SETTINGSFACT(VideoSettings, tcpUrl)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoTiles)
DECLARE_SETTINGSFACT(VideoSettings, uvcCameraFormat)
DECLARE_SETTINGSFACT(VideoSettings, uvcCameraRotation)
DECLARE_SETTINGSFACT(VideoSettings, uvcCameraMirror)
DECLARE_SETTINGSFACT(VideoSettings, uvcWhiteBalance)
DECLARE_SETTINGSFACT(VideoSettings, uvcZoom)
DECLARE_SETTINGSFACT(VideoSettings, uvcExposureCompensation)
DECLARE_SETTINGSFACT(VideoSettings, uvcFocusMode)
DECLARE_SETTINGSFACT(VideoSettings, uvcRecordingCodec)
DECLARE_SETTINGSFACT(VideoSettings, uvcRecordingQuality)

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoSource)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
        // The stored source may be a camera that isn't attached, which is listed only while it is selected. Queued:
        // the combo box that sets the source deletes its items when its model changes, which it mustn't do while one
        // of them reports its click.
        _updateVideoSources();
        (void) connect(_videoSourceFact, &Fact::rawValueChanged, this, &VideoSettings::_updateVideoSources,
                       Qt::QueuedConnection);
    }
    return _videoSourceFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, uvcTileCamera)
{
    if (!_uvcTileCameraFact) {
        _uvcTileCameraFact = _createSettingsFact(uvcTileCameraName);
        // As for videoSource: the stored camera may not be attached, and the list changes only after the click.
        _updateVideoSources();
        (void) connect(_uvcTileCameraFact, &Fact::rawValueChanged, this, &VideoSettings::_updateVideoSources,
                       Qt::QueuedConnection);
    }
    return _uvcTileCameraFact;
}

// Only a GStreamer build offers a choice: without GStreamer, Qt Multimedia plays the streams.
DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoBackend)
{
    if (!_videoBackendFact) {
        _videoBackendFact = _createSettingsFact(videoBackendName);
        _videoBackendFact->setUserVisible(hasPipelineSettings());
    }
    return _videoBackendFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, forceVideoDecoder)
{
    if (!_forceVideoDecoderFact) {
        _forceVideoDecoderFact = _createSettingsFact(forceVideoDecoderName);

        _forceVideoDecoderFact->setUserVisible(hasPipelineSettings());
    }
    return _forceVideoDecoderFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, lowLatencyMode)
{
    if (!_lowLatencyModeFact) {
        _lowLatencyModeFact = _createSettingsFact(lowLatencyModeName);

        _lowLatencyModeFact->setUserVisible(hasStreaming());
    }
    return _lowLatencyModeFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtpJitterLatencyMs)
{
    if (!_rtpJitterLatencyMsFact) {
        _rtpJitterLatencyMsFact = _createSettingsFact(rtpJitterLatencyMsName);
        _rtpJitterLatencyMsFact->setUserVisible(hasPipelineSettings());
    }
    return _rtpJitterLatencyMsFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspAutoReconnect)
{
    if (!_rtspAutoReconnectFact) {
        _rtspAutoReconnectFact = _createSettingsFact(rtspAutoReconnectName);
        _rtspAutoReconnectFact->setUserVisible(hasStreaming());
    }
    return _rtspAutoReconnectFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, forceCpuVideoPath)
{
    if (!_forceCpuVideoPathFact) {
        _forceCpuVideoPathFact = _createSettingsFact(forceCpuVideoPathName);

        _forceCpuVideoPathFact->setUserVisible(VideoBackend::instance().capabilities().gpuVideoPath);
    }
    return _forceCpuVideoPathFact;
}

// videoConversionElement / disablePixelAspectRatio reach the receivers' display sinks as construct-only properties
// (VideoStreamResolver::displayConfig()); VideoManager restarts the streams, which rebuilds the sinks, when they
// change.
DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoConversionElement)
{
    if (!_videoConversionElementFact) {
        _videoConversionElementFact = _createSettingsFact(videoConversionElementName);
        _videoConversionElementFact->setUserVisible(hasPipelineSettings());
    }
    return _videoConversionElementFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, disablePixelAspectRatio)
{
    if (!_disablePixelAspectRatioFact) {
        _disablePixelAspectRatioFact = _createSettingsFact(disablePixelAspectRatioName);
        _disablePixelAspectRatioFact->setUserVisible(hasPipelineSettings());
    }
    return _disablePixelAspectRatioFact;
}


DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspTimeout)
{
    if (!_rtspTimeoutFact) {
        _rtspTimeoutFact = _createSettingsFact(rtspTimeoutName);

        _rtspTimeoutFact->setUserVisible(hasStreaming());
    }
    return _rtspTimeoutFact;
}

void VideoSettings::_setForceVideoDecodeList()
{
    if (!hasPipelineSettings()) {
        return;
    }
    static const QList<VideoDecoderOption> removeForceVideoDecodeList{
#if defined(Q_OS_ANDROID)
        VideoDecoderOption::DirectX3D, VideoDecoderOption::VideoToolbox, VideoDecoderOption::VAAPI,
        VideoDecoderOption::NVIDIA,    VideoDecoderOption::Intel,
#elif defined(Q_OS_LINUX)
        VideoDecoderOption::DirectX3D,
        VideoDecoderOption::VideoToolbox,
#elif defined(Q_OS_WIN)
        VideoDecoderOption::VideoToolbox,
        VideoDecoderOption::Vulkan,
#elif defined(Q_OS_MACOS)
        VideoDecoderOption::DirectX3D,
        VideoDecoderOption::VAAPI,
#elif defined(Q_OS_IOS)
        VideoDecoderOption::DirectX3D,
        VideoDecoderOption::VAAPI,
        VideoDecoderOption::NVIDIA,
        VideoDecoderOption::Intel,
#endif
    };

    for (const VideoDecoderOption option : removeForceVideoDecodeList) {
        _nameToMetaDataMap[forceVideoDecoderName]->removeEnumInfo(static_cast<int>(option));
    }
}

void VideoSettings::pruneUnavailableDecoders()
{
    if (!hasPipelineSettings()) {
        return;
    }
    static const QList<VideoDecoderOption> hardwareFamilies{
        VideoDecoderOption::NVIDIA,       VideoDecoderOption::VAAPI, VideoDecoderOption::DirectX3D,
        VideoDecoderOption::VideoToolbox, VideoDecoderOption::Intel, VideoDecoderOption::Vulkan,
    };

    const QList<VideoDecoderOption> available = VideoBackend::instance().availableDecoderFamilies();
    const auto metaIt = _nameToMetaDataMap.constFind(forceVideoDecoderName);
    if (metaIt == _nameToMetaDataMap.constEnd() || !metaIt.value()) {
        return;
    }
    FactMetaData* const metaData = metaIt.value();
    bool pruned = false;
    for (const auto family : hardwareFamilies) {
        // removeEnumInfo() qWarns on an absent value, so skip families not in the enum; values are
        // stored as QVariant(int), so match that representation.
        const QVariant familyValue = static_cast<int>(family);
        if (!available.contains(family) && metaData->enumValues().contains(familyValue)) {
            metaData->removeEnumInfo(familyValue);
            pruned = true;
        }
    }

    Fact* const fact = forceVideoDecoder();
    if (pruned) {
        // Backend init is async — refresh any live FactComboBox bound to this fact.
        emit fact->enumsChanged();
    }
    if (!metaData->enumValues().contains(fact->rawValue())) {
        fact->setRawValue(static_cast<int>(VideoDecoderOption::Default));
    }
}
