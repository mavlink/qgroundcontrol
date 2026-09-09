#include "GPSBaseStationState.h"

#include <QtCore/QPointer>

#include <array>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSBaseStationStateLog, "GPS.BaseStation.GPSBaseStationState")

GPSBaseStationState::GPSBaseStationState(GPSReceiverSession& session, GPSBaseStationFactGroup& facts, QObject* parent)
    : QObject(parent)
    , _session(session)
    , _facts(facts)
{
    qCDebug(GPSBaseStationStateLog) << this;
    connect(&_session, &GPSReceiverSession::surveyInReceived, this, &GPSBaseStationState::_updateSurvey);
    connect(&_session, &GPSReceiverSession::disconnected, this, &GPSBaseStationState::_reset);
    connect(&_session, &GPSReceiverSession::receiverTypeChanged, this, &GPSBaseStationState::_reset);
    connect(&_session, &GPSReceiverSession::capabilitiesUpdated, this, [this]() {
        if (!_acceptsSurvey()) {
            _reset();
        }
    });
}

GPSBaseStationState::~GPSBaseStationState()
{
    qCDebug(GPSBaseStationStateLog) << this;
    _session.disconnect(this);
}

bool GPSBaseStationState::_acceptsSurvey() const
{
    return _session.hasReceiver() && _session.config().role == GPSReceiverConfig::Role::RTKBase &&
           _session.capabilities().rtkBase != GPSReceiverCapabilities::Support::Unsupported;
}

void GPSBaseStationState::_updateSurvey(const GPSSurveyInStatus& status)
{
    if (!_acceptsSurvey() || (status.sessionId != 0 && status.sessionId != _session.sessionId())) {
        return;
    }
    const QPointer<GPSBaseStationState> guard(this);
    const quint64 revision = ++_revision;
    const quint64 sessionId = _session.sessionId();
    GPSBaseReference reference;
    reference.valid = status.valid;
    reference.observation.position = QGeoPositionInfo(
        QGeoCoordinate(status.latitude, status.longitude, status.altitude), QDateTime::currentDateTimeUtc());
    reference.observation.sessionId = sessionId;
    reference.observation.monotonicTimestampUs = status.monotonicTimestampUs;
    reference.observation.altitudeDatum = status.altitudeDatum;
    reference.observation.sourceId = QStringLiteral("RTK Base");
    reference.observation.fixQuality =
        status.valid ? GPSObservation::FixQuality::Unknown : GPSObservation::FixQuality::NoFix;
    if (status.meanAccuracyMM) {
        reference.accuracyMeters = *status.meanAccuracyMM / 1000.0;
    }
    _reference = reference;
    emit referenceChanged();
    if (!guard || _revision != revision || _session.sessionId() != sessionId) {
        return;
    }
    const std::array<std::pair<Fact*, QVariant>, 7> values = {{
        {_facts.currentDuration(), status.durationSecs},
        {_facts.currentAccuracy(), reference.accuracyMeters.value_or(qQNaN())},
        {_facts.currentLatitude(), status.latitude},
        {_facts.currentLongitude(), status.longitude},
        {_facts.currentAltitude(), status.altitude},
        {_facts.valid(), status.valid},
        {_facts.active(), status.active},
    }};
    for (const auto& [fact, value] : values) {
        if (!guard || _revision != revision || _session.sessionId() != sessionId || !_acceptsSurvey()) {
            return;
        }
        fact->setRawValue(value);
    }
}

void GPSBaseStationState::_reset()
{
    const QPointer<GPSBaseStationState> guard(this);
    const quint64 revision = ++_revision;
    _reference = {};
    emit referenceChanged();
    if (!guard || _revision != revision) {
        return;
    }
    const std::array<std::pair<Fact*, QVariant>, 7> values = {{
        {_facts.valid(), false},
        {_facts.active(), false},
        {_facts.currentDuration(), 0},
        {_facts.currentAccuracy(), qQNaN()},
        {_facts.currentLatitude(), qQNaN()},
        {_facts.currentLongitude(), qQNaN()},
        {_facts.currentAltitude(), qQNaN()},
    }};
    for (const auto& [fact, value] : values) {
        if (!guard || _revision != revision) {
            return;
        }
        fact->setRawValue(value);
    }
}
