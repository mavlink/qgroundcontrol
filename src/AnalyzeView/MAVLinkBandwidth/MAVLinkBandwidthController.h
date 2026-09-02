#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include <chrono>

#include "LinkInterface.h"
#include "MAVLinkBandwidthProtocol.h"
#include "MAVLinkFtpBandwidthTest.h"
#include "MAVLinkMessageType.h"

class Vehicle;
class FTPManager;

class MAVLinkBandwidthController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(MAVLinkBandwidthController)
    QML_ELEMENT
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(bool vehicleAvailable READ vehicleAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(bool endpointAvailable READ endpointAvailable NOTIFY endpointAvailableChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool stopping READ stopping NOTIFY stoppingChanged)
    Q_PROPERTY(bool canStart READ canStart NOTIFY canStartChanged)
    Q_PROPERTY(int activeVehicleId READ activeVehicleId NOTIFY availabilityChanged)
    Q_PROPERTY(QString linkName READ linkName NOTIFY linkNameChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(TestMode testMode READ testMode WRITE setTestMode NOTIFY configurationChanged)
    Q_PROPERTY(Direction direction READ direction WRITE setDirection NOTIFY configurationChanged)
    Q_PROPERTY(int targetRateKbps READ targetRateKbps WRITE setTargetRateKbps NOTIFY configurationChanged)
    Q_PROPERTY(int durationSeconds READ durationSeconds WRITE setDurationSeconds NOTIFY configurationChanged)
    Q_PROPERTY(int ftpFileSizeKiB READ ftpFileSizeKiB WRITE setFtpFileSizeKiB NOTIFY configurationChanged)
    Q_PROPERTY(bool streamingSupported READ streamingSupported NOTIFY availabilityChanged)
    Q_PROPERTY(bool streamingAvailable READ streamingAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(bool scriptDeploying READ scriptDeploying NOTIFY scriptDeploymentChanged)
    Q_PROPERTY(float scriptDeploymentProgress READ scriptDeploymentProgress NOTIFY scriptDeploymentProgressChanged)
    Q_PROPERTY(double progress READ progress NOTIFY statisticsChanged)
    Q_PROPERTY(double transmitRateKbps READ transmitRateKbps NOTIFY statisticsChanged)
    Q_PROPERTY(double receiveRateKbps READ receiveRateKbps NOTIFY statisticsChanged)
    Q_PROPERTY(double wireTransmitRateKbps READ wireTransmitRateKbps NOTIFY statisticsChanged)
    Q_PROPERTY(double wireReceiveRateKbps READ wireReceiveRateKbps NOTIFY statisticsChanged)
    Q_PROPERTY(quint64 transmittedPayloadBytes READ transmittedPayloadBytes NOTIFY statisticsChanged)
    Q_PROPERTY(quint64 receivedPayloadBytes READ receivedPayloadBytes NOTIFY statisticsChanged)
    Q_PROPERTY(quint64 transmittedPackets READ transmittedPackets NOTIFY statisticsChanged)
    Q_PROPERTY(quint64 receivedPackets READ receivedPackets NOTIFY statisticsChanged)
    Q_PROPERTY(quint64 lostPackets READ lostPackets NOTIFY statisticsChanged)
    Q_PROPERTY(quint64 outOfOrderPackets READ outOfOrderPackets NOTIFY statisticsChanged)
    Q_PROPERTY(quint64 sendFailures READ sendFailures NOTIFY statisticsChanged)

public:
    enum class TestMode
    {
        Streaming,
        MavFtp,
    };
    Q_ENUM(TestMode)

    enum class Direction
    {
        QgcToVehicle = 1,
        VehicleToQgc,
    };
    Q_ENUM(Direction)

    explicit MAVLinkBandwidthController(QObject* parent = nullptr);
    ~MAVLinkBandwidthController() override;

    bool vehicleAvailable() const;

    bool endpointAvailable() const { return _endpointAvailable; }

    bool running() const { return _running; }

    bool stopping() const { return _stopping; }

    bool canStart() const;

    int activeVehicleId() const;

    QString linkName() const;

    QString statusText() const { return _statusText; }

    TestMode testMode() const { return _testMode; }

    Direction direction() const { return _direction; }

    int targetRateKbps() const { return _targetRateKbps; }

    int durationSeconds() const { return _durationSeconds; }

    int ftpFileSizeKiB() const { return _ftpFileSizeKiB; }

    bool streamingSupported() const;

    bool streamingAvailable() const;

    bool scriptDeploying() const { return _scriptDeploying; }

    float scriptDeploymentProgress() const { return _scriptDeploymentProgress; }

    double progress() const { return _progress; }

    double transmitRateKbps() const { return _transmitRateKbps; }

    double receiveRateKbps() const { return _receiveRateKbps; }

    double wireTransmitRateKbps() const { return _wireTransmitRateKbps; }

    double wireReceiveRateKbps() const { return _wireReceiveRateKbps; }

    quint64 transmittedPayloadBytes() const { return _transmittedPayloadBytes; }

    quint64 receivedPayloadBytes() const { return _receivedPayloadBytes; }

    quint64 transmittedPackets() const { return _transmittedPackets; }

    quint64 receivedPackets() const { return _receivedPackets; }

    quint64 lostPackets() const { return _lostPackets; }

    quint64 outOfOrderPackets() const { return _outOfOrderPackets; }

    quint64 sendFailures() const { return _sendFailures; }

    void setTestMode(TestMode testMode);
    void setDirection(Direction direction);
    void setTargetRateKbps(int targetRateKbps);
    void setDurationSeconds(int durationSeconds);
    void setFtpFileSizeKiB(int ftpFileSizeKiB);

    Q_INVOKABLE void probeEndpoint();
    Q_INVOKABLE void installScriptAndReboot(int expectedVehicleId);
    Q_INVOKABLE void startTest();
    Q_INVOKABLE void stopTest();

signals:
    void availabilityChanged();
    void endpointAvailableChanged();
    void runningChanged();
    void stoppingChanged();
    void canStartChanged();
    void linkNameChanged();
    void statusTextChanged();
    void configurationChanged();
    void statisticsChanged();
    void scriptDeploymentChanged();
    void scriptDeploymentProgressChanged();
    void scriptDeploymentFinished(bool success, const QString& statusText);

private slots:
    void _setActiveVehicle(Vehicle* vehicle);
    void _updatePrimaryLink();
    void _receiveMessage(LinkInterface* link, const mavlink_message_t& message);
    void _sendTick();
    void _statisticsTick();
    void _testTimeout();
    void _probeTimeout();
    void _bytesReceived(LinkInterface* link, const QByteArray& data);
    void _bytesSent(LinkInterface* link, const QByteArray& data);
    void _armedChanged(bool armed);
    void _ftpPhaseChanged(const QString& phaseText);
    void _ftpMeasurementStarted(MAVLinkFtpBandwidthTest::Direction direction);
    void _ftpMeasurementProgress(MAVLinkFtpBandwidthTest::Direction direction, float progress);
    void _ftpMeasurementComplete(MAVLinkFtpBandwidthTest::Direction direction, qint64 elapsedMs, quint64 payloadBytes);
    void _ftpCleanupStarted();
    void _ftpFinished(bool success, const QString& statusText);
    void _scriptDeleteComplete(const QString& remotePath, const QString& errorMessage);
    void _scriptDownloadComplete(const QString& localPath, const QString& errorMessage);
    void _scriptReplaceComplete(const QString& fromPath, const QString& toPath, const QString& errorMessage);
    void _scriptUploadComplete(const QString& remotePath, const QString& errorMessage);
    void _scriptUploadProgress(float progress);

private:
    enum class ScriptDeploymentPhase
    {
        Idle,
        DeletingStaging,
        UploadingStaging,
        VerifyingStaging,
        Replacing,
    };

    void _startStreamingTest();
    void _startFtpTest();
    void _abortActiveTest(const QString& statusText);
    void _detachFtpTestForCleanup(const QString& statusText);
    void _cancelProbe();
    void _cancelScriptDeployment(const QString& statusText);
    void _finishScriptDeployment(bool success, const QString& statusText);
    void _setScriptDeploying(bool deploying);
    void _resetStatistics();
    void _finishTest(const QString& statusText, bool completed);
    void _setEndpointAvailable(bool available);
    void _setRunning(bool running);
    void _setStopping(bool stopping);
    void _setStatusText(const QString& statusText);
    bool _sendPacket(const MAVLinkBandwidth::Packet& packet);
    uint32_t _newSessionId() const;
    void _updateAvailabilityStatus();

    Vehicle* _vehicle = nullptr;
    SharedLinkInterfacePtr _link;
    QList<QMetaObject::Connection> _vehicleConnections;
    QList<QMetaObject::Connection> _linkConnections;
    QTimer _sendTimer;
    QTimer _statisticsTimer;
    QTimer _testTimer;
    QTimer _probeTimer;
    QElapsedTimer _elapsedTimer;
    MAVLinkFtpBandwidthTest* _ftpTest = nullptr;
    QPointer<FTPManager> _scriptFtpManager;
    QMetaObject::Connection _scriptDeleteCompleteConnection;
    QMetaObject::Connection _scriptDownloadCompleteConnection;
    QMetaObject::Connection _scriptReplaceCompleteConnection;
    QMetaObject::Connection _scriptUploadCompleteConnection;
    QMetaObject::Connection _scriptUploadProgressConnection;
    TestMode _testMode = TestMode::Streaming;
    Direction _direction = Direction::QgcToVehicle;
    QString _statusText;
    int _targetRateKbps = 100;
    int _durationSeconds = 5;
    int _ftpFileSizeKiB = 1024;
    uint32_t _sessionId = 0;
    uint32_t _probeSessionId = 0;
    uint32_t _nextTransmitSequence = 0;
    uint32_t _lastReceiveSequence = 0;
    bool _haveReceiveSequence = false;
    bool _endpointAvailable = false;
    bool _running = false;
    bool _stopping = false;
    bool _ftpMeasuring = false;
    bool _scriptDeploying = false;
    ScriptDeploymentPhase _scriptDeploymentPhase = ScriptDeploymentPhase::Idle;
    float _scriptDeploymentProgress = 0.F;
    QTemporaryDir _scriptTemporaryDirectory;
    QByteArray _scriptExpectedHash;
    MAVLinkFtpBandwidthTest::Direction _ftpMeasurementDirection = MAVLinkFtpBandwidthTest::Direction::QgcToVehicle;
    qint64 _ftpUploadElapsedMs = 0;
    qint64 _ftpDownloadElapsedMs = 0;
    qint64 _streamingElapsedMs = 0;
    qint64 _lastEndpointElapsedMs = 0;
    bool _awaitingTerminalReport = false;
    bool _probeActive = false;
    double _progress = 0.;
    double _transmitRateKbps = 0.;
    double _receiveRateKbps = 0.;
    double _wireTransmitRateKbps = 0.;
    double _wireReceiveRateKbps = 0.;
    quint64 _transmittedPayloadBytes = 0;
    quint64 _receivedPayloadBytes = 0;
    quint64 _transmittedPackets = 0;
    quint64 _receivedPackets = 0;
    quint64 _lostPackets = 0;
    quint64 _outOfOrderPackets = 0;
    quint64 _sendFailures = 0;
    quint64 _wireTransmitBytes = 0;
    quint64 _wireReceiveBytes = 0;

    static constexpr std::chrono::milliseconds kSendInterval{10};
    static constexpr std::chrono::milliseconds kStatisticsInterval{200};
    static constexpr std::chrono::milliseconds kProbeTimeout{2000};
    static constexpr std::chrono::milliseconds kFinalReportTimeout{1500};
    static constexpr std::chrono::seconds kFtpInactivityTimeout{15};
    static constexpr int kMaximumPacketsPerTick = 32;
    static constexpr const char* kEmbeddedScriptPath = ":/mavlink-bandwidth/mavlink_bandwidth.lua";
    static constexpr const char* kRemoteScriptPath = "/APM/scripts/mavlink_bandwidth.lua";
    static constexpr const char* kStagingScriptPath = "/APM/scripts/.mavlink_bandwidth.lua.qgc-upload";
};
