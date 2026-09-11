#include "GPSReceiverState.h"

#include <QtCore/QPointer>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSReceiverStateLog, "GPS.Receiver.GPSReceiverState")

GPSReceiverState::GPSReceiverState(GPSReceiverSession& session, QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _session(session)
    , _health(this, scheduler)
    , _satellites(this, GPSSourceHealth::FRESHNESS_TIMEOUT_MS, scheduler)
    , _relativePosition(this, GPSSourceHealth::FRESHNESS_TIMEOUT_MS, scheduler)
    , _integrity(this, scheduler)
{
    qCDebug(GPSReceiverStateLog) << this;
    connect(&_satellites, &GPSSatelliteStore::observationChanged, &_health,
            &GPSSourceHealth::applySatelliteObservation);
    connect(&_session, &GPSReceiverSession::attemptChanged, this, &GPSReceiverState::_attemptChanged);
    connect(&_session, &GPSReceiverSession::positionReceived, this, &GPSReceiverState::updatePosition);
    connect(&_session, &GPSReceiverSession::satellitesReceived, &_satellites, &GPSSatelliteStore::updateObservation);
    connect(&_session, &GPSReceiverSession::relativePositionReceived, &_relativePosition,
            &GPSRelativePositionStore::updateObservation);
    connect(&_session, &GPSReceiverSession::integrityReceived, this,
            [this](const GPSIntegrityObservation& observation) {
                if (_session.hasReceiver() && observation.sessionId == _session.sessionId()) {
                    _integrity.updateObservation(observation);
                }
            });
    connect(&_session, &GPSReceiverSession::surveyInReceived, this, &GPSReceiverState::_updateSurvey);
    connect(&_session, &GPSReceiverSession::disconnected, this, &GPSReceiverState::_resetReference);
    connect(&_session, &GPSReceiverSession::receiverTypeChanged, this, &GPSReceiverState::_resetReference);
    connect(&_session, &GPSReceiverSession::capabilitiesUpdated, this, [this]() {
        if (!_acceptsSurvey()) {
            _resetReference();
        }
    });
    _satellites.beginSession(QStringLiteral("nativeReceiver"), _session.sessionId());
    _relativePosition.beginSession(QStringLiteral("nativeReceiver"), _session.sessionId());
    _integrity.beginSession(_session.sessionId());
}

GPSReceiverState::~GPSReceiverState()
{
    qCDebug(GPSReceiverStateLog) << this;
    _session.disconnect(this);
}

void GPSReceiverState::_attemptChanged(const GPSReceiverAttempt& attempt)
{
    if (attempt.terminal() || attempt.phase == GPSReceiverAttempt::Phase::Connecting) {
        const QPointer<GPSReceiverState> guard(this);
        const auto generation = attempt.generation;
        reset();
        if (!guard || _session.sessionId() != generation) {
            return;
        }
        if (attempt.phase == GPSReceiverAttempt::Phase::Connecting) {
            _satellites.beginSession(QStringLiteral("nativeReceiver"), generation);
            if (!guard || _session.sessionId() != generation) {
                return;
            }
            _relativePosition.beginSession(QStringLiteral("nativeReceiver"), generation);
            if (guard && _session.sessionId() == generation) {
                _integrity.beginSession(generation);
            }
        }
    }
}

void GPSReceiverState::updatePosition(const GPSObservation& observation)
{
    if (!_session.attempt().ready() || (observation.sessionId && observation.sessionId != _session.sessionId())) {
        return;
    }
    _health.updateObservation(observation);
}

void GPSReceiverState::reset()
{
    const QPointer<GPSReceiverState> guard(this);
    const auto revision = ++_revision;
    const auto current = [&]() { return guard && revision == _revision; };
    _health.reset();
    if (!current()) {
        return;
    }
    _satellites.clear();
    if (!current()) {
        return;
    }
    _relativePosition.reset();
    if (!current()) {
        return;
    }
    _integrity.reset();
    if (current()) {
        _resetReference();
    }
}

bool GPSReceiverState::_acceptsSurvey() const
{
    return _session.hasReceiver() && _session.config().role == GPSReceiverConfig::Role::RTKBase &&
           _session.capabilities().rtkBase != GPSReceiverCapabilities::Support::Unsupported;
}

void GPSReceiverState::_updateSurvey(const GPSSurveyInStatus& status)
{
    if (!_acceptsSurvey() || (status.sessionId && status.sessionId != _session.sessionId())) {
        return;
    }
    ++_revision;
    _survey = status;
    _reference = {};
    _reference.valid = status.valid;
    _reference.observation.position = QGeoPositionInfo(
        QGeoCoordinate(status.latitude, status.longitude, status.altitude), QDateTime::currentDateTimeUtc());
    _reference.observation.sessionId = _session.sessionId();
    _reference.observation.monotonicTimestampUs = status.monotonicTimestampUs;
    _reference.observation.altitudeDatum = status.altitudeDatum;
    _reference.observation.sourceId = QStringLiteral("RTK Base");
    _reference.observation.fixQuality =
        status.valid ? GPSObservation::FixQuality::Unknown : GPSObservation::FixQuality::NoFix;
    if (status.meanAccuracyMM) {
        _reference.accuracyMeters = *status.meanAccuracyMM / 1000.0;
    }
    emit referenceChanged();
}

void GPSReceiverState::_resetReference()
{
    ++_revision;
    _reference = {};
    _survey = {};
    emit referenceChanged();
}
