#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GPSObservation.h"
#include "GPSReceiverConnector.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverReports.h"
#include "ScheduledTask.h"

class GPSCorrectionManager;
class GPSReceiverConnectionPolicy;
class GPSReceiverFactGroup;
class GPSReceiverSession;
class GPSSourceHealth;
class PositionManager;
class RuntimeScheduler;
class SerialPortManager;

/// Services a GPSReceiver uses, which must outlive it.
struct GPSReceiverDependencies
{
    /// Receives a forwarding receiver's RTCM output as the local correction source.
    GPSCorrectionManager* corrections = nullptr;
    /// Ready receivers publish their solution here as the GCS "RTK receiver" position source.
    PositionManager* positions = nullptr;
#ifndef QGC_NO_SERIAL_LINK
    /// The serial devices a receiver may connect or discover, and their claims; without it none is available. A
    /// session holds its claim until its worker has stopped, and ends unplugged when a scan no longer lists its device.
    SerialPortManager* serialPorts = nullptr;
#endif
};

/// A serial device the receiver settings offer.
struct GPSSerialPortEntry
{
    Q_GADGET
    QML_VALUE_TYPE(gpsSerialPortEntry)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(QString value MEMBER value FINAL)
    Q_PROPERTY(QString label MEMBER label FINAL)

public:
    /// The system location, which the device setting stores.
    QString value;
    /// What the picker shows.
    QString label;

    bool operator==(const GPSSerialPortEntry&) const = default;
};

