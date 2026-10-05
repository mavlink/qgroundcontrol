#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

class AtlasJetsonClient;
class AtlasSiyiClient;

class AtlasPayloadRouter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QString modeDisplayName READ modeDisplayName NOTIFY modeChanged)
    Q_PROPERTY(bool directMode READ directMode NOTIFY modeChanged)
    Q_PROPERTY(bool jetsonMode READ jetsonMode NOTIFY modeChanged)
    Q_PROPERTY(bool aiAvailable READ aiAvailable NOTIFY modeChanged)
    Q_PROPERTY(bool hm30MetricsAvailable READ hm30MetricsAvailable NOTIFY modeChanged)

    Q_PROPERTY(QString directA8Host READ directA8Host NOTIFY endpointsChanged)
    Q_PROPERTY(int directA8Port READ directA8Port NOTIFY endpointsChanged)
    Q_PROPERTY(QString directRtspUrl READ directRtspUrl NOTIFY endpointsChanged)
    Q_PROPERTY(QString jetsonHost READ jetsonHost NOTIFY endpointsChanged)
    Q_PROPERTY(int jetsonPort READ jetsonPort NOTIFY endpointsChanged)
    Q_PROPERTY(QString jetsonRtspUrl READ jetsonRtspUrl NOTIFY endpointsChanged)
    Q_PROPERTY(QString activeControlHost READ activeControlHost NOTIFY routeChanged)
    Q_PROPERTY(QString activeRtspUrl READ activeRtspUrl NOTIFY routeChanged)

    Q_PROPERTY(bool connected READ connected NOTIFY statusChanged)
    Q_PROPERTY(bool hm30Connected READ hm30Connected NOTIFY statusChanged)
    Q_PROPERTY(bool a8Connected READ a8Connected NOTIFY statusChanged)
    Q_PROPERTY(bool directA8Connected READ directA8Connected NOTIFY statusChanged)
    Q_PROPERTY(bool jetsonConnected READ jetsonConnected NOTIFY statusChanged)

    Q_PROPERTY(int rttMs READ rttMs NOTIFY statusChanged)

    Q_PROPERTY(double jetsonCpuPercent READ jetsonCpuPercent NOTIFY statusChanged)
    Q_PROPERTY(double jetsonGpuPercent READ jetsonGpuPercent NOTIFY statusChanged)
    Q_PROPERTY(double jetsonRamUsedGB READ jetsonRamUsedGB NOTIFY statusChanged)
    Q_PROPERTY(double jetsonRamTotalGB READ jetsonRamTotalGB NOTIFY statusChanged)
    Q_PROPERTY(double jetsonTempC READ jetsonTempC NOTIFY statusChanged)
    Q_PROPERTY(double jetsonPowerW READ jetsonPowerW NOTIFY statusChanged)
    Q_PROPERTY(double jetsonSwapGB READ jetsonSwapGB NOTIFY statusChanged)
    Q_PROPERTY(bool jetsonThrottled READ jetsonThrottled NOTIFY statusChanged)

    Q_PROPERTY(double hm30SignalPercent READ hm30SignalPercent NOTIFY statusChanged)
    Q_PROPERTY(double hm30RssiDbm READ hm30RssiDbm NOTIFY statusChanged)
    Q_PROPERTY(double hm30PacketLossPercent READ hm30PacketLossPercent NOTIFY statusChanged)
    Q_PROPERTY(double hm30LatencyMs READ hm30LatencyMs NOTIFY statusChanged)
    Q_PROPERTY(double hm30UpMbps READ hm30UpMbps NOTIFY statusChanged)
    Q_PROPERTY(double hm30DownMbps READ hm30DownMbps NOTIFY statusChanged)

    Q_PROPERTY(bool recording READ recording NOTIFY statusChanged)
    Q_PROPERTY(double zoomLevel READ zoomLevel NOTIFY statusChanged)
    Q_PROPERTY(double gimbalYawDeg READ gimbalYawDeg NOTIFY statusChanged)
    Q_PROPERTY(double gimbalPitchDeg READ gimbalPitchDeg NOTIFY statusChanged)

    // Verified SIYI A8 codec/status settings (Direct A8 route).
    Q_PROPERTY(bool a8MainStreamInfoValid READ a8MainStreamInfoValid NOTIFY statusChanged)
    Q_PROPERTY(QString a8MainStreamCodec READ a8MainStreamCodec NOTIFY statusChanged)
    Q_PROPERTY(int a8MainStreamWidth READ a8MainStreamWidth NOTIFY statusChanged)
    Q_PROPERTY(int a8MainStreamHeight READ a8MainStreamHeight NOTIFY statusChanged)
    Q_PROPERTY(int a8MainStreamBitrateKbps READ a8MainStreamBitrateKbps NOTIFY statusChanged)
    Q_PROPERTY(int a8MainStreamFps READ a8MainStreamFps NOTIFY statusChanged)
    Q_PROPERTY(bool a8RecordStreamInfoValid READ a8RecordStreamInfoValid NOTIFY statusChanged)
    Q_PROPERTY(QString a8RecordStreamCodec READ a8RecordStreamCodec NOTIFY statusChanged)
    Q_PROPERTY(int a8RecordStreamWidth READ a8RecordStreamWidth NOTIFY statusChanged)
    Q_PROPERTY(int a8RecordStreamHeight READ a8RecordStreamHeight NOTIFY statusChanged)
    Q_PROPERTY(int a8RecordStreamBitrateKbps READ a8RecordStreamBitrateKbps NOTIFY statusChanged)
    Q_PROPERTY(int a8RecordStreamFps READ a8RecordStreamFps NOTIFY statusChanged)

    Q_PROPERTY(bool aiEnabled READ aiEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool sahiEnabled READ sahiEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool depthEnabled READ depthEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool tracking READ tracking NOTIFY statusChanged)
    Q_PROPERTY(bool trackingModeEnabled READ trackingModeEnabled NOTIFY statusChanged)
    Q_PROPERTY(bool autoGimbal READ autoGimbal NOTIFY statusChanged)

    Q_PROPERTY(QString aiModel READ aiModel NOTIFY statusChanged)
    Q_PROPERTY(QString performanceProfile READ performanceProfile NOTIFY statusChanged)
    Q_PROPERTY(QString missionProfile READ missionProfile NOTIFY statusChanged)
    Q_PROPERTY(QString streamMode READ streamMode NOTIFY statusChanged)
    Q_PROPERTY(QString videoProfile READ videoProfile NOTIFY statusChanged)
    Q_PROPERTY(QStringList aiClasses READ aiClasses NOTIFY statusChanged)

    Q_PROPERTY(double confidence READ confidence NOTIFY statusChanged)
    Q_PROPERTY(int aiFpsTarget READ aiFpsTarget NOTIFY statusChanged)
    Q_PROPERTY(int aiResolution READ aiResolution NOTIFY statusChanged)
    Q_PROPERTY(int sahiSlice READ sahiSlice NOTIFY statusChanged)
    Q_PROPERTY(double aiFps READ aiFps NOTIFY statusChanged)
    Q_PROPERTY(double videoFps READ videoFps NOTIFY statusChanged)
    Q_PROPERTY(double videoBitrateMbps READ videoBitrateMbps NOTIFY statusChanged)

    Q_PROPERTY(bool targetValid READ targetValid NOTIFY statusChanged)
    Q_PROPERTY(int targetId READ targetId NOTIFY statusChanged)
    Q_PROPERTY(QString targetClass READ targetClass NOTIFY statusChanged)
    Q_PROPERTY(double targetConfidence READ targetConfidence NOTIFY statusChanged)
    Q_PROPERTY(double targetDistanceM READ targetDistanceM NOTIFY statusChanged)
    Q_PROPERTY(double targetBearingDeg READ targetBearingDeg NOTIFY statusChanged)
    Q_PROPERTY(double targetLatitude READ targetLatitude NOTIFY statusChanged)
    Q_PROPERTY(double targetLongitude READ targetLongitude NOTIFY statusChanged)

