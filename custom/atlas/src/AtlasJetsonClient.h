#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QJsonObject>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtNetwork/QTcpSocket>

class AtlasJetsonClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool hm30Connected READ hm30Connected NOTIFY hm30ConnectedChanged)
    Q_PROPERTY(bool a8Connected READ a8Connected NOTIFY a8ConnectedChanged)

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

    Q_PROPERTY(QString host READ host NOTIFY endpointChanged)
    Q_PROPERTY(int port READ port NOTIFY endpointChanged)

public:
    explicit AtlasJetsonClient(QObject *parent = nullptr);
    ~AtlasJetsonClient() override;

    bool connected() const { return _connected; }
    bool hm30Connected() const { return _hm30Connected; }
    bool a8Connected() const { return _a8Connected; }

    int rttMs() const { return _rttMs; }

    double jetsonCpuPercent() const { return _jetsonCpuPercent; }
    double jetsonGpuPercent() const { return _jetsonGpuPercent; }
    double jetsonRamUsedGB() const { return _jetsonRamUsedGB; }
    double jetsonRamTotalGB() const { return _jetsonRamTotalGB; }
    double jetsonTempC() const { return _jetsonTempC; }
    double jetsonPowerW() const { return _jetsonPowerW; }
    double jetsonSwapGB() const { return _jetsonSwapGB; }
    bool jetsonThrottled() const { return _jetsonThrottled; }

    double hm30SignalPercent() const { return _hm30SignalPercent; }
    double hm30RssiDbm() const { return _hm30RssiDbm; }
    double hm30PacketLossPercent() const { return _hm30PacketLossPercent; }
    double hm30LatencyMs() const { return _hm30LatencyMs; }
    double hm30UpMbps() const { return _hm30UpMbps; }
    double hm30DownMbps() const { return _hm30DownMbps; }

    bool recording() const { return _recording; }
    double zoomLevel() const { return _zoomLevel; }
    double gimbalYawDeg() const { return _gimbalYawDeg; }
    double gimbalPitchDeg() const { return _gimbalPitchDeg; }

    bool aiEnabled() const { return _aiEnabled; }
    bool sahiEnabled() const { return _sahiEnabled; }
    bool depthEnabled() const { return _depthEnabled; }
    bool tracking() const { return _tracking; }
    bool trackingModeEnabled() const { return _trackingModeEnabled; }
    bool autoGimbal() const { return _autoGimbal; }

    QString aiModel() const { return _aiModel; }
    QString performanceProfile() const { return _performanceProfile; }
    QString missionProfile() const { return _missionProfile; }
    QString streamMode() const { return _streamMode; }
    QString videoProfile() const { return _videoProfile; }
    QStringList aiClasses() const { return _aiClasses; }

    double confidence() const { return _confidence; }
    int aiFpsTarget() const { return _aiFpsTarget; }
    int aiResolution() const { return _aiResolution; }
    int sahiSlice() const { return _sahiSlice; }
    double aiFps() const { return _aiFps; }
    double videoFps() const { return _videoFps; }
    double videoBitrateMbps() const { return _videoBitrateMbps; }

    bool targetValid() const { return _targetValid; }
    int targetId() const { return _targetId; }
    QString targetClass() const { return _targetClass; }
    double targetConfidence() const { return _targetConfidence; }
    double targetDistanceM() const { return _targetDistanceM; }
    double targetBearingDeg() const { return _targetBearingDeg; }
    double targetLatitude() const { return _targetLatitude; }
    double targetLongitude() const { return _targetLongitude; }

    QString host() const { return _host; }
    int port() const { return static_cast<int>(_port); }

    Q_INVOKABLE void connectToHost(const QString &host, int port);
    Q_INVOKABLE void disconnectFromHost();

    Q_INVOKABLE void gimbalRate(double yaw, double pitch);
    Q_INVOKABLE void gimbalStop();
    Q_INVOKABLE void gimbalCenter();
    Q_INVOKABLE void gimbalPreset(const QString &preset);

    Q_INVOKABLE void zoom(int value);
    Q_INVOKABLE void takePhoto();
    Q_INVOKABLE void setRecording(bool enabled);

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
    void connectedChanged();
    void hm30ConnectedChanged();
    void a8ConnectedChanged();
    void statusChanged();
    void endpointChanged();

    void eventReceived(const QString &level, const QString &message, qint64 timestampMs);

private slots:
    void _onConnected();
    void _onDisconnected();
    void _onReadyRead();
    void _onSocketError(QAbstractSocket::SocketError error);
    void _connectNow();
    void _onHeartbeat();

private:
    void _setConnected(bool connected);
    void _setHm30Connected(bool connected);
    void _setA8Connected(bool connected);
    void _clearRemoteLinkStatus();
    void _scheduleReconnect();
    void _sendCommand(QJsonObject command);
    bool _applyStatus(const QJsonObject &message);

    static constexpr int kHeartbeatIntervalMs = 500;
    static constexpr qint64 kWatchdogTimeoutMs = 2000;

    QTcpSocket _socket;
    QTimer _reconnectTimer;
    QTimer _heartbeatTimer;
    QElapsedTimer _lastRxTimer;

    QString _host = QStringLiteral("127.0.0.1");
    quint16 _port = 45900;
    QByteArray _rxBuffer;

    bool _connected = false;
    bool _hm30Connected = false;
    bool _a8Connected = false;
    bool _wantConnection = false;
    bool _errorLogged = false;

    int _rttMs = -1;

    double _jetsonCpuPercent = 0.0;
    double _jetsonGpuPercent = 0.0;
    double _jetsonRamUsedGB = 0.0;
    double _jetsonRamTotalGB = 8.0;
    double _jetsonTempC = 0.0;
    double _jetsonPowerW = 0.0;
    double _jetsonSwapGB = 0.0;
    bool _jetsonThrottled = false;

    double _hm30SignalPercent = 0.0;
    double _hm30RssiDbm = 0.0;
    double _hm30PacketLossPercent = 0.0;
    double _hm30LatencyMs = 0.0;
    double _hm30UpMbps = 0.0;
    double _hm30DownMbps = 0.0;

    bool _recording = false;
    double _zoomLevel = 1.0;
    double _gimbalYawDeg = 0.0;
    double _gimbalPitchDeg = 0.0;

    bool _aiEnabled = true;
    bool _sahiEnabled = false;
    bool _depthEnabled = false;
    bool _tracking = false;
    bool _trackingModeEnabled = false;
    bool _autoGimbal = false;

    QString _aiModel = QStringLiteral("YOLO11s");
    QString _performanceProfile = QStringLiteral("BALANCED");
    QString _missionProfile = QStringLiteral("SEARCH");
    QString _streamMode = QStringLiteral("AI");
    QString _videoProfile = QStringLiteral("LOW");
    QStringList _aiClasses = {QStringLiteral("PERSON")};

    double _confidence = 0.45;
    int _aiFpsTarget = 18;
    int _aiResolution = 640;
    int _sahiSlice = 640;
    double _aiFps = 0.0;
    double _videoFps = 0.0;
    double _videoBitrateMbps = 0.0;

    bool _targetValid = false;
    int _targetId = -1;
    QString _targetClass;
    double _targetConfidence = 0.0;
    double _targetDistanceM = 0.0;
    double _targetBearingDeg = 0.0;
    double _targetLatitude = 0.0;
    double _targetLongitude = 0.0;

    quint64 _sequence = 0;
};