/// Runs the local GPS receiver session and publishes its status Facts, corrections, and position.
/// Public methods and report handlers update state first and emit each changed signal once at their end. The status
/// Facts update their raw values as the receiver writes them, and notify QML of new values at most once a second.
/// Observers may stop or replace the receiver from a signal, and delete it only with deleteLater().
class GPSReceiver : public QObject, private GPSReceiverConnector
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Managed by GPSManager")
    Q_MOC_INCLUDE("GPSReceiverFactGroup.h")

    Q_PROPERTY(GPSReceiverFactGroup* facts READ facts CONSTANT FINAL)
    Q_PROPERTY(QList<GPSSerialPortEntry> serialPortEntries READ serialPortEntries NOTIFY serialPortEntriesChanged FINAL)
    Q_PROPERTY(QList<int> serialBaudRates READ serialBaudRates CONSTANT FINAL)
    Q_PROPERTY(bool hasReceiver READ hasReceiver NOTIFY receiverChanged FINAL)
    Q_PROPERTY(bool serialSupported READ serialSupported CONSTANT FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(int activeBaseMode READ activeBaseMode NOTIFY receiverChanged FINAL)
    Q_PROPERTY(QString activeEndpoint READ activeEndpoint NOTIFY receiverChanged FINAL)
    Q_PROPERTY(QString receiverIdentity READ receiverIdentity NOTIFY receiverChanged FINAL)
    Q_PROPERTY(QString detectedReceiver READ detectedReceiver NOTIFY receiverChanged FINAL)
    Q_PROPERTY(bool reconnecting READ reconnecting NOTIFY receiverChanged FINAL)
    Q_PROPERTY(GPSReceiverPresentation activePresentation READ activePresentation NOTIFY receiverChanged FINAL)
    Q_PROPERTY(RTKSettings::ReceiverRole activeRole READ activeRole NOTIFY receiverChanged FINAL)
    /// The active receiver's RTCM output is a correction source.
    Q_PROPERTY(bool forwardingCorrections READ forwardingCorrections NOTIFY receiverChanged FINAL)
    /// The one-use permission to write receiver flash, for the configuration currently selected.
    Q_PROPERTY(bool persistentChangesAllowed READ persistentChangesAllowed WRITE setPersistentChangesAllowed NOTIFY
                   persistentChangesAllowedChanged FINAL)
    Q_PROPERTY(RTKSettings::ConnectionType effectiveConnectionType READ effectiveConnectionType NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(bool connectionSupported READ connectionSupported NOTIFY configurationChanged FINAL)
    /// Short state for compact views: a configured base's survey state, otherwise the fix type.
    Q_PROPERTY(QString summaryLabel READ summaryLabel NOTIFY summaryLabelChanged FINAL)
    /// What the receiver is doing, as a sentence; empty when no receiver is connected or being connected.
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged FINAL)
    /// The configured receiver supports the selected base mode; a passive receiver has none to support.
    Q_PROPERTY(bool baseModeSupported READ baseModeSupported NOTIFY configurationChanged FINAL)
    /// Connect may start the configured receiver: a selected receiver type, base mode and connection it supports.
    Q_PROPERTY(bool canConnect READ canConnect NOTIFY configurationChanged FINAL)

    friend class GPSReceiverTest;
    friend class GPSReceiverSettingsBindingTest;
    friend class GPSReceiverConnectionPolicyTest;

public:
    using Configuration = GPSReceiverConfiguration;
    using WorkerFactory = GPSReceiverWorkerFactory;

    /// How often connection polling ticks the receiver.
    static constexpr std::chrono::seconds POLL_INTERVAL{1};
    /// How long the warning about output the receiver dropped stays after the last report of it.
    static constexpr std::chrono::seconds OUTPUT_OVERFLOW_WARNING_DURATION{30};
    /// How long shutdown() waits for the workers of retired sessions.
    static constexpr std::chrono::seconds SHUTDOWN_TIMEOUT{2};

    using Dependencies = GPSReceiverDependencies;

    explicit GPSReceiver(QObject* parent = nullptr, RuntimeScheduler* scheduler = nullptr,
                         const Dependencies& dependencies = {});
    ~GPSReceiver();

    void setConfiguration(const Configuration& configuration);
    /// Inject before connecting; an empty factory restores the real worker.
    void setWorkerFactory(WorkerFactory factory);

    const Configuration& configuration() const { return _configuration; }

    /// The serial device picker's entries from the last scan: "value" is the system location, "label" the device's
    /// description and path, marked when another QGroundControl connection, such as a vehicle link, holds the port.
    QList<GPSSerialPortEntry> serialPortEntries() const { return _serialPortEntries; }

    /// The application's serial rates that a receiver accepts as an explicit rate; empty without serial links.
    static QList<int> serialBaudRates();

    /// Latest receiver solution that passes the consumer's gates; empty without a fresh fix.
    std::optional<GPSObservation> acceptedPositionObservation(GPSObservation::PositionUse use) const;
    /// Connects the configured receiver, spending the persistent-change permission on this attempt. Lifecycle
    /// observers may stop or replace this receiver, and delete it only with deleteLater().
    Q_INVOKABLE bool connectReceiver();
    /// Disconnects and pauses automatic connection until the user connects again.
    Q_INVOKABLE void disconnectReceiver();
    /// Runs tick() every POLL_INTERVAL until shutdown(), skipping ticks while @a suspended returns true.
    void startConnectionPolling(std::function<bool()> suspended = {});
    /// One step of automatic connection: connections due, reconnections, and USB discovery.
    void tick();
    /// Stops polling and disconnects, then waits up to SHUTDOWN_TIMEOUT for the workers of retired sessions, which
    /// otherwise finish on their own once the event loop delivers their exit.
    void shutdown();

    /// The receiver's reported state; with no receiver, every Fact is at its metadata default.
    GPSReceiverFactGroup* facts() const { return _facts; }

    bool hasReceiver() const override;

    bool serialSupported() const
    {
#ifdef QGC_NO_SERIAL_LINK
        return false;
#else
        return true;
#endif
    }

    /// The connection or input problem, else a recent report that the receiver dropped output.
    QString errorMessage() const;

    int activeBaseMode() const;

    /// Serial device or TCP host:port of the active receiver.
    QString activeEndpoint() const;

    QString receiverIdentity() const;

    /// Name of the family an Automatic connection detected, such as "Septentrio", or of the protocol a passive
    /// receiver's output carries, such as "u-blox (UBX)" or "NMEA"; empty until detected.
    QString detectedReceiver() const;

    QString summaryLabel() const { return _summaryLabel; }

    QString statusText() const { return _statusText; }

    /// The saved receiver is being connected automatically, at startup or after its connection was lost.
    bool reconnecting() const;

    /// Editable settings for a role; configured bases also depend on the manufacturer.
    Q_INVOKABLE GPSReceiverPresentation capabilitiesFor(int role, int manufacturer) const;

    /// Presentation capabilities of the connected receiver family.
    GPSReceiverPresentation activePresentation() const;

    RTKSettings::ReceiverRole activeRole() const;

    bool forwardingCorrections() const;

    bool persistentChangesAllowed() const { return _persistentChangesAllowed; }

    /// Granting permission only lasts until the configuration changes, a session starts or ends, or it is used.
    void setPersistentChangesAllowed(bool allowed);

    RTKSettings::ConnectionType effectiveConnectionType() const { return _configuration.effectiveConnectionType(); }

    /// UDP only receives, so a receiver QGroundControl configures needs serial or TCP.
    bool connectionSupported() const;

    bool baseModeSupported() const;

    bool canConnect() const;

signals:
    void receiverChanged();
    void errorMessageChanged();
    void configurationChanged();
    void persistentChangesAllowedChanged();
    void serialPortEntriesChanged();
    void summaryLabelChanged();
    void statusTextChanged();

private slots:
    void _onSatelliteInfoUpdated(const GPSSatelliteReport& msg);
    void _onPositionUpdated(const GPSPositionReport& report);
    void _onGPSConnect();
    void _onReceiverDetected();
    void _onInputProblem(GPSInputProblem problem);
    void _onGPSSurveyReport(const GPSSurveyReport& status);

private:
    /// The values behind receiverChanged.
    struct ReceiverState
    {
        bool hasReceiver = false;
        int baseMode = -1;
        QString endpoint;
        QString identity;
        QString detected;
        bool reconnecting = false;
        GPSReceiverPresentation presentation;
        RTKSettings::ReceiverRole role = RTKSettings::ConfiguredBase;
        bool forwardingCorrections = false;
        bool operator==(const ReceiverState&) const = default;
    };

    /// The values last notified, each recorded before its signal so a nested change never repeats an older one.
    struct Notified
    {
        quint64 configurationRevision = 0;
        bool persistentChangesAllowed = false;
        QString errorMessage;
        ReceiverState receiver;
    };

    int _activeManufacturer() const;

    void setConnectionError(const QString& message) override { _setError(message); }

    void endConnection(bool clearError) override;
    bool connectTcp(const QString& host, quint16 port, GPSType type, bool allowPersistentChanges) override;
    bool connectUdp(quint16 port, GPSType type) override;
#ifndef QGC_NO_SERIAL_LINK
    bool connectSerial(const QString& device, GPSType type, uint32_t baudRate, bool allowPersistentChanges,
                       GPSSerialClaim claim) override;
    bool serialPortAvailable(const QString& device) const override;
    QStringList rtkReceiverPorts() const override;
    GPSSerialClaim claimSerialPort(const QString& device) override;
    /// Ends a serial session whose device the scan no longer lists, then updates the picker entries.
    void _onSerialPortsEnumerated(const QStringList& devices);
    void _updateSerialPortEntries();
#endif

    bool _connectReceiver(GPSType type, GPSReceiverWorker::TransportFactory transportFactory,
                          const QString& sourceInstance, uint32_t baudRate, bool allowPersistentChanges,
                          const QString& serialDevice = {}, const QString& endpoint = {});
    void _endSession(GPSConnectionError error, const QString& detail, bool portRemoved);
    /// The retired session is deleted later, so it may be retired from inside its own signals.
    void _retireSession();
    /// Waits for the workers of retired sessions and deletes those that have exited.
    void _joinRetiredWorkers();

    void _setError(const QString& message = {}) { _errorMessage = message; }

    /// Shows the output-overflow warning for a report the receiver has not had before.
    void _updateOutputOverflow(uint64_t receiptUs);
    void _setPersistentChangesAllowed(bool allowed);
    void _resetStatus();
    void _updateLabels();
    /// Clears the receiver's fix, satellites, and integrity once it stops reporting position.
    void _clearStaleSolution();
    ReceiverState _receiverState() const;
    /// Emits each signal whose value changed since it was last notified.
    void _emitChanges();

    Configuration _configuration;
    GPSReceiverSession* _session = nullptr;
    /// Workers of retired sessions whose workers may still be running.
    QList<QPointer<GPSReceiverWorker>> _retiredWorkers;
    const QPointer<GPSCorrectionManager> _correctionManager;
    const QPointer<PositionManager> _positionManager;
    RuntimeScheduler* const _scheduler;
    GPSSourceHealth* const _positionHealth;
    ScheduledTask _outputOverflowTask;
    ScheduledTask _pollTask;
    quint64 _sessionCount = 0;
    QString _errorMessage;
    QString _summaryLabel;
    QString _statusText;
    QString _outputOverflowWarning;
    uint64_t _outputOverflowUs = 0;
    std::unique_ptr<GPSReceiverConnectionPolicy> _connection;
    GPSReceiverFactGroup* _facts = nullptr;
    WorkerFactory _workerFactory;
    quint64 _configurationRevision = 0;
    Notified _notified;
    bool _destroying = false;
    bool _persistentChangesAllowed = false;
    QList<GPSSerialPortEntry> _serialPortEntries;
#ifndef QGC_NO_SERIAL_LINK
    const QPointer<SerialPortManager> _serialPorts;
    /// The claims this receiver made, which never mark their port as in use, even while a retired session's worker
    /// still holds one.
    QHash<QString, std::weak_ptr<const SerialPortManager::Reservation>> _serialClaims;
    std::function<std::unique_ptr<GPSTransport>(const QString&, GPSCancelToken)> _serialTransportFactory;
#endif
};
