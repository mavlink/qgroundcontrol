#include "NMEAPositionSource.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include <QtCore/QIODevice>
#include <QtCore/QPointer>
#include <QtCore/QTimeZone>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"
#include "ReadTimestamp.h"

namespace {
constexpr int DEFAULT_REQUEST_TIMEOUT_MS = std::chrono::milliseconds(std::chrono::minutes(5)).count();
constexpr quint64 METADATA_MAX_AGE_US = std::chrono::microseconds(std::chrono::seconds(2)).count();
constexpr quint64 UNTIMED_METADATA_MAX_AGE_US = std::chrono::microseconds(std::chrono::seconds(1)).count();
constexpr qsizetype MAX_READ_BYTES_PER_TURN = 32 * 1024;
constexpr double USER_EQUIVALENT_RANGE_ERROR_METERS = 5.1;
constexpr double NMEA_ACCURACY_SCALE = 2.0;

QTime qtTime(int timeMs)
{
    return QTime::fromMSecsSinceStartOfDay(timeMs);
}

void setFiniteAttribute(QGeoPositionInfo& position, QGeoPositionInfo::Attribute attribute, double value)
{
    if (std::isfinite(value)) {
        position.setAttribute(attribute, value);
    } else {
        position.removeAttribute(attribute);
    }
}

void applyHorizontalFallback(QGeoPositionInfo& position, std::optional<double> dop)
{
    if (dop && *dop > 0.0) {
        position.setAttribute(QGeoPositionInfo::HorizontalAccuracy,
                              *dop * USER_EQUIVALENT_RANGE_ERROR_METERS * NMEA_ACCURACY_SCALE);
    } else {
        position.removeAttribute(QGeoPositionInfo::HorizontalAccuracy);
    }
}

void applyVerticalFallback(QGeoPositionInfo& position, std::optional<double> dop)
{
    if (dop && *dop >= 0.0) {
        position.setAttribute(QGeoPositionInfo::VerticalAccuracy,
                              *dop * USER_EQUIVALENT_RANGE_ERROR_METERS * NMEA_ACCURACY_SCALE);
    } else {
        position.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
    }
}
}  // namespace

QGC_LOGGING_CATEGORY(NMEAPositionSourceLog, "GPS.NMEA.NMEAPositionSource")

NMEAPositionSource::NMEAPositionSource(QIODevice* device, QObject* parent, RuntimeScheduler* scheduler)
    : QGeoPositionInfoSource(parent)
    , _device(device)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _requestTask(_scheduler, this)
    , _publicationTask(_scheduler, this)
    , _lossTask(_scheduler, this)
    , _errorTask(_scheduler, this)
    , _lineFramer(_sentenceBuffer)
    , _navigationAssembler({.metadataMaxAgeUs = METADATA_MAX_AGE_US,
                            .untimedMetadataMaxAgeUs = UNTIMED_METADATA_MAX_AGE_US - 1,
                            .autonomousFixQuality = GPSFixQuality::Unknown,
                            .useGsaDimensionForAutonomousFix = true,
                            .requirePositionTime = true,
                            .reconstructDate = true,
                            .enforceNavigationOrder = true,
                            .untimedMetadataUsesPositionReceipt = false})
{
    qCDebug(NMEAPositionSourceLog) << this;
    if (_device) {
        connect(_device, &QIODevice::readyRead, this, &NMEAPositionSource::_readAvailableData);
        connect(_device, &QIODevice::aboutToClose, this, &NMEAPositionSource::_closeInput);
        connect(_device, &QObject::destroyed, this, [this]() {
            _device = nullptr;
            _closeInput();
        });
    }
    _resetDecoder();
}

NMEAPositionSource::~NMEAPositionSource()
{
    qCDebug(NMEAPositionSourceLog) << this;
    _requestTask.cancel();
    _publicationTask.cancel();
    _lossTask.cancel();
    _errorTask.cancel();
}

void NMEAPositionSource::_resetDecoder()
{
    ++_generation;
    _requestTask.cancel();
    _publicationTask.cancel();
    _lossTask.cancel();
    _errorTask.cancel();
    _pendingLoss.reset();
    _pendingFix = {};
    _lastObservation = {};
    _lastKnownPosition = {};
    _publishedEpochs.clear();
    _lineFramer.reset();
    _navigationAssembler.reset();
    _sentenceTimestampUs = 0;
    _drainPending = false;
    _closed = !_device || !_device->isReadable();
    _error = NoError;
}

