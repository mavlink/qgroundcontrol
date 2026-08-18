#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "LinkInterface.h"
#include "MAVLinkBandwidthProtocol.h"
#include "MAVLinkMessageType.h"

class Vehicle;
class MAVLinkFtpBandwidthTest;

class MAVLinkBandwidthController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(bool vehicleAvailable READ vehicleAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(bool endpointAvailable READ endpointAvailable NOTIFY endpointAvailableChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool canStart READ canStart NOTIFY canStartChanged)
    Q_PROPERTY(QString linkName READ linkName NOTIFY linkNameChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(TestMode testMode READ testMode WRITE setTestMode NOTIFY configurationChanged)
    Q_PROPERTY(Direction direction READ direction WRITE setDirection NOTIFY configurationChanged)
    Q_PROPERTY(int targetRateKbps READ targetRateKbps WRITE setTargetRateKbps NOTIFY configurationChanged)
    Q_PROPERTY(int durationSeconds READ durationSeconds WRITE setDurationSeconds NOTIFY configurationChanged)
    Q_PROPERTY(int ftpFileSizeKiB READ ftpFileSizeKiB WRITE setFtpFileSizeKiB NOTIFY configurationChanged)
    Q_PROPERTY(bool streamingAvailable READ streamingAvailable NOTIFY availabilityChanged)
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

    bool canStart() const;

    QString linkName() const;

    QString statusText() const { return _statusText; }

    TestMode testMode() const { return _testMode; }

    Direction direction() const { return _direction; }

    int targetRateKbps() const { return _targetRateKbps; }

    int durationSeconds() const { return _durationSeconds; }

    int ftpFileSizeKiB() const { return _ftpFileSizeKiB; }

    bool streamingAvailable() const;

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
    Q_INVOKABLE void startTest();
    Q_INVOKABLE void stopTest();

signals:
    void availabilityChanged();
    void endpointAvailableChanged();
    void runningChanged();
    void canStartChanged();
    void linkNameChanged();
    void statusTextChanged();
    void configurationChanged();
    void statisticsChanged();

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
    void _ftpPreparationProgress(float progress);
    void _ftpMeasurementStarted();
    void _ftpMeasurementProgress(float progress);
    void _ftpMeasurementComplete(qint64 elapsedMs, quint64 payloadBytes);
    void _ftpFinished(bool success, const QString& statusText);

private:
    void _startStreamingTest();
    void _startFtpTest();
    void _abortActiveTest(const QString& statusText);
    void _resetStatistics();
    void _finishTest(const QString& statusText, bool completed);
    void _setEndpointAvailable(bool available);
    void _setRunning(bool running);
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
    bool _ftpMeasuring = false;
    qint64 _ftpMeasurementElapsedMs = 0;
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

    static constexpr int kSendIntervalMs = 10;
    static constexpr int kStatisticsIntervalMs = 200;
    static constexpr int kProbeTimeoutMs = 2000;
    static constexpr int kMaximumPacketsPerTick = 16;
};
