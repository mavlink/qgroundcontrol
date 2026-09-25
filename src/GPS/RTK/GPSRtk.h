#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSDriverReports.h"
#include "GPSEllipsoidPosition.h"
#include "GPSFixQuality.h"
#include "GPSNotificationQueue.h"
#include "GPSObservation.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSProvider.h"
#include "GPSReceiverDescriptor.h"
#include "RTKConnectionTarget.h"

class GPSCorrectionManager;
class GPSPositionService;
class GPSSerialPorts;
class GPSSourceHealth;
class RuntimeScheduler;
class RTKConnectionPolicy;

/// Runs the local RTK receiver session and publishes its status, corrections, and position.
/// Operations update state first; statusChanged, receiverChanged, and errorMessageChanged are published once
/// the outermost operation finishes, so observers never run inside an operation in progress.
class GPSRtk : public QObject, private RTKConnectionTarget
{
    Q_OBJECT

    Q_PROPERTY(bool hasReceiver READ hasReceiver NOTIFY receiverChanged)
    Q_PROPERTY(bool serialSupported READ serialSupported CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int activeManufacturer READ activeManufacturer NOTIFY receiverChanged)
    Q_PROPERTY(int activeBaseMode READ activeBaseMode NOTIFY receiverChanged)
    Q_PROPERTY(QString activeEndpoint READ activeEndpoint NOTIFY receiverChanged)
    Q_PROPERTY(QString receiverIdentity READ receiverIdentity NOTIFY receiverChanged)
    Q_PROPERTY(bool reconnecting READ reconnecting NOTIFY receiverChanged)
    Q_PROPERTY(GPSReceiverPresentation activePresentation READ activePresentation NOTIFY receiverChanged)
    Q_PROPERTY(ReceiverRole activeRole READ activeRole NOTIFY receiverChanged)

    friend class GPSRtkTest;
    friend class RTKConnectionPolicyTest;

public:
    /// Values of RTKSettings::connectionType.
    enum ConnectionType
    {
        Serial = 0,
        Tcp = 1,
        Udp = 2,
    };
    Q_ENUM(ConnectionType)

    /// Values of RTKSettings::receiverRole. Only a configured base is written to by QGroundControl.
    enum ReceiverRole
    {
        PositionOnly = 0,
        Passive = 1,
        ConfiguredBase = 2,
    };
    Q_ENUM(ReceiverRole)

    /// The application's settings binding supplies the receiver configuration as values.
    struct Configuration
    {
        ReceiverRole receiverRole = ConfiguredBase;
        int baseReceiverManufacturer = 1;
        ConnectionType connectionType = Serial;
        QString tcpHost;
        uint32_t tcpPort = 0;
        uint32_t udpPort = 14401;
        QString serialDevice;
        uint32_t serialBaudRate = 115200;
        int baseMode = 0;
        double fixedBasePositionLatitude = 0.;
        double fixedBasePositionLongitude = 0.;
        float fixedBasePositionAltitude = 0.f;
        float fixedBasePositionAccuracy = 0.f;
        double surveyInAccuracyLimit = 2.;
        int64_t surveyInMinObservationDuration = 180;
        uint32_t receiverAveragingDuration = 60;
        bool compactRtcmCorrections = false;
        bool autoConnect = true;
        bool operator==(const Configuration&) const = default;
    };

    /// The receiver's reported state; the defaults describe no receiver. Unavailable values are NaN or -1.
    struct Status
    {
        /// The session's receiver finished configuration.
        bool connected = false;
        // Survey-in progress of a configured base.
        std::chrono::seconds currentDuration{0};
        double currentAccuracy = std::numeric_limits<double>::quiet_NaN();
        double currentLatitude = std::numeric_limits<double>::quiet_NaN();
        double currentLongitude = std::numeric_limits<double>::quiet_NaN();
        float currentAltitude = std::numeric_limits<float>::quiet_NaN();
        bool valid = false;
        bool active = false;
        int numSatellites = -1;
        int numSatellitesUsed = -1;
        GPSFixQuality fixType = GPSFixQuality::Unknown;
        GPSIntegrityReport::JammingState jammingState = GPSIntegrityReport::JammingState::Unknown;
        GPSIntegrityReport::SpoofingState spoofingState = GPSIntegrityReport::SpoofingState::Unknown;
    };

    explicit GPSRtk(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr);
    ~GPSRtk();

    using ProviderFactory =
        std::function<GPSProvider*(GPSProvider::TransportFactory, GPSType, const GPSReceiverConfig&, QObject*)>;

    void setConfiguration(const Configuration& configuration);
    /// Inject before connecting; an empty factory restores the real worker provider.
    void setProviderFactory(ProviderFactory factory);