void NMEAPositionSource::_discardAvailableData()
{
    if (!_device || !_device->isReadable()) {
        return;
    }
    _lineFramer.reset();
    const QPointer<NMEAPositionSource> guard(this);
    while (_device && _device->isReadable() && _device->bytesAvailable() > 0) {
        const QByteArray data = _device->read(MAX_READ_BYTES_PER_TURN);
        if (data.isEmpty()) {
            return;
        }
        const quint64 receivedAtUs = ReadTimestamp::from(_device, _scheduler->nowUs());
        emit dataReceived(receivedAtUs);
        if (!guard || _closed) {
            return;
        }
    }
}

void NMEAPositionSource::_readAvailableData()
{
    if (_closed || _drainPending || (!_started && !_requestTask.active())) {
        return;
    }
    if (!_device || !_device->isReadable()) {
        _closeInput();
        return;
    }
    const QPointer<NMEAPositionSource> guard(this);
    qsizetype remaining = MAX_READ_BYTES_PER_TURN;
    while (remaining > 0 && _device && _device->isReadable() && _device->bytesAvailable() > 0) {
        const QByteArray data = _device->read(remaining);
        remaining -= data.size();
        if (data.isEmpty()) {
            return;
        }
        const quint64 receivedAtUs = ReadTimestamp::from(_device, _scheduler->nowUs());
        emit dataReceived(receivedAtUs);
        if (!guard || _closed) {
            return;
        }
        QList<NMEASentenceEnvelope> sentences;
        for (const char byte : data) {
            const auto framed = _lineFramer.addByte(static_cast<uint8_t>(byte));
            if (framed.started) {
                _sentenceTimestampUs = receivedAtUs;
            }
            if (framed.line) {
                const auto line = *framed.line;
                QByteArray bytes(line.data(), static_cast<qsizetype>(line.size()));
                bytes.append(framed.endedWithCarriageReturn ? "\r\n" : "\n");
                if (auto sentence = NMEASentenceEnvelope::parse(std::move(bytes), _sentenceTimestampUs)) {
                    sentences.append(std::move(*sentence));
                }
            }
        }
        for (const auto& sentence : sentences) {
            _processSentence(sentence);
            if (!guard || _closed) {
                return;
            }
            emit sentenceReceived(sentence);
            if (!guard || _closed) {
                return;
            }
        }
    }
    if (_device && _device->isReadable() && _device->bytesAvailable() > 0) {
        _drainPending = true;
        QMetaObject::invokeMethod(
            this,
            [this]() {
                _drainPending = false;
                _readAvailableData();
            },
            Qt::QueuedConnection);
    }
}

void NMEAPositionSource::_closeInput()
{
    const QPointer<NMEAPositionSource> guard(this);
    if (_closed) {
        return;
    }
    _closed = true;
    _lineFramer.reset();
    _publicationTask.cancel();
    _pendingFix = {};
    if (_started || _requestTask.active()) {
        _errorTask.cancel();
        _error = ClosedError;
        emit errorOccurred(_error);
        if (!guard) {
            return;
        }
    }
    if (guard) {
        emit closed();
    }
}

QDateTime NMEAPositionSource::_receiptTime(quint64 timestampUs) const
{
    const auto nowUs = _scheduler->nowUs();
    const auto ageMs = MonotonicClock::ageMilliseconds(timestampUs, nowUs);
    return ageMs >= 0 ? QDateTime::currentDateTimeUtc().addMSecs(-ageMs) : QDateTime();
}

