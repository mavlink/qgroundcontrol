#pragma once

#include <functional>

#include <QtCore/QStringList>
#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class VideoSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    friend class VideoSettingsTest;

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
    DEFINE_SETTINGFACT(showTelemetryOverlay)
    DEFINE_SETTINGFACT(showRecControl)
    DEFINE_SETTINGFACT(recordingFormat)
    DEFINE_SETTINGFACT(maxVideoSize)
    DEFINE_SETTINGFACT(enableStorageLimit)
    /// For RTSP streams only: the others keep VideoReceiver::Config's timeout.
    DEFINE_SETTINGFACT(rtspTimeout)
    DEFINE_SETTINGFACT(disableWhenDisarmed)
    DEFINE_SETTINGFACT(lowLatencyMode)
    DEFINE_SETTINGFACT(rtpJitterLatencyMs)
    /// For streams of every transport, despite its name, which is the one settings are stored under.
    DEFINE_SETTINGFACT(rtspAutoReconnect)
    DEFINE_SETTINGFACT(videoBackend)
    DEFINE_SETTINGFACT(forceVideoDecoder)
    DEFINE_SETTINGFACT(forceCpuVideoPath)
    DEFINE_SETTINGFACT(videoConversionElement)
    DEFINE_SETTINGFACT(disablePixelAspectRatio)
    /// How many other video sources the video view shows in tiles beside the main video; 0 for none.
    DEFINE_SETTINGFACT(maxVideoTiles)
    /// The USB camera to show in a tile while the video source is a stream; empty for none.
    DEFINE_SETTINGFACT(uvcTileCamera)
    /// The tile's USB camera is the main picture, and the stream is in its tile: what a tap on the camera's tile chose.
    DEFINE_SETTINGFACT(uvcTileCameraAsMainPicture)
    DEFINE_SETTINGFACT(uvcCameraFormat)
    DEFINE_SETTINGFACT(uvcCameraRotation)
    DEFINE_SETTINGFACT(uvcCameraMirror)
    DEFINE_SETTINGFACT(uvcWhiteBalance)
    DEFINE_SETTINGFACT(uvcZoom)
    DEFINE_SETTINGFACT(uvcExposureCompensation)
    DEFINE_SETTINGFACT(uvcFocusMode)
    /// For every recording Qt encodes, a USB camera's or a Qt Multimedia stream's, despite their names, which are the
    /// ones the settings are stored under.
    DEFINE_SETTINGFACT(uvcRecordingCodec)
    DEFINE_SETTINGFACT(uvcRecordingQuality)

    /// videoFit's values (Video.SettingsGroup.json).
    enum VideoFit
    {
        VideoFitWidth = 0,
        VideoFitHeight = 1,
        VideoFitFill = 2,
        VideoFitNoCrop = 3,
    };
    Q_ENUM(VideoFit)

    /// videoBackend's values (Video.SettingsGroup.json).
    enum VideoBackendChoice
    {
        VideoBackendGStreamer = 0,
        VideoBackendQtMultimedia = 1,
    };
    Q_ENUM(VideoBackendChoice)

    Q_PROPERTY(QString  rtspVideoSource         READ rtspVideoSource        CONSTANT)
    Q_PROPERTY(QString  udp264VideoSource       READ udp264VideoSource      CONSTANT)
    Q_PROPERTY(QString  udp265VideoSource       READ udp265VideoSource      CONSTANT)
    Q_PROPERTY(QString  tcpVideoSource          READ tcpVideoSource         CONSTANT)
    Q_PROPERTY(QString  mpegtsVideoSource       READ mpegtsVideoSource      CONSTANT)
    Q_PROPERTY(QString  disabledVideoSource     READ disabledVideoSource    CONSTANT)

    QString  rtspVideoSource        () { return videoSourceRTSP; }
    QString  udp264VideoSource      () { return videoSourceUDPH264; }
    QString  udp265VideoSource      () { return videoSourceUDPH265; }
    QString  tcpVideoSource         () { return videoSourceTCP; }
    QString  mpegtsVideoSource      () { return videoSourceMPEGTS; }
    QString  disabledVideoSource    () { return videoDisabled; }

    /// Remove hardware forced-decoder options absent from the running GStreamer registry, and
    /// reset the active choice to Default if it was pruned. Call after the video backend has
    /// initialized (the registry is empty until then).
    void pruneUnavailableDecoders();

    static constexpr const char* videoDisabled                = QT_TRANSLATE_NOOP("VideoSettings", "Video Stream Disabled");
    static constexpr const char* videoSourceRTSP              = QT_TRANSLATE_NOOP("VideoSettings", "RTSP Video Stream");
    static constexpr const char* videoSourceUDPH264           = QT_TRANSLATE_NOOP("VideoSettings", "UDP h.264 Video Stream");
    static constexpr const char* videoSourceUDPH265           = QT_TRANSLATE_NOOP("VideoSettings", "UDP h.265 Video Stream");
    static constexpr const char* videoSourceTCP               = QT_TRANSLATE_NOOP("VideoSettings", "TCP-MPEG2 Video Stream");
    static constexpr const char* videoSourceMPEGTS            = QT_TRANSLATE_NOOP("VideoSettings", "MPEG-TS Video Stream");
    // A Solo sends video only after a handshake, which APMFirmwarePlugin::initializeVehicle() makes as the vehicle
    // connects: selected later, the source plays nothing until the next connection. The text is also the stored value.
    static constexpr const char* videoSource3DRSolo           = QT_TRANSLATE_NOOP("VideoSettings", "3DR Solo (requires restart)");
    static constexpr const char* videoSourceParrotDiscovery   = QT_TRANSLATE_NOOP("VideoSettings", "Parrot Discovery");
    static constexpr const char* videoSourceYuneecMantisG     = QT_TRANSLATE_NOOP("VideoSettings", "Yuneec Mantis G");
    static constexpr const char* videoSourceHerelinkAirUnit   = QT_TRANSLATE_NOOP("VideoSettings", "Herelink AirUnit");
    static constexpr const char* videoSourceHerelinkHotspot   = QT_TRANSLATE_NOOP("VideoSettings", "Herelink Hotspot");

private:
    void _setForceVideoDecodeList();
    /// Lists what videoSource offers: no video, the stream sources, the attached cameras and, when it is none of those,
    /// the selected source. That one is a camera that isn't attached: it stays selected, so that it plays once it is
    /// plugged in. Lists what uvcTileCamera offers the same way: no camera, then the cameras.
    void _updateVideoSources();
    /// Gives @p fact, or its @p metaData until it exists, the choices @p values labelled @p labels, then the attached
    /// @p cameras, then the selected value when it is none of those.
    void _offerCameras(FactMetaData* metaData, Fact* fact, QVariantList values, QStringList labels,
                       const QStringList& cameras);

    /// Stands in for UVCDevices::deviceNames() in tests.
    std::function<QStringList()> _cameraNames;
};
