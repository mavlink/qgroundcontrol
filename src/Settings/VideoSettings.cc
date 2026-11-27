#include "VideoSettings.h"

#include <QtCore/QVariantList>
#include <QtMultimedia/QMediaDevices>

#include "QtVideoRecorder.h"
#include "UVCDevices.h"
#include "VideoBackend.h"

namespace {

bool hasPipelineSettings()
{
    return VideoBackend::instance().capabilities().pipelineSettings;
}

/// Whether a receiver of this build takes the decoder settings and the RTP jitter latency: GStreamer's or the
/// experimental FFmpeg one.
bool hasDecoderSettings()
{
#ifdef QGC_FFMPEG_VIDEO
    return true;
#else
    return hasPipelineSettings();
#endif
}

}  // namespace

DECLARE_SETTINGGROUP(Video, "Video")
{
    _updateVideoSources();
    (void) connect(new QMediaDevices(this), &QMediaDevices::videoInputsChanged, this,
                   &VideoSettings::_updateVideoSources);

    _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoDisabled);
#ifdef QGC_FFMPEG_VIDEO
    _nameToMetaDataMap[videoBackendName]->addEnumInfo(VideoSettings::tr("FFmpeg (experimental)"), VideoBackendFFmpeg);
#endif
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
    _offerChoices(metaData, fact, values, labels, tr("%1 (not connected)"));
}

void VideoSettings::offerCameraFormats(const QStringList& formats)
{
    _cameraFormats = formats;
    _updateCameraFormats();
}

void VideoSettings::_updateCameraFormats()
{
    QVariantList values{QString()};
    QStringList labels{tr("Auto")};
    for (const QString& format : std::as_const(_cameraFormats)) {
        values.append(format);
        labels.append(format);
    }
    _offerChoices(_nameToMetaDataMap.value(uvcCameraFormatName), _uvcCameraFormatFact, values, labels,
                  tr("%1 (not offered)"));
}

void VideoSettings::_updateRecordingCodecs()
{
    QVariantList values{QString()};
    QStringList labels{tr("Auto")};
    for (const QString& key : QtVideoRecorder::recordingCodecs()) {
        values.append(key);
        labels.append(QtVideoRecorder::recordingCodecName(key));
    }
    _offerChoices(_nameToMetaDataMap.value(uvcRecordingCodecName), _uvcRecordingCodecFact, values, labels,
                  tr("%1 (not available)"));
}

void VideoSettings::_offerChoices(FactMetaData* metaData, Fact* fact, QVariantList values, QStringList labels,
                                  const QString& absentLabel)
{
    if (!metaData) {
        return;
    }
    const QString selected = fact ? fact->rawValue().toString() : QString();
    if (!selected.isEmpty() && !values.contains(selected)) {
        values.append(selected);
        labels.append(absentLabel.arg(selected));
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

DECLARE_SETTINGSFACT(VideoSettings, videoFit)
DECLARE_SETTINGSFACT(VideoSettings, gridLines)
DECLARE_SETTINGSFACT(VideoSettings, showTelemetryOverlay)
DECLARE_SETTINGSFACT(VideoSettings, recordingFormat)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoSize)
DECLARE_SETTINGSFACT(VideoSettings, enableStorageLimit)
DECLARE_SETTINGSFACT(VideoSettings, disableWhenDisarmed)
DECLARE_SETTINGSFACT(VideoSettings, udpUrl)
DECLARE_SETTINGSFACT(VideoSettings, rtspUrl)
DECLARE_SETTINGSFACT(VideoSettings, tcpUrl)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoTiles)
DECLARE_SETTINGSFACT(VideoSettings, uvcTileCameraAsMainPicture)
DECLARE_SETTINGSFACT(VideoSettings, uvcCameraRotation)
DECLARE_SETTINGSFACT(VideoSettings, uvcCameraMirror)
DECLARE_SETTINGSFACT(VideoSettings, uvcWhiteBalance)
DECLARE_SETTINGSFACT(VideoSettings, uvcZoom)
DECLARE_SETTINGSFACT(VideoSettings, uvcExposureCompensation)
DECLARE_SETTINGSFACT(VideoSettings, uvcFocusMode)
DECLARE_SETTINGSFACT(VideoSettings, uvcRecordingQuality)

