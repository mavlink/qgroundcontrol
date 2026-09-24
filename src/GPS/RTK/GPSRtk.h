#pragma once

#include <memory>
#include <optional>
#include <utility>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSCorrectionSourceRegistration.h"
#include "GPSEllipsoidPosition.h"
#include "GPSNotificationQueue.h"
#include "GPSObservation.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSProvider.h"

class GPSRTKFactGroup;
class GPSCorrectionManager;
class GPSPositionService;
class GPSSourceHealth;
class Fact;
class RTKConnectionPolicy;
class SerialPortManager;
class RTKSettings;

/// Runs the local RTK receiver session and publishes its Facts, corrections, and position.
/// Operations update state first; Facts, receiverChanged, and errorMessageChanged are published once
/// the outermost operation finishes, so observers never run inside an operation in progress.
class GPSRtk : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Managed by GPSManager")
    Q_MOC_INCLUDE("GPSRTKFactGroup.h")

    Q_PROPERTY(GPSRTKFactGroup* facts READ gpsRtkFactGroup CONSTANT)
    Q_PROPERTY(bool hasReceiver READ hasReceiver NOTIFY receiverChanged)
    Q_PROPERTY(bool serialSupported READ serialSupported CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int activeManufacturer READ activeManufacturer NOTIFY receiverChanged)
    Q_PROPERTY(int activeBaseMode READ activeBaseMode NOTIFY receiverChanged)
    Q_PROPERTY(QString activeEndpoint READ activeEndpoint NOTIFY receiverChanged)
    Q_PROPERTY(QString receiverIdentity READ receiverIdentity NOTIFY receiverChanged)
    Q_PROPERTY(bool reconnecting READ reconnecting NOTIFY receiverChanged)
    Q_PROPERTY(QGeoCoordinate basePosition READ basePosition NOTIFY basePositionChanged)
    Q_PROPERTY(bool basePositionFinal READ basePositionFinal NOTIFY basePositionChanged)

    friend class GPSRtkTest;
    friend class RTKConnectionPolicy;
    friend class RTKConnectionPolicyTest;

public:
    /// Values of RTKSettings::connectionType.
    enum ConnectionType
    {
        Serial = 0,
        Tcp = 1,
    };
    Q_ENUM(ConnectionType)

    explicit GPSRtk(QObject* parent = nullptr);
    ~GPSRtk();

#ifndef QGC_NO_SERIAL_LINK
    /// Connects outside the connection policy, which forgets any manual or auto-connected ownership.
    bool connectGPS(const QString& device, QStringView gps_type, uint32_t baudRate = 0,
                    bool allowPersistentChanges = false);
    void setSerialPortManager(SerialPortManager* serialPorts);
#endif
    /// Inject before connecting; the caller retains ownership.
    void setCorrectionManager(GPSCorrectionManager* manager);
    /// Ready receivers publish their solution as the GCS "RTK receiver" position source. Inject before connecting.
    void setPositionService(GPSPositionService* service);
    /// Latest receiver solution that passes the consumer's gates; empty without a fresh fix.
    std::optional<GPSObservation> acceptedPositionObservation(GPSObservation::PositionUse use) const;
    /// Synchronous lifecycle observers may stop, replace, or delete this receiver. Superseded attempts return false.
    bool connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory,
                         const QString& sourceInstance = {}, uint32_t baudRate = 0,
                         bool allowPersistentChanges = false);
    Q_INVOKABLE bool connectConfiguredGPS(bool allowPersistentChanges = false);
    Q_INVOKABLE void disconnectConfiguredGPS();
    /// Retires immediately and ends retries; the worker retains its transport reservation until cancellation completes.
    void disconnectGPS();

    /// Decides manual, auto-connected, and retried connections; GPSManager drives its periodic update().
    RTKConnectionPolicy* connectionPolicy() const { return _connection; }

    /// The current session's receiver finished configuration.
    bool connected() const { return _session.ready; }

    bool hasReceiver() const { return !_session.provider.isNull(); }

    bool serialSupported() const;

    QString errorMessage() const { return _errorMessage; }

    int activeManufacturer() const { return _session.manufacturer; }

    int activeBaseMode() const { return _session.baseMode; }

    /// Serial device or TCP host:port of the active receiver.
    QString activeEndpoint() const { return _session.endpoint; }

    QString receiverIdentity() const { return _session.identity; }

    /// A manually connected receiver was lost and QGroundControl is retrying the connection.
    bool reconnecting() const;

    /// Base antenna position: fixed or surveyed, else the survey-in mean so far; invalid without a base receiver.
    QGeoCoordinate basePosition() const;

    /// basePosition is the fixed or completed survey position rather than a survey in progress.
    bool basePositionFinal() const { return _session.basePosition.has_value(); }

    Q_INVOKABLE QVariantMap capabilitiesForManufacturer(int manufacturer) const;

    static std::optional<GPSType> typeForManufacturer(int manufacturer);
    static int manufacturerForType(GPSType type);

    GPSRTKFactGroup* gpsRtkFactGroup();

signals:
    void receiverChanged();
    void errorMessageChanged();
    void basePositionChanged();

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
        int baseMode = -1;
        // Fixed or surveyed antenna position; base receivers then report only a time fix.
        std::optional<std::pair<GPSEllipsoidPosition, double>> basePosition;
        std::optional<GPSEllipsoidPosition> surveyPosition;
        std::optional<double> surveyAccuracyLimitMeters;
        std::optional<int> fixType;
        bool started = false;
        bool ready = false;
    };

    static QString _receiverConfig(GPSType type, RTKSettings* settings, uint32_t baudRate, GPSReceiverConfig& config,
                                   bool allowPersistentChanges = false);
#ifndef QGC_NO_SERIAL_LINK
    bool _connectGPS(const QString& device, QStringView gps_type, uint32_t baudRate = 0,
                     bool allowPersistentChanges = false);
    bool _connectSerialGPS(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges);
#endif
    bool _connectTcpGPS(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges);
    bool _connectReceiver(GPSType type, GPSProvider::TransportFactory transportFactory, const QString& sourceInstance,
                          uint32_t baudRate, bool allowPersistentChanges, const QString& serialDevice = {},
                          const QString& endpoint = {});
    void _disconnect(bool clearError);
    void _endSession(GPSConnectionError error, const QString& detail, bool portRemoved);
    void _retireSession();
    void _setError(GPSConnectionError error, const QString& message = {});
    void _stageFact(Fact* fact, const QVariant& value);
    void _stageDisconnectedFacts();
    void _notifyReceiverChanged();

    ReceiverSession _session;
    // Fact setters can still be unwinding after a notification deletes their receiver owner.
    std::shared_ptr<GPSRTKFactGroup> _gpsRtkFactGroup;
    GPSNotificationQueue _notifications{this};
    QPointer<GPSCorrectionManager> _correctionManager;
    QPointer<GPSPositionService> _positionService;
    GPSSourceHealth* const _positionHealth;
    quint64 _sessionCount = 0;
    QString _errorMessage;
    GPSConnectionError _connectionError = GPSConnectionError::None;
    RTKConnectionPolicy* _connection = nullptr;
    bool _destroying = false;
#ifndef QGC_NO_SERIAL_LINK
    QPointer<SerialPortManager> _serialPorts;
    QMetaObject::Connection _portEnumerationConnection;
    std::function<std::unique_ptr<GPSTransport>(const QString&, const std::atomic_bool&)> _serialTransportFactory;
#endif
};
