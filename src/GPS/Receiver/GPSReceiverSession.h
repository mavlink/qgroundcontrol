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
#include "GPSReceiverReports.h"
#include "GPSReceiverWorker.h"
#include "PositionManager.h"

class GPSCorrectionManager;
class GPSCorrectionSourceHandle;
class GPSSourceHealth;

/// A receiver session's base-station mode and the antenna position it reports.
struct GPSBaseStationState
{
    /// Base mode of a configured base; empty for a passive receiver.
    std::optional<BaseModeDefinition::Mode> mode;
    /// Antenna position and its accuracy in meters once known; the receiver then reports only a time fix.
    std::optional<std::pair<GPSEllipsoidPosition, double>> position;
    /// Assumed accuracy of a survey-in result that reports none.
    std::optional<double> surveyAccuracyLimitMeters;

    /// A surveying base adopts a valid, located result and otherwise has no position.
    void applySurvey(const GPSSurveyReport& report);
};

/// Builds the receiver request for one connection: a passive receiver gets no base settings, and compact
/// observations apply only where the family supports them. @a error, when given, receives the translated diagnostic,
/// which is empty when the receiver supports the request; the request is returned either way.
[[nodiscard]] GPSReceiverConfig gpsReceiverConfigFor(const GPSBaseStationConfig& base, GPSType type, uint32_t baudRate,
                                                     bool allowPersistentChanges, QString* error);

/// The state a session starts from; a fixed base knows its position before the receiver reports.
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
        GPSType type = GPSType::ublox;
        RTKSettings::ReceiverRole role = RTKSettings::ConfiguredBase;
        /// The receiver's RTCM output is a correction source.
        bool forwardsCorrections = true;
        GPSReceiverConfig config{};
        /// Serial device whose removal ends the session; empty for network receivers.
        QString serialDevice;
        /// Serial device or network address shown to the user.
        QString endpoint;
    };

    GPSReceiverSession(quint64 id, Connection connection, QObject* parent);
    ~GPSReceiverSession() override;

    /// Forwards the receiver's RTCM as the local correction source when the connection asks for it.
    void registerCorrections(GPSCorrectionManager* manager, const QString& sourceInstance);
    /// Creates the worker, with a real worker for an empty @a factory, and starts it. False when none is created.
    bool start(const GPSReceiverWorkerFactory& factory, GPSReceiverWorker::TransportFactory transportFactory);
    /// Publishes @a producer as the receiver position source. Registration observers may retire this session.
    void registerPosition(PositionManager* positionManager, GPSSourceHealth* producer);
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
    int manufacturer() const
    {
        return gpsReceiverManufacturerForType(
            _connection.type == GPSType::automatic ? _detectedType.value_or(GPSType::automatic) : _connection.type);
    }

    /// The family an Automatic session detected, or the protocol a passive receiver's output carries: the family
    /// whose native protocol it is, or GPSType::passive for standard NMEA. Empty until then and for a selected family.
    std::optional<GPSType> detectedType() const { return _detectedType; }

    /// BaseModeDefinition::Mode of a configured base, or -1 for a passive receiver.
    int baseMode() const { return _base.mode ? static_cast<int>(*_base.mode) : -1; }

    const QString& serialDevice() const { return _connection.serialDevice; }

    const QString& endpoint() const { return _connection.endpoint; }

    const QString& identity() const { return _identity; }

    /// A base with a known antenna position reports that position instead of its navigation solution.
    GPSObservation observation(const GPSPositionReport& report, quint64 receivedAtUs) const;

signals:
    void satelliteInfoUpdated(const GPSSatelliteReport& report);
    void positionUpdated(const GPSPositionReport& report);
    /// Survey progress of a configured base.
    void surveyInStatusUpdated(const GPSSurveyReport& report);
    void receiverReady(const QString& identity);
    /// The family an Automatic session detected, or the protocol a passive receiver's output carries.
    void receiverDetected(GPSType type);
    /// Why a passive receiver delivers no position; None once positions arrive.
    void inputProblem(GPSInputProblem problem);
    /// The worker failed or exited; the owner ends the session.
    void ended(GPSConnectionError error, const QString& detail);

private:
    void _onSurvey(const GPSSurveyReport& report);
    void _onReady(const QString& identity);
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
    bool _retired = false;
};