// DECLARE_SETTINGSFACT for a setting whose choices UPDATE lists. It lists them as the Fact is created, when the stored
// value may be one that is no longer offered, and after each change of the value. Queued: the combo box that sets the
// value deletes its items when its model changes, which it mustn't do while one of them reports its click.
#define DECLARE_VIDEO_SETTINGSFACT_OFFERED(NAME, UPDATE)                                                               \
    DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, NAME)                                                                  \
    {                                                                                                                  \
        if (!_##NAME##Fact) {                                                                                          \
            _##NAME##Fact = _createSettingsFact(NAME##Name);                                                           \
            UPDATE();                                                                                                  \
            (void) connect(_##NAME##Fact, &Fact::rawValueChanged, this, &VideoSettings::UPDATE, Qt::QueuedConnection); \
        }                                                                                                              \
        return _##NAME##Fact;                                                                                          \
    }

DECLARE_VIDEO_SETTINGSFACT_OFFERED(videoSource, _updateVideoSources)
DECLARE_VIDEO_SETTINGSFACT_OFFERED(uvcTileCamera, _updateVideoSources)
DECLARE_VIDEO_SETTINGSFACT_OFFERED(uvcCameraFormat, _updateCameraFormats)
DECLARE_VIDEO_SETTINGSFACT_OFFERED(uvcRecordingCodec, _updateRecordingCodecs)

#undef DECLARE_VIDEO_SETTINGSFACT_OFFERED

// DECLARE_SETTINGSFACT for a setting shown only where VISIBLE holds, read as the Fact is created.
#define DECLARE_VIDEO_SETTINGSFACT_VISIBLE(NAME, VISIBLE)    \
    DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, NAME)        \
    {                                                        \
        if (!_##NAME##Fact) {                                \
            _##NAME##Fact = _createSettingsFact(NAME##Name); \
            _##NAME##Fact->setUserVisible(VISIBLE);          \
        }                                                    \
        return _##NAME##Fact;                                \
    }

#ifdef QGC_FFMPEG_VIDEO
// The experimental FFmpeg backend is a choice also without GStreamer, whose choice then plays through Qt Multimedia.
DECLARE_VIDEO_SETTINGSFACT_VISIBLE(videoBackend, true)
#else
// Only a GStreamer build offers a choice: without GStreamer, Qt Multimedia plays the streams.
DECLARE_VIDEO_SETTINGSFACT_VISIBLE(videoBackend, hasPipelineSettings())
#endif

DECLARE_VIDEO_SETTINGSFACT_VISIBLE(forceVideoDecoder, hasDecoderSettings())
DECLARE_VIDEO_SETTINGSFACT_VISIBLE(softwareDecoderThreads, hasDecoderSettings())
DECLARE_SETTINGSFACT(VideoSettings, lowLatencyMode)
DECLARE_VIDEO_SETTINGSFACT_VISIBLE(rtpJitterLatencyMs, hasDecoderSettings())
DECLARE_SETTINGSFACT(VideoSettings, rtspAutoReconnect)
DECLARE_VIDEO_SETTINGSFACT_VISIBLE(forceCpuVideoPath, VideoBackend::instance().capabilities().gpuVideoPath)

// videoConversionElement reaches the receivers' display sinks as a construct-only property
// (VideoStreamResolver::displayConfig()); VideoManager restarts the streams, which rebuilds the sinks, when it
// changes.
DECLARE_VIDEO_SETTINGSFACT_VISIBLE(videoConversionElement, hasPipelineSettings())
DECLARE_SETTINGSFACT(VideoSettings, forcedAspectRatio)

DECLARE_SETTINGSFACT(VideoSettings, rtspTimeout)

#undef DECLARE_VIDEO_SETTINGSFACT_VISIBLE

double VideoSettings::aspectRatioOf(int forcedAspectRatio)
{
    switch (forcedAspectRatio) {
        case AspectRatio16x9:
            return 16.0 / 9.0;
        case AspectRatio4x3:
            return 4.0 / 3.0;
        case AspectRatio5x4:
            return 5.0 / 4.0;
        case AspectRatio3x2:
            return 3.0 / 2.0;
        case AspectRatio1x1:
            return 1.0;
        default:
            return 0.0;
    }
}

bool VideoSettings::backendTakesDecoderSettings(int backend) const
{
#ifdef QGC_FFMPEG_VIDEO
    if (backend == VideoBackendFFmpeg) {
        return true;
    }
#endif
    return (backend == VideoBackendGStreamer) && hasPipelineSettings();
}

void VideoSettings::pruneUnavailableDecoders()
{
    if (!hasPipelineSettings()) {
        return;
    }
    static const QList<VideoDecoderOption> hardwareFamilies{
        VideoDecoderOption::NVIDIA,       VideoDecoderOption::VAAPI, VideoDecoderOption::DirectX3D,
        VideoDecoderOption::VideoToolbox, VideoDecoderOption::Intel,
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