GPSObservation NMEAPositionSource::_observation(const NMEA::NavigationEpoch& epoch, const QDateTime& receivedAt)
{
    GPSObservation observation;
    if (epoch.timeMs && epoch.date.isValid()) {
        observation.position.setTimestamp(QDateTime(epoch.date, qtTime(*epoch.timeMs), QTimeZone::UTC));
    }
    if (epoch.hasCoordinate()) {
        QGeoCoordinate coordinate(epoch.latitude, epoch.longitude);
        if (epoch.altitudeMslMeters) {
            coordinate.setAltitude(*epoch.altitudeMslMeters);
        }
        observation.position.setCoordinate(coordinate);
    }
    if (epoch.speedMetersPerSecond) {
        setFiniteAttribute(observation.position, QGeoPositionInfo::GroundSpeed, *epoch.speedMetersPerSecond);
    }
    if (epoch.courseDegrees) {
        setFiniteAttribute(observation.position, QGeoPositionInfo::Direction, *epoch.courseDegrees);
    }
    observation.receiverFixValid = true;
    observation.fixQuality = epoch.fixQuality;
    if (epoch.satellitesUsed) {
        observation.satellitesUsed = static_cast<int>(*epoch.satellitesUsed);
    }
    observation.horizontalDop = epoch.horizontalDop && *epoch.horizontalDop > 0.0 ? epoch.horizontalDop : std::nullopt;
    observation.verticalDop = epoch.verticalDop;
    observation.dopTimestampUs = epoch.dopReceivedAtUs;
    observation.accuracyTimestampUs = epoch.accuracyReceivedAtUs;
    if (epoch.horizontalAccuracyMeters) {
        setFiniteAttribute(observation.position, QGeoPositionInfo::HorizontalAccuracy, *epoch.horizontalAccuracyMeters);
    } else if (!epoch.accuracyReceivedAtUs) {
        applyHorizontalFallback(observation.position, observation.horizontalDop);
    } else {
        observation.position.removeAttribute(QGeoPositionInfo::HorizontalAccuracy);
    }
    if (epoch.verticalAccuracyMeters) {
        setFiniteAttribute(observation.position, QGeoPositionInfo::VerticalAccuracy, *epoch.verticalAccuracyMeters);
    } else if (!epoch.accuracyReceivedAtUs) {
        applyVerticalFallback(observation.position, observation.verticalDop);
    } else {
        observation.position.removeAttribute(QGeoPositionInfo::VerticalAccuracy);
    }
    observation.altitudeDatum = GPSAltitudeDatum::Unknown;
    observation.altitudeEllipsoidMeters.reset();
    if (epoch.altitudeMslMeters) {
        observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
        observation.altitudeEllipsoidMeters = epoch.altitudeEllipsoidMeters();
    }
    observation.monotonicTimestampUs = epoch.receivedAtUs;
    observation.sourceId = QStringLiteral("NMEA");
    observation.receivedAt = receivedAt;
    return observation;
}

GPSObservation NMEAPositionSource::_lossObservation(const NMEA::NavigationEpoch& epoch, const QDateTime& receivedAt)
{
    GPSObservation observation;
    if (epoch.timeMs && epoch.date.isValid()) {
        observation.position.setTimestamp(QDateTime(epoch.date, qtTime(*epoch.timeMs), QTimeZone::UTC));
    }
    observation.receiverFixValid = false;
    observation.fixQuality = GPSObservation::FixQuality::NoFix;
    observation.monotonicTimestampUs = epoch.receivedAtUs;
    observation.sourceId = QStringLiteral("NMEA");
    observation.receivedAt = receivedAt;
    return observation;
}

void NMEAPositionSource::_processSentence(const NMEASentenceEnvelope& envelope)
{
    const auto update = _navigationAssembler.ingest(envelope.sentence(), envelope.receivedAtUs());
    if (!update) {
        return;
    }
    if (update->type == NMEA::NavigationUpdate::Type::FixLoss) {
        _fixLost(_lossObservation(update->epoch, _receiptTime(update->epoch.receivedAtUs)));
        return;
    }
    _queueEpoch(update->epoch);
}

void NMEAPositionSource::_queueEpoch(const NMEA::NavigationEpoch& epoch)
{
    if (!epoch.timeMs || _publishedEpochs.value(*epoch.timeMs) == epoch.revision) {
        return;
    }

    GPSObservation observation = _observation(epoch, _receiptTime(epoch.receivedAtUs));
    if (!observation.position.isValid() || !observation.receiverFixValid.value_or(true) ||
        observation.fixQuality == GPSObservation::FixQuality::NoFix) {
        return;
    }

    if (_pendingFix.requested && _publicationTask.active() && _pendingFix.epochTimeMs &&
        *_pendingFix.epochTimeMs != *epoch.timeMs) {
        return;
    }
    _lastKnownPosition = observation.position;
    _pendingFix.requested |= _requestTask.active();
    _pendingFix.observation = observation;
    _pendingFix.position = observation.position;
    _pendingFix.epochTimeMs = epoch.timeMs;
    _pendingFix.epochRevision = epoch.revision;
    if (_pendingFix.requested) {
        _publicationTask.cancel();
    }
    _schedulePublication();
}

void NMEAPositionSource::_fixLost(const GPSObservation& observation)
{
    if (!_started && !_requestTask.active()) {
        return;
    }
    _publicationTask.cancel();
    _pendingFix = {};
    _pendingLoss = observation;
    if (!_lossTask.active()) {
        const auto generation = _generation;
        _lossTask.schedule(std::chrono::microseconds::zero(), [this, generation]() {
            if (generation == _generation) {
                _publishLoss();
            }
        });
    }
}

