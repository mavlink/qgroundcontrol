#pragma once

#include <memory>
#include <optional>
#include <utility>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSObservation.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverConnector.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverReports.h"
#include "GPSReceiverWorker.h"
#include "PositionManager.h"

class GPSCorrectionManager;
class GPSCorrectionSourceHandle;
class GPSSourceHealth;

/// A receiver session's base-station mode, and the state and antenna position its receiver reports.
struct GPSBaseStationState
{
    /// Base mode of a configured base; empty for a passive receiver.
    std::optional<BaseModeDefinition::Mode> mode;
    /// What the receiver last reported of its base position; Inactive until it reports, and for a passive receiver.
    GPSBaseState state = GPSBaseState::Inactive;
    /// Antenna position and its accuracy in meters while the receiver reports it valid; the receiver then reports only
    /// a time fix.
    std::optional<std::pair<GPSEllipsoidPosition, double>> position;
    /// Accuracy of a base position reported without one: the fixed position's, or the limit of a survey-in.
    std::optional<double> assumedAccuracyMeters;

    /// A base in any mode takes the reported state, and adopts a valid, located position and otherwise has none.
    void applyStatus(const GPSBaseStatus& status);
};

/// What the detection of an Automatic session looks for at the moment.
struct GPSDetectionProgress
{
    /// The family that is asked to identify itself; GPSType::Automatic while detection listens for the output of
    /// any family.
    GPSType family = GPSType::Automatic;
    /// The rate the link runs at meanwhile.
    unsigned baud = 0;
    /// The family is asked where the session's hint says it was, before detection scans for any.
    bool remembered = false;

    bool operator==(const GPSDetectionProgress&) const = default;
};

/// Builds the receiver request for one connection: a passive receiver gets no base settings, and compact
/// observations apply only where the family supports them. @a error, when given, receives the translated diagnostic,
/// which is empty when the receiver supports the request; the request is returned either way.
[[nodiscard]] GPSReceiverConfig gpsReceiverConfigFor(const GPSBaseStationConfig& base, GPSType type, uint32_t baudRate,
                                                     bool allowPersistentChanges, QString* error);

/// The state a session starts from: its base mode, without a position until the receiver reports one.
[[nodiscard]] GPSBaseStationState gpsBaseStationStateFor(GPSType type, const GPSReceiverConfig& config);

/// One receiver connection of GPSReceiver: its worker, correction and position registrations, and identity.
/// Worker signals are queued to the session and relayed only until it is retired. retire() disconnects them, but
/// Qt still delivers calls it had already queued, so a retired session drops those itself.
class GPSReceiverSession : public QObject
{
    Q_OBJECT

public:
    struct Connection
    {
        GPSType type = GPSType::UBlox;
        RTKSettings::ReceiverRole role = RTKSettings::ConfiguredBase;
        /// The receiver's RTCM output is a correction source.
        bool forwardsCorrections = true;
        GPSReceiverConfig config{};
        /// Serial device whose removal ends the session; empty for network receivers.
        QString serialDevice;
        /// Serial device or network address shown to the user.
        QString endpoint;
        /// Names the link among those receivers are detected on: "serial:" and the device, or "tcp:" and the address.
        QString link;
        /// Where an Automatic session looks for its receiver first; no hint for any other.
        GPSDetectionHint detectionHint;
    };

    GPSReceiverSession(quint64 id, Connection connection, QObject* parent);
    ~GPSReceiverSession() override;

    /// Forwards the receiver's RTCM as the local correction source when the connection asks for it.
    void registerCorrections(GPSCorrectionManager* manager, const QString& sourceInstance);
    /// Creates the worker, with a real worker for an empty @a factory, and starts it. False when none is created.
    bool start(const GPSReceiverWorkerFactory& factory, GPSReceiverWorker::TransportFactory transportFactory);
    /// Publishes @a producer as the position source of @a receiver. Registration observers may retire this session.
    void registerPosition(PositionManager* positionManager, GPSSourceHealth* producer,
                          PositionManager::Receiver receiver = PositionManager::Receiver::Rtk);
    /// Disconnects and stops the worker and ends both registrations. The worker keeps its transport, and so any
    /// serial reservation, until it exits and deletes itself.
    void retire();

