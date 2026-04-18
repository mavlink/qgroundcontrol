#pragma once

#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class VideoSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
public:
    VideoSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(videoSource)
    DEFINE_SETTINGFACT(udpUrl)
    DEFINE_SETTINGFACT(tcpUrl)
    DEFINE_SETTINGFACT(rtspUrl)
    DEFINE_SETTINGFACT(aspectRatio)
    DEFINE_SETTINGFACT(videoFit)
    DEFINE_SETTINGFACT(gridLines)
    DEFINE_SETTINGFACT(showVideoStats)
    DEFINE_SETTINGFACT(showRecControl)
    DEFINE_SETTINGFACT(recordingFormat)
    DEFINE_SETTINGFACT(maxVideoSize)
    DEFINE_SETTINGFACT(enableStorageLimit)
    DEFINE_SETTINGFACT(rtspTimeout)
    DEFINE_SETTINGFACT(streamEnabled)
    DEFINE_SETTINGFACT(disableWhenDisarmed)
    DEFINE_SETTINGFACT(lowLatencyMode)
    DEFINE_SETTINGFACT(forceVideoDecoder)
    DEFINE_SETTINGFACT(videoSavePath)

    Q_PROPERTY(bool streamConfigured READ streamConfigured NOTIFY streamConfiguredChanged)
    Q_PROPERTY(QString rtspVideoSource READ rtspVideoSource CONSTANT)
    Q_PROPERTY(QString udp264VideoSource READ udp264VideoSource CONSTANT)
    Q_PROPERTY(QString udp265VideoSource READ udp265VideoSource CONSTANT)
    Q_PROPERTY(QString tcpVideoSource READ tcpVideoSource CONSTANT)
    Q_PROPERTY(QString mpegtsVideoSource READ mpegtsVideoSource CONSTANT)
    Q_PROPERTY(QString disabledVideoSource READ disabledVideoSource CONSTANT)
    Q_PROPERTY(QString gstPipelineVideoSource READ gstPipelineVideoSource CONSTANT)

    bool streamConfigured();

    QString rtspVideoSource() { return videoSourceRTSP; }

    QString udp264VideoSource() { return videoSourceUDPH264; }

    QString udp265VideoSource() { return videoSourceUDPH265; }

    QString tcpVideoSource() { return videoSourceTCP; }

    QString mpegtsVideoSource() { return videoSourceMPEGTS; }

    QString disabledVideoSource() { return videoDisabled; }

    QString gstPipelineVideoSource() { return videoSourceGstPipeline; }

    DEFINE_SETTINGFACT(gstPipelineUrl)

    /// Compile-time safe video source identification.
    /// Replaces scattered string comparisons with switch-able dispatch.
    enum class SourceType : uint8_t
    {
        NoVideo,
        Disabled,
        RTSP,
        UDPH264,
        UDPH265,
        TCP,
        MPEGTS,
        GstPipeline,
        Solo3DR,
        ParrotDiscovery,
        YuneecMantisG,
        HerelinkAirUnit,
        HerelinkHotspot,
        UVC,      ///< Dynamic — matches any UVC device name
        Unknown,  ///< Unrecognized source string
    };

    /// Parse the string persisted in the videoSource Fact into an enum.
    static SourceType sourceTypeFromString(const QString& s);

    /// Get the current source type (convenience wrapper).
    SourceType currentSourceType();

    // String constants — kept for QML/persistence backward compatibility.
    // New C++ code should use SourceType enum; these are the persistence keys.
    static constexpr const char* videoSourceNoVideo = QT_TRANSLATE_NOOP("VideoSettings", "No Video Available");
    static constexpr const char* videoDisabled = QT_TRANSLATE_NOOP("VideoSettings", "Video Stream Disabled");
    static constexpr const char* videoSourceRTSP = QT_TRANSLATE_NOOP("VideoSettings", "RTSP Video Stream");
    static constexpr const char* videoSourceUDPH264 = QT_TRANSLATE_NOOP("VideoSettings", "UDP h.264 Video Stream");
    static constexpr const char* videoSourceUDPH265 = QT_TRANSLATE_NOOP("VideoSettings", "UDP h.265 Video Stream");
    static constexpr const char* videoSourceTCP = QT_TRANSLATE_NOOP("VideoSettings", "TCP-MPEG2 Video Stream");
    static constexpr const char* videoSourceMPEGTS = QT_TRANSLATE_NOOP("VideoSettings", "MPEG-TS Video Stream");
    static constexpr const char* videoSourceGstPipeline = QT_TRANSLATE_NOOP("VideoSettings", "GStreamer Pipeline");
    static constexpr const char* videoSource3DRSolo = QT_TRANSLATE_NOOP("VideoSettings", "3DR Solo (requires restart)");
    static constexpr const char* videoSourceParrotDiscovery = QT_TRANSLATE_NOOP("VideoSettings", "Parrot Discovery");
    static constexpr const char* videoSourceYuneecMantisG = QT_TRANSLATE_NOOP("VideoSettings", "Yuneec Mantis G");
    static constexpr const char* videoSourceHerelinkAirUnit = QT_TRANSLATE_NOOP("VideoSettings", "Herelink AirUnit");
    static constexpr const char* videoSourceHerelinkHotspot = QT_TRANSLATE_NOOP("VideoSettings", "Herelink Hotspot");

signals:
    void streamConfiguredChanged(bool configured);

private slots:
    void _configChanged(QVariant value);

private:
    void _setDefaults();
    void _setForceVideoDecodeList();

private:
    bool _noVideo = false;
};