    const Configuration& configuration() const { return _configuration; }

#ifndef QGC_NO_SERIAL_LINK
    /// Inject before connecting; the caller retains ownership.
    void setSerialPorts(GPSSerialPorts* serialPorts);
#endif
    /// Inject before connecting; the caller retains ownership.
    void setCorrectionManager(GPSCorrectionManager* manager);
    /// Ready receivers publish their solution as the GCS "RTK receiver" position source. Inject before connecting.
    void setPositionService(GPSPositionService* service);
    /// Latest receiver solution that passes the consumer's gates; empty without a fresh fix.
    std::optional<GPSObservation> acceptedPositionObservation(GPSObservation::PositionUse use) const;
    /// Lifecycle observers may stop or replace this receiver, and delete it only with deleteLater().
    /// Superseded attempts return false.
    /// A passive @a type forwards the receiver's RTCM output unless @a role is PositionOnly.
    bool connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                         const QString& sourceInstance = {}, uint32_t baudRate = 0, bool allowPersistentChanges = false,
                         std::optional<ReceiverRole> role = std::nullopt);
    Q_INVOKABLE bool connectConfiguredGPS(bool allowPersistentChanges = false);
    Q_INVOKABLE void disconnectConfiguredGPS();
    /// Retires immediately and ends retries; the worker retains its transport reservation until cancellation completes.
    void disconnectGPS();

    /// Decides manual, auto-connected, and retried connections; GPSManager drives its periodic update().
    RTKConnectionPolicy* connectionPolicy() const { return _connection; }

    /// The current session's receiver finished configuration.
    bool connected() const { return _session.ready; }

    bool hasReceiver() const override { return !_session.provider.isNull(); }

    bool serialSupported() const;

    QString errorMessage() const { return _errorMessage; }

    int activeManufacturer() const { return _session.manufacturer; }

    int activeBaseMode() const { return _session.baseMode; }

    /// Serial device or TCP host:port of the active receiver.
    QString activeEndpoint() const { return _session.endpoint; }

    QString receiverIdentity() const { return _session.identity; }

    /// A manually connected receiver was lost and QGroundControl is retrying the connection.
    bool reconnecting() const;

    /// Editable settings for a role; configured bases also depend on the manufacturer.
    Q_INVOKABLE GPSReceiverPresentation capabilitiesFor(int role, int manufacturer) const;

    /// Presentation capabilities of the connected receiver family.
    GPSReceiverPresentation activePresentation() const;

    ReceiverRole activeRole() const { return _session.role; }

    static std::optional<GPSType> typeForManufacturer(int manufacturer);
    static int manufacturerForType(GPSType type);

    const Status& status() const { return _status; }

signals:
    /// Coalesced per operation; status() holds the published state.
    void statusChanged();
    void receiverChanged();
    void errorMessageChanged();
    void autoConnectDisabled();
    void baseManufacturerDetected(int manufacturer);

private slots:
    void _satelliteInfoUpdate(const GPSSatelliteReport& msg);
    void _positionUpdate(const GPSPositionReport& report);
    void _onGPSConnect(const QString& identity = {});
    void _onGPSSurveyReport(const GPSSurveyReport& status);

private:
    struct ReceiverSession
    {
        QPointer<GPSProvider> provider;
        GPSCorrectionSourceRegistration corrections;
        GPSPositionSourceRegistration position;
        quint64 id = 0;
        QString serialDevice;
        QString endpoint;
        QString identity;
        int manufacturer = 0;
        ReceiverRole role = ConfiguredBase;
        int baseMode = -1;
        // Fixed or surveyed antenna position; base receivers then report only a time fix.
        std::optional<std::pair<GPSEllipsoidPosition, double>> basePosition;
        std::optional<double> surveyAccuracyLimitMeters;
        std::optional<int> fixType;
        bool started = false;
        bool ready = false;
    };

    static QString _receiverConfig(GPSType type, const Configuration& configuration, uint32_t baudRate,
                                   GPSReceiverConfig& config, bool allowPersistentChanges = false);

    GPSConnectionError connectionError() const override { return _connectionError; }

    void setConnectionError(GPSConnectionError error, const QString& message) override { _setError(error, message); }

    void disconnectReceiver(bool clearError) override;
    bool connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges) override;
    bool connectUdp(quint16 port, GPSType type) override;
#ifndef QGC_NO_SERIAL_LINK
    GPSSerialPorts* serialPorts() const override;
    bool connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges) override;
    bool connectDiscovered(const QString& device, QStringView boardName) override;
#endif
    GPSNotificationQueue& notifications() override { return _notifications; }

    bool _connectReceiver(GPSType type, ReceiverRole role, GPSProvider::TransportFactory transportFactory,
                          const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                          const QString& serialDevice = {}, const QString& endpoint = {});
    /// The saved role applies to passive receivers; every other type is a configured base.
    ReceiverRole _roleFor(GPSType type) const;
    void _endSession(GPSConnectionError error, const QString& detail, bool portRemoved);
    void _retireSession();
    void _setError(GPSConnectionError error, const QString& message = {});
    void _publishStatus();
    void _resetStatus();
    /// Clears the receiver's fix, satellites, and integrity once it stops reporting position.
    void _clearStaleSolution();
    void _notifyReceiverChanged();

    Configuration _configuration;
    ReceiverSession _session;
    Status _status;
    GPSNotificationQueue _notifications{this};
    QPointer<GPSCorrectionManager> _correctionManager;
    QPointer<GPSPositionService> _positionService;
    GPSSourceHealth* const _positionHealth;
    quint64 _sessionCount = 0;
    QString _errorMessage;
    GPSConnectionError _connectionError = GPSConnectionError::None;
    RTKConnectionPolicy* _connection = nullptr;
    ProviderFactory _providerFactory;
    bool _destroying = false;
#ifndef QGC_NO_SERIAL_LINK
    QPointer<GPSSerialPorts> _serialPorts;
    QMetaObject::Connection _portEnumerationConnection;
    std::function<std::unique_ptr<GPSTransport>(const QString&, const std::atomic_bool&)> _serialTransportFactory;
#endif
};

QDebug operator<<(QDebug debug, const GPSRtk::Configuration& configuration);