public:
    explicit AtlasPayloadRouter(AtlasJetsonClient *jetsonClient,
                                AtlasSiyiClient *siyiClient,
                                QObject *parent = nullptr);

    QString mode() const;
    QString modeDisplayName() const;
    bool directMode() const;
    bool jetsonMode() const;
    bool aiAvailable() const { return jetsonMode(); }
    bool hm30MetricsAvailable() const { return jetsonMode(); }

    QString directA8Host() const { return _directA8Host; }
    int directA8Port() const { return _directA8Port; }
    QString directRtspUrl() const { return _directRtspUrl; }
    QString jetsonHost() const { return _jetsonHost; }
    int jetsonPort() const { return _jetsonPort; }
    QString jetsonRtspUrl() const { return _jetsonRtspUrl; }
    QString activeControlHost() const;
    QString activeRtspUrl() const;

    bool connected() const;
    bool hm30Connected() const;
    bool a8Connected() const;
    bool directA8Connected() const;
    bool jetsonConnected() const;

    int rttMs() const;

    double jetsonCpuPercent() const;
    double jetsonGpuPercent() const;
    double jetsonRamUsedGB() const;
    double jetsonRamTotalGB() const;
    double jetsonTempC() const;
    double jetsonPowerW() const;
    double jetsonSwapGB() const;
    bool jetsonThrottled() const;

    double hm30SignalPercent() const;
    double hm30RssiDbm() const;
    double hm30PacketLossPercent() const;
    double hm30LatencyMs() const;
    double hm30UpMbps() const;
    double hm30DownMbps() const;

    bool recording() const;
    double zoomLevel() const;
    double gimbalYawDeg() const;
    double gimbalPitchDeg() const;

    bool a8MainStreamInfoValid() const;
    QString a8MainStreamCodec() const;
    int a8MainStreamWidth() const;
    int a8MainStreamHeight() const;
    int a8MainStreamBitrateKbps() const;
    int a8MainStreamFps() const;
    bool a8RecordStreamInfoValid() const;
    QString a8RecordStreamCodec() const;
    int a8RecordStreamWidth() const;
    int a8RecordStreamHeight() const;
    int a8RecordStreamBitrateKbps() const;
    int a8RecordStreamFps() const;

    bool aiEnabled() const;
    bool sahiEnabled() const;
    bool depthEnabled() const;
    bool tracking() const;
    bool trackingModeEnabled() const;
    bool autoGimbal() const;

    QString aiModel() const;
    QString performanceProfile() const;
    QString missionProfile() const;
    QString streamMode() const;
    QString videoProfile() const;
    QStringList aiClasses() const;

    double confidence() const;
    int aiFpsTarget() const;
    int aiResolution() const;
    int sahiSlice() const;
    double aiFps() const;
    double videoFps() const;
    double videoBitrateMbps() const;

    bool targetValid() const;
    int targetId() const;
    QString targetClass() const;
    double targetConfidence() const;
    double targetDistanceM() const;
    double targetBearingDeg() const;
    double targetLatitude() const;
    double targetLongitude() const;

    void start();

    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void configureDirectA8(const QString &host, int port, const QString &rtspUrl);
    Q_INVOKABLE void configureJetson(const QString &host, int port, const QString &rtspUrl);

    Q_INVOKABLE void gimbalRate(double yaw, double pitch);
    Q_INVOKABLE void gimbalStop();
    Q_INVOKABLE void gimbalCenter();
    Q_INVOKABLE void gimbalPreset(const QString &preset);
    Q_INVOKABLE void zoom(int value);
    Q_INVOKABLE void takePhoto();
    Q_INVOKABLE void setRecording(bool enabled);

    Q_INVOKABLE void refreshA8CodecSpecs();
    Q_INVOKABLE void configureA8MainStream(const QString &codec, int width, int height, int bitrateKbps);
    Q_INVOKABLE void configureA8RecordStream(int width, int height, int bitrateKbps);
    Q_INVOKABLE void setA8GimbalWorkingMode(const QString &mode);
    Q_INVOKABLE void setA8VideoOutput(const QString &output);

    Q_INVOKABLE void setAiEnabled(bool enabled);
    Q_INVOKABLE void setSahiEnabled(bool enabled);
    Q_INVOKABLE void setDepthEnabled(bool enabled);
    Q_INVOKABLE void setAiModel(const QString &model);
    Q_INVOKABLE void setPerformanceProfile(const QString &profile);
    Q_INVOKABLE void setMissionProfile(const QString &profile);
    Q_INVOKABLE void setStreamMode(const QString &mode);
    Q_INVOKABLE void setVideoProfile(const QString &profile);
    Q_INVOKABLE void setAiClasses(const QStringList &classes);
    Q_INVOKABLE void setConfidence(double value);
    Q_INVOKABLE void setAiFpsTarget(int value);
    Q_INVOKABLE void setAiResolution(int value);
    Q_INVOKABLE void setSahiSlice(int value);
    Q_INVOKABLE void setTrackingModeEnabled(bool enabled);
    Q_INVOKABLE void trackPoint(double xNorm, double yNorm);
    Q_INVOKABLE void trackRoi(double xNorm, double yNorm, double widthNorm, double heightNorm);
    Q_INVOKABLE void setAutoGimbal(bool enabled);
    Q_INVOKABLE void clearTracking();

signals:
    void modeChanged();
    void endpointsChanged();
    void routeChanged();
    void statusChanged();
    void eventReceived(const QString &level, const QString &message, qint64 timestampMs);

private:
    enum class Mode {
        DirectA8,
        Jetson,
    };

    void _loadSettings();
    void _saveSettings() const;
    void _activateMode();
    void _applyVideoUrl(const QString &url);
    void _disableVideoIfUnconfigured();
    void _emitRouteEvent(const QString &message);

    AtlasJetsonClient *_jetson = nullptr;
    AtlasSiyiClient *_siyi = nullptr;
    Mode _mode = Mode::Jetson;
    bool _started = false;

    QString _directA8Host = QStringLiteral("192.168.144.25");
    int _directA8Port = 37260;
    QString _directRtspUrl = QStringLiteral("rtsp://192.168.144.25:8554/main.264");

    QString _jetsonHost = QStringLiteral("127.0.0.1");
    int _jetsonPort = 45900;
    QString _jetsonRtspUrl;
};
