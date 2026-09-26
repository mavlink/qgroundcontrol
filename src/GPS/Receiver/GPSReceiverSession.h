#pragma once

#include <optional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSBaseStationSettings.h"
#include "GPSCorrectionSourceRegistration.h"
#include "GPSObservation.h"
#include "GPSPositionSourceRegistration.h"
#include "GPSProvider.h"
#include "GPSReceiver.h"
#include "GPSReceiverConfig.h"
#include "GPSReceiverReports.h"

class GPSCorrectionManager;
class GPSPositionService;
class GPSSourceHealth;

/// One receiver connection of GPSReceiver: its worker provider, correction and position registrations, and identity.
/// Provider signals are queued to the session and relayed only while it is current. retire() disconnects them, but
/// Qt still delivers calls it had already queued, so a retired session drops those itself.
class GPSReceiverSession : public QObject
{
    Q_OBJECT

public:
    struct Connection
    {
        GPSType type = GPSType::ublox;
        GPSReceiver::ReceiverRole role = GPSReceiver::ConfiguredBase;
        GPSReceiverConfig config{};
        /// Serial device whose removal ends the session; empty for network receivers.
        QString serialDevice;
        /// Serial device or network address shown to the user.
        QString endpoint;
    };

    GPSReceiverSession(quint64 id, Connection connection, QObject* parent);
    ~GPSReceiverSession() override;

    /// Forwards the receiver's RTCM as the local correction source unless the role is PositionOnly. Registration
    /// observers may reenter the owner.
    void registerCorrections(GPSCorrectionManager* manager, const QString& sourceInstance);
    /// Creates the provider, with a real worker for an empty @a factory, and starts it. False when none is created.
    bool start(const GPSReceiver::ProviderFactory& factory, GPSProvider::TransportFactory transportFactory);
    /// Publishes @a producer as the receiver position source. Registration observers may retire this session.
    void registerPosition(GPSPositionService* service, GPSSourceHealth* producer);
    /// Disconnects and stops the provider and ends both registrations. A started worker keeps its transport, and so
    /// any serial reservation, until it exits and deletes itself; a provider that never started is deleted now.
    void retire();

    bool hasProvider() const { return !_provider.isNull(); }

    GPSReceiver::ReceiverRole role() const { return _connection.role; }

    /// The detected family's once an Automatic session detected it.
    int manufacturer() const { return GPSReceiver::manufacturerForType(_detectedType.value_or(_connection.type)); }

    /// The family an Automatic session detected; empty before detection and for a selected family.
    std::optional<GPSType> detectedType() const { return _detectedType; }

    /// GPSBaseStationSettings::Mode of a configured base, or -1 for a passive receiver.
    int baseMode() const { return _base.mode ? static_cast<int>(*_base.mode) : -1; }

    const QString& serialDevice() const { return _connection.serialDevice; }

    const QString& endpoint() const { return _connection.endpoint; }

    const QString& identity() const { return _identity; }

    /// A base with a known antenna position reports that position instead of its navigation solution.
    GPSObservation observation(const GPSPositionReport& report) const;

signals:
    void satelliteInfoUpdate(const GPSSatelliteReport& report);
    void positionUpdate(const GPSPositionReport& report);
    /// Survey progress of a configured base.
    void surveyInStatus(const GPSSurveyReport& report);
    void receiverReady(const QString& identity);
    void receiverDetected(GPSType type);
    /// The provider failed or exited; the owner ends the session.
    void ended(GPSConnectionError error, const QString& detail);

private:
    /// Not retired, and its correction registration, if it made one, is still the router's current one.
    bool _current() const { return !_retired && (!_registered || _corrections.valid()); }

    void _onSurvey(const GPSSurveyReport& report);
    void _onReady(const QString& identity);
    void _onDetected(GPSType type);
    void _forwardCorrection(const QByteArray& data, qint64 receivedAtMs);

    const quint64 _sessionId;
    const Connection _connection;
    GPSBaseStationState _base;
    QPointer<GPSProvider> _provider;
    QPointer<GPSCorrectionManager> _correctionManager;
    GPSCorrectionSourceRegistration _corrections;
    GPSPositionSourceRegistration _position;
    QString _identity;
    std::optional<GPSType> _detectedType;
    bool _registered = false;
    bool _started = false;
    bool _retired = false;
};
