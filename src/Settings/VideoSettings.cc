#include "VideoSettings.h"

#include "QGCLoggingCategory.h"
#include <QtCore/QSettings>
#include <QtCore/QVariantList>
#include <QtMultimedia/QMediaFormat>

#include "VideoSourceAvailability.h"

QGC_LOGGING_CATEGORY(VideoSettingsLog, "Settings.VideoSettings")

DECLARE_SETTINGGROUP(Video, "Video")
{
    // Setup enum values for videoSource settings into meta data
    QVariantList videoSourceList = VideoSourceAvailability::availableSourceNames();
    if (videoSourceList.count() == 0) {
        _noVideo = true;
        videoSourceList.append(videoSourceNoVideo);
        setUserVisible(false);
    } else {
        videoSourceList.insert(0, videoDisabled);
    }

    // make translated strings
    QStringList videoSourceCookedList;
    for (const QVariant& videoSource : videoSourceList) {
        videoSourceCookedList.append(VideoSettings::tr(videoSource.toString().toStdString().c_str()));
    }

    _nameToMetaDataMap[videoSourceName]->setEnumInfo(videoSourceCookedList, videoSourceList);

    // ── One-shot migration: legacy FILE_FORMAT (0/1/2) → QMediaFormat::FileFormat ──
    // Old values: FILE_FORMAT_MKV=0, FILE_FORMAT_MOV=1, FILE_FORMAT_MP4=2
    // New values: QMediaFormat::Matroska=2, QMediaFormat::QuickTime=5, QMediaFormat::MPEG4=3
    // We use a raw QSettings marker key to ensure this runs exactly once.
    {
        QSettings migrationMarker;
        const QLatin1String markerKey("Video/recordingFormatMigrated_v2");
        if (!migrationMarker.value(markerKey, false).toBool()) {
            // Read the raw persisted value before the Fact system applies the default.
            QSettings rawSettings;
            const QLatin1String rawKey("Video/recordingFormat");
            if (rawSettings.contains(rawKey)) {
                const int old = rawSettings.value(rawKey).toInt();
                int migrated = static_cast<int>(QMediaFormat::Matroska);  // safe default
                switch (old) {
                    case 0: migrated = static_cast<int>(QMediaFormat::Matroska);  break;  // MKV
                    case 1: migrated = static_cast<int>(QMediaFormat::QuickTime); break;  // MOV
                    case 2: migrated = static_cast<int>(QMediaFormat::MPEG4);     break;  // MP4
                    default: break;  // already a new value or unrecognised — keep as-is
                }
                rawSettings.setValue(rawKey, migrated);
            }
            migrationMarker.setValue(markerKey, true);
        }
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
DECLARE_SETTINGSFACT(VideoSettings, showVideoStats)
DECLARE_SETTINGSFACT(VideoSettings, showRecControl)
DECLARE_SETTINGSFACT(VideoSettings, recordingFormat)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoSize)
DECLARE_SETTINGSFACT(VideoSettings, enableStorageLimit)
DECLARE_SETTINGSFACT(VideoSettings, streamEnabled)
DECLARE_SETTINGSFACT(VideoSettings, disableWhenDisarmed)
DECLARE_SETTINGSFACT(VideoSettings, videoSavePath)

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoSource)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
        //-- Check for sources no longer available
        if (!_videoSourceFact->enumValues().contains(_videoSourceFact->rawValue().toString())) {
            if (_noVideo) {
                _videoSourceFact->setRawValue(videoSourceNoVideo);
            } else {
                _videoSourceFact->setRawValue(videoDisabled);
            }
        }
        connect(_videoSourceFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _videoSourceFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, lowLatencyMode)
{
    if (!_lowLatencyModeFact) {
        _lowLatencyModeFact = _createSettingsFact(lowLatencyModeName);

        _lowLatencyModeFact->setUserVisible(true);

        connect(_lowLatencyModeFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _lowLatencyModeFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspTimeout)
{
    if (!_rtspTimeoutFact) {
        _rtspTimeoutFact = _createSettingsFact(rtspTimeoutName);

        _rtspTimeoutFact->setUserVisible(true);

        connect(_rtspTimeoutFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspTimeoutFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, udpUrl)
{
    if (!_udpUrlFact) {
        _udpUrlFact = _createSettingsFact(udpUrlName);
        connect(_udpUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _udpUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspUrl)
{
    if (!_rtspUrlFact) {
        _rtspUrlFact = _createSettingsFact(rtspUrlName);
        connect(_rtspUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, tcpUrl)
{
    if (!_tcpUrlFact) {
        _tcpUrlFact = _createSettingsFact(tcpUrlName);
        connect(_tcpUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _tcpUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, gstPipelineUrl)
{
    if (!_gstPipelineUrlFact) {
        _gstPipelineUrlFact = _createSettingsFact(gstPipelineUrlName);
        connect(_gstPipelineUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _gstPipelineUrlFact;
}

VideoSettings::SourceType VideoSettings::sourceTypeFromString(const QString& s)
{
    return VideoSourceAvailability::sourceTypeFromString(s);
}

VideoSettings::SourceType VideoSettings::currentSourceType()
{
    return sourceTypeFromString(videoSource()->rawValue().toString());
}

bool VideoSettings::streamConfigured()
{
    return VideoSourceAvailability::manualSourceConfigured(currentSourceType(),
                                                          rtspUrl()->rawValue().toString(),
                                                          udpUrl()->rawValue().toString(),
                                                          tcpUrl()->rawValue().toString(),
                                                          gstPipelineUrl()->rawValue().toString());
}

void VideoSettings::_configChanged(QVariant)
{
    emit streamConfiguredChanged(streamConfigured());
}