void NMEAPositionSource::_publishLoss()
{
    if (!_pendingLoss) {
        return;
    }
    const auto loss = *_pendingLoss;
    _pendingLoss.reset();
    _lastObservation = loss;
    emit observationReceived(loss);
}

void NMEAPositionSource::_schedulePublication()
{
    if (_publicationTask.active() || !_pendingFix.position) {
        return;
    }
    const auto generation = _generation;
    const auto delay = std::chrono::milliseconds(_pendingFix.requested ? 0 : updateInterval());
    _publicationTask.schedule(delay, [this, generation]() {
        if (generation == _generation) {
            _publishPending();
        }
    });
}

void NMEAPositionSource::_publishPending()
{
    if (_pendingLoss) {
        const QPointer<NMEAPositionSource> guard(this);
        const auto generation = _generation;
        _lossTask.cancel();
        _publishLoss();
        if (!guard || generation != _generation) {
            return;
        }
    }
    if (!_pendingFix.observation) {
        return;
    }
    const auto observation = *_pendingFix.observation;
    const bool requested = _pendingFix.requested;
    const auto epochTimeMs = _pendingFix.epochTimeMs;
    const auto epochRevision = _pendingFix.epochRevision;
    _pendingFix = {};
    if (!observation.receiverFixValid.value_or(true)) {
        return;
    }
    if (_started || requested) {
        _requestTask.cancel();
        _error = NoError;
        _lastObservation = observation;
        if (epochTimeMs) {
            _publishedEpochs[*epochTimeMs] = epochRevision;
        }
        const QPointer<NMEAPositionSource> guard(this);
        const auto generation = _generation;
        emit observationReceived(observation);
        if (guard && generation == _generation) {
            emit positionUpdated(observation.position);
        }
    }
}

void NMEAPositionSource::_publishError(Error error)
{
    const auto generation = _generation;
    _errorTask.schedule(std::chrono::microseconds::zero(), [this, generation, error]() {
        if (generation == _generation) {
            _error = error;
            emit errorOccurred(error);
        }
    });
}

void NMEAPositionSource::setUpdateInterval(int msec)
{
    QGeoPositionInfoSource::setUpdateInterval(msec == 0 ? 0 : (std::max) (msec, minimumUpdateInterval()));
    _publicationTask.cancel();
    _schedulePublication();
}

QGeoPositionInfo NMEAPositionSource::lastKnownPosition(bool /*satelliteOnly*/) const
{
    return _lastKnownPosition;
}

QGeoPositionInfoSource::PositioningMethods NMEAPositionSource::supportedPositioningMethods() const
{
    return SatellitePositioningMethods;
}

int NMEAPositionSource::minimumUpdateInterval() const
{
    return 0;
}

QGeoPositionInfoSource::Error NMEAPositionSource::error() const
{
    return _error;
}

void NMEAPositionSource::startUpdates()
{
    if (_started) {
        return;
    }
    const QPointer<NMEAPositionSource> guard(this);
    if (!_requestTask.active()) {
        _resetDecoder();
        _discardAvailableData();
        if (!guard) {
            return;
        }
    }
    if (_closed || !_device || !_device->isReadable()) {
        _publishError(ClosedError);
        return;
    }
    _started = true;
}

void NMEAPositionSource::stopUpdates()
{
    _started = false;
    if (!_requestTask.active()) {
        _lossTask.cancel();
        _pendingLoss.reset();
        if (!_pendingFix.requested) {
            _publicationTask.cancel();
            _pendingFix.position.reset();
            _pendingFix.observation.reset();
            _pendingFix.epochTimeMs.reset();
        }
    }
}

void NMEAPositionSource::requestUpdate(int timeout)
{
    if (_requestTask.active()) {
        return;
    }
    if (timeout < 0 || (timeout > 0 && timeout < minimumUpdateInterval())) {
        _publishError(UpdateTimeoutError);
        return;
    }
    if (!_device || !_device->isReadable()) {
        _publishError(ClosedError);
        return;
    }
    if (!_started) {
        _resetDecoder();
    }
    const auto generation = _generation;
    _error = NoError;
    _requestTask.schedule(std::chrono::milliseconds(timeout == 0 ? DEFAULT_REQUEST_TIMEOUT_MS : timeout),
                          [this, generation]() {
                              if (generation != _generation) {
                                  return;
                              }
                              _pendingFix.requested = false;
                              _error = UpdateTimeoutError;
                              emit errorOccurred(_error);
                          });
    if (!_started && _device && _device->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, &NMEAPositionSource::_readAvailableData, Qt::QueuedConnection);
    }
}