    bool hasWorker() const { return !_worker.isNull(); }

    /// The worker, which outlives the session once retired until its thread exits.
    QPointer<GPSReceiverWorker> worker() const { return _worker; }

    RTKSettings::ReceiverRole role() const { return _connection.role; }

    bool forwardsCorrections() const { return _connection.forwardsCorrections; }

    /// A configured base sends compact (MSM4) observations.
    bool compactObservations() const { return _connection.config.base.compactObservations; }

    /// The detected family's once an Automatic session detected it; a passive receiver's stays passive.
    int manufacturer() const { return gpsReceiverManufacturerForType(_family()); }

    /// The family an Automatic session detected, or the protocol a passive receiver's output carries: the family
    /// whose native protocol it is, or GPSType::Passive for standard NMEA. Empty until then and for a selected family.
    std::optional<GPSType> detectedType() const { return _detectedType; }

    /// Base mode of a configured base; empty for a passive receiver.
    std::optional<BaseModeDefinition::Mode> baseMode() const { return _base.mode; }

    /// What a configured base last reported of its base position; Inactive until it reports, and for a passive
    /// receiver.
    GPSBaseState baseState() const { return _base.state; }

    /// The receiver's corrections wait for a valid base position, which it does not report, so none are forwarded.
    /// Never for the families whose corrections do not wait for it (forwardsCorrectionsWithoutBase()).
    bool correctionsWithheld() const
    {
        return _base.state != GPSBaseState::Valid && !forwardsCorrectionsWithoutBase(_family());
    }

    const QString& serialDevice() const { return _connection.serialDevice; }

    const QString& endpoint() const { return _connection.endpoint; }

    const QString& link() const { return _connection.link; }

    /// What an Automatic session's detection looks for now. Empty until it starts looking, once it found the family,
    /// and for any other session.
    std::optional<GPSDetectionProgress> detecting() const { return _detecting; }

    /// What an Automatic session detected, once its receiver is ready, as the hint for the link's next session; no
    /// hint until then and for any other session.
    const GPSDetectionHint& detected() const { return _detected; }

    const QString& identity() const { return _identity; }

    /// A base with a known antenna position reports that position instead of its navigation solution.
    GPSObservation observation(const GPSPositionReport& report, quint64 receivedAtUs) const;

signals:
    void satelliteInfoUpdated(const GPSSatelliteReport& report);
    void positionUpdated(const GPSPositionReport& report);
    /// The base position status of a configured base, in every base mode.
    void baseStatusUpdated(const GPSBaseStatus& report);
    void receiverReady(const QString& identity);
    /// The family an Automatic session detected, or the protocol a passive receiver's output carries.
    void receiverDetected(GPSType type);
    void detectingChanged();
    /// Why a passive receiver delivers no position; None once positions arrive.
    void inputProblem(GPSInputProblem problem);
    /// The worker failed or exited; the owner ends the session.
    void ended(GPSConnectionError error, const QString& detail);

private:
    /// The selected family, or the detected one once an Automatic session detected it.
    GPSType _family() const
    {
        return _connection.type == GPSType::Automatic ? _detectedType.value_or(GPSType::Automatic) : _connection.type;
    }

    void _onBaseStatus(const GPSBaseStatus& report);
    void _onReady(const QString& identity, const GPSDetectionHint& detected);
    void _onDetecting(GPSType family, unsigned baud, bool remembered);
    void _onDetected(GPSType type);
    void _forwardCorrection(const QByteArray& data, qint64 receivedAtMs);

    const quint64 _sessionId;
    const Connection _connection;
    GPSBaseStationState _base;
    QPointer<GPSReceiverWorker> _worker;
    /// The receiver's registration as the local correction source.
    std::unique_ptr<GPSCorrectionSourceHandle> _correction;
    GPSPositionSourceRegistration _position;
    QString _identity;
    std::optional<GPSType> _detectedType;
    std::optional<GPSDetectionProgress> _detecting;
    GPSDetectionHint _detected;
    bool _retired = false;
};
