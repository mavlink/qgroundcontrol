#include "VideoSettings.h"

#include <QtCore/QSettings>
#include <QtCore/QVariantList>

#include "QGCLoggingCategory.h"
#include "UVCDevices.h"
#include "VideoBackend.h"

QGC_LOGGING_CATEGORY(VideoSettingsLog, "Settings.VideoSettings")

namespace {

bool hasStreaming()
{
    return VideoBackend::instance().hasStreaming();
}

}  // namespace

DECLARE_SETTINGGROUP(Video, "Video")
{
    // Setup enum values for videoSource settings into meta data
    QVariantList videoSourceList;
    videoSourceList.append(videoSourceRTSP);
    videoSourceList.append(videoSourceUDPH264);
    videoSourceList.append(videoSourceUDPH265);
    videoSourceList.append(videoSourceTCP);
    videoSourceList.append(videoSourceMPEGTS);
    videoSourceList.append(videoSource3DRSolo);
    videoSourceList.append(videoSourceParrotDiscovery);
    videoSourceList.append(videoSourceYuneecMantisG);

#ifdef QGC_HERELINK_AIRUNIT_VIDEO
    videoSourceList.append(videoSourceHerelinkAirUnit);
#else
    videoSourceList.append(videoSourceHerelinkHotspot);
#endif
    QStringList uvcDevices = UVCDevices::deviceNames();
    for (const QString& device : uvcDevices) {
        videoSourceList.append(device);
    }
    if (videoSourceList.count() == 0) {
        _noVideo = true;
        videoSourceList.append(videoSourceNoVideo);
        setUserVisible(false);
    } else {
        videoSourceList.insert(0, videoDisabled);
    }

    // make translated strings
    QStringList videoSourceCookedList;
    for (const QVariant& videoSource: videoSourceList) {
        videoSourceCookedList.append( VideoSettings::tr(videoSource.toString().toStdString().c_str()) );
    }

    _nameToMetaDataMap[videoSourceName]->setEnumInfo(videoSourceCookedList, videoSourceList);

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

    // Drop a stored streamEnabled=false: it has no UI, so it is a stale "disable when disarmed" state.
    {
        QSettings settings;
        settings.beginGroup(settingsGroup);
        if (!settings.value(streamEnabledName, true).toBool()) {
            qCDebug(VideoSettingsLog) << "Clearing a stored streamEnabled=false";
            settings.remove(streamEnabledName);
        }
        settings.endGroup();
    }

    // Set default value for videoSource
    _setDefaults();
}

void VideoSettings::_setDefaults()
{
    if (_noVideo) {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoSourceNoVideo);
    } else {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoDisabled);
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
DECLARE_SETTINGSFACT(VideoSettings, streamEnabled)
DECLARE_SETTINGSFACT(VideoSettings, disableWhenDisarmed)
DECLARE_SETTINGSFACT(VideoSettings, udpUrl)
DECLARE_SETTINGSFACT(VideoSettings, rtspUrl)
DECLARE_SETTINGSFACT(VideoSettings, tcpUrl)
DECLARE_SETTINGSFACT(VideoSettings, uvcCameraFormat)
DECLARE_SETTINGSFACT(VideoSettings, uvcCameraRotation)
DECLARE_SETTINGSFACT(VideoSettings, uvcCameraMirror)
DECLARE_SETTINGSFACT(VideoSettings, uvcRecordingCodec)

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoSource)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
        //-- Check for sources no longer available
        if(!_videoSourceFact->enumValues().contains(_videoSourceFact->rawValue().toString())) {
            if (_noVideo) {
                _videoSourceFact->setRawValue(videoSourceNoVideo);
            } else {
                _videoSourceFact->setRawValue(videoDisabled);
            }
        }
    }
    return _videoSourceFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, forceVideoDecoder)
{
    if (!_forceVideoDecoderFact) {
        _forceVideoDecoderFact = _createSettingsFact(forceVideoDecoderName);

        _forceVideoDecoderFact->setUserVisible(hasStreaming());
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
        _rtpJitterLatencyMsFact->setUserVisible(hasStreaming());
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

        _forceCpuVideoPathFact->setUserVisible(VideoBackend::instance().hasGpuVideoPath());
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
        _videoConversionElementFact->setUserVisible(hasStreaming());
    }
    return _videoConversionElementFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, disablePixelAspectRatio)
{
    if (!_disablePixelAspectRatioFact) {
        _disablePixelAspectRatioFact = _createSettingsFact(disablePixelAspectRatioName);
        _disablePixelAspectRatioFact->setUserVisible(hasStreaming());
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
    if (!hasStreaming()) {
        return;
    }
    static const QList<VideoDecoder::Options> removeForceVideoDecodeList{
#if defined(Q_OS_ANDROID)
        VideoDecoder::ForceVideoDecoderDirectX3D, VideoDecoder::ForceVideoDecoderVideoToolbox,
        VideoDecoder::ForceVideoDecoderVAAPI,     VideoDecoder::ForceVideoDecoderNVIDIA,
        VideoDecoder::ForceVideoDecoderIntel,
#elif defined(Q_OS_LINUX)
        VideoDecoder::ForceVideoDecoderDirectX3D,
        VideoDecoder::ForceVideoDecoderVideoToolbox,
#elif defined(Q_OS_WIN)
        VideoDecoder::ForceVideoDecoderVideoToolbox,
        VideoDecoder::ForceVideoDecoderVulkan,
#elif defined(Q_OS_MACOS)
        VideoDecoder::ForceVideoDecoderDirectX3D,
        VideoDecoder::ForceVideoDecoderVAAPI,
#elif defined(Q_OS_IOS)
        VideoDecoder::ForceVideoDecoderDirectX3D,
        VideoDecoder::ForceVideoDecoderVAAPI,
        VideoDecoder::ForceVideoDecoderNVIDIA,
        VideoDecoder::ForceVideoDecoderIntel,
#endif
    };

    for (const auto& value : removeForceVideoDecodeList) {
        _nameToMetaDataMap[forceVideoDecoderName]->removeEnumInfo(value);
    }
}

void VideoSettings::pruneUnavailableDecoders()
{
    if (!hasStreaming()) {
        return;
    }
    static const QList<VideoDecoder::Options> hardwareFamilies{
        VideoDecoder::ForceVideoDecoderNVIDIA,    VideoDecoder::ForceVideoDecoderVAAPI,
        VideoDecoder::ForceVideoDecoderDirectX3D, VideoDecoder::ForceVideoDecoderVideoToolbox,
        VideoDecoder::ForceVideoDecoderIntel,     VideoDecoder::ForceVideoDecoderVulkan,
    };

    const QList<VideoDecoder::Options> available = VideoBackend::instance().availableDecoderFamilies();
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
        fact->setRawValue(VideoDecoder::ForceVideoDecoderDefault);
    }
}
