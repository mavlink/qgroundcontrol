#include "NMEAPositionSource.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include <QtCore/QIODevice>
#include <QtCore/QPointer>
#include <QtCore/QTimeZone>

#include "MonotonicClock.h"
#include "NMEAMetadata.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"
#include "ReadTimestamp.h"

namespace {
constexpr int DEFAULT_REQUEST_TIMEOUT_MS = std::chrono::milliseconds(std::chrono::minutes(5)).count();
constexpr quint64 METADATA_MAX_AGE_US = std::chrono::microseconds(std::chrono::seconds(2)).count();
constexpr quint64 UNTIMED_METADATA_MAX_AGE_US = std::chrono::microseconds(std::chrono::seconds(1)).count();
constexpr qsizetype MAX_RETAINED_EPOCHS = 32;
constexpr qsizetype MAX_READ_BYTES_PER_TURN = 32 * 1024;
constexpr qsizetype MAX_SENTENCE_BYTES = 1024;
constexpr double USER_EQUIVALENT_RANGE_ERROR_METERS = 5.1;
constexpr double NMEA_ACCURACY_SCALE = 2.0;
constexpr int HALF_DAY_MS = std::chrono::milliseconds(std::chrono::hours(12)).count();
constexpr int DAY_MS = std::chrono::milliseconds(std::chrono::days(1)).count();

QDate qtDate(const NMEA::UtcDate& date)
{
    return QDate(date.year, static_cast<int>(date.month), static_cast<int>(date.day));
}

QTime qtTime(int timeMs)
{
    return QTime::fromMSecsSinceStartOfDay(timeMs);
}

std::optional<double> nonnegativeNumber(std::string_view field)
{
    const auto value = NMEA::number<double>(field);
    return value && *value >= 0.0 ? value : std::nullopt;
}

void setFiniteAttribute(QGeoPositionInfo& position, QGeoPositionInfo::Attribute attribute, double value)
{
    if (std::isfinite(value)) {
        position.setAttribute(attribute, value);
    } else {
        position.removeAttribute(attribute);
    }
}

void mergeCoordinate(QGeoPositionInfo& position, double latitude, double longitude, std::optional<double> altitude)
{
    QGeoCoordinate coordinate(latitude, longitude);
    if (!coordinate.isValid()) {
        return;
    }
    if (altitude && std::isfinite(*altitude)) {
        coordinate.setAltitude(*altitude);
    } else if (const auto previous = position.coordinate();
               previous.type() == QGeoCoordinate::Coordinate3D && std::isfinite(previous.altitude())) {
        coordinate.setAltitude(previous.altitude());
    }
    position.setCoordinate(coordinate);
}

void markReceipt(GPSObservation& observation, quint64 receivedAtUs)
{
    observation.monotonicTimestampUs =
        observation.monotonicTimestampUs == 0 ? receivedAtUs : std::min(observation.monotonicTimestampUs, receivedAtUs);
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
    _epochs.clear();
    _currentEpochMs.reset();
    _dateReference = {};
    _dateReferenceTimeMs.reset();
    _statusReceiptUs = 0;
    _statusTimeMs.reset();
    _statusDate = {};
    _sentenceSequence = 0;
    _invalidThroughSequence = 0;
    _navigationValid = true;
    _sentence.clear();
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
    _sentence.clear();
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
            if (byte == '$') {
                _sentence = "$";
                _sentenceTimestampUs = receivedAtUs;
            } else if (!_sentence.isEmpty()) {
                _sentence.append(byte);
                if (byte == '\n') {
                    if (auto sentence = NMEASentenceEnvelope::parse(_sentence, _sentenceTimestampUs)) {
                        sentences.append(std::move(*sentence));
                    }
                    _sentence.clear();
                } else if (_sentence.size() > MAX_SENTENCE_BYTES || (byte != '\r' && (byte < ' ' || byte > '~'))) {
                    _sentence.clear();
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
    _sentence.clear();
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

QDate NMEAPositionSource::_dateForTime(int timeMs) const
{
    if (!_dateReference.isValid() || !_dateReferenceTimeMs) {
        return {};
    }
    if (*_dateReferenceTimeMs - timeMs < -HALF_DAY_MS) {
        return _dateReference.addDays(-1);
    }
    if (*_dateReferenceTimeMs - timeMs > HALF_DAY_MS) {
        return _dateReference.addDays(1);
    }
    return _dateReference;
}

void NMEAPositionSource::_setDateReference(const QDate& date, int timeMs)
{
    if (!date.isValid()) {
        return;
    }
    _dateReference = date;
    _dateReferenceTimeMs = timeMs;
}

void NMEAPositionSource::_handleDatedSentence(const NMEASentenceEnvelope& envelope)
{
    const auto dated = NMEA::zda(envelope.sentence());
    if (!dated || !dated->utcMilliseconds) {
        return;
    }
    const QDate date = qtDate(dated->date);
    if (!_acceptNavigationStatus(dated->utcMilliseconds, date, envelope.receivedAtUs())) {
        return;
    }
    _setDateReference(date, *dated->utcMilliseconds);
    if (auto epoch = _epochs.find(*dated->utcMilliseconds); epoch != _epochs.end()) {
        epoch->observation.position.setTimestamp(QDateTime(date, qtTime(*dated->utcMilliseconds), QTimeZone::UTC));
        _queueEpoch(*dated->utcMilliseconds);
    }
}

QDateTime NMEAPositionSource::_timestamp(int timeMs) const
{
    const QDate date = _dateForTime(timeMs);
    return date.isValid() ? QDateTime(date, qtTime(timeMs), QTimeZone::UTC) : QDateTime();
}

QDateTime NMEAPositionSource::_receiptTime(quint64 timestampUs) const
{
    const auto nowUs = _scheduler->nowUs();
    const auto ageMs = MonotonicClock::ageMilliseconds(timestampUs, nowUs);
    return ageMs >= 0 ? QDateTime::currentDateTimeUtc().addMSecs(-ageMs) : QDateTime();
}

bool NMEAPositionSource::_acceptNavigationStatus(std::optional<int> timeMs, const QDate& date, quint64 receivedAtUs)
{
    if (!receivedAtUs || receivedAtUs < _statusReceiptUs) {
        return false;
    }
    if (timeMs && _statusTimeMs) {
        if (date.isValid() && _statusDate.isValid()) {
            if (QDateTime(date, qtTime(*timeMs), QTimeZone::UTC) <
                QDateTime(_statusDate, qtTime(*_statusTimeMs), QTimeZone::UTC)) {
                return false;
            }
        } else {
            int difference = *_statusTimeMs - *timeMs;
            if (difference < -HALF_DAY_MS) {
                difference += DAY_MS;
            } else if (difference > HALF_DAY_MS) {
                difference -= DAY_MS;
            }
            if (difference > 0) {
                return false;
            }
        }
    }
    _statusReceiptUs = receivedAtUs;
    if (timeMs) {
        _statusTimeMs = timeMs;
        _statusDate = date;
    }
    return true;
}

void NMEAPositionSource::_resetExpiredEpoch(EpochMetadata& epoch, const QDateTime& timestamp, quint64 receivedAtUs)
{
    const bool expired = epoch.observation.monotonicTimestampUs &&
                         receivedAtUs > epoch.observation.monotonicTimestampUs &&
                         !NMEA::freshAt(epoch.observation.monotonicTimestampUs, receivedAtUs, METADATA_MAX_AGE_US);
    const bool dateChanged = timestamp.date().isValid() && epoch.observation.position.timestamp().date().isValid() &&
                             epoch.observation.position.timestamp().date() != timestamp.date();
    if (!expired && !dateChanged) {
        return;
    }
    const auto navigationSequence = epoch.navigationSequence;
    epoch = {};
    epoch.navigationSequence = navigationSequence;
}

GPSObservation::FixQuality NMEAPositionSource::_fixQuality(const EpochMetadata& epoch)
{
    using Quality = GPSObservation::FixQuality;
    const auto autonomous = epoch.dimension == NMEA::FixDimension::TWO_D     ? Quality::Fix2D
                            : epoch.dimension == NMEA::FixDimension::THREE_D ? Quality::Fix3D
                                                                             : Quality::Unknown;
    return NMEA::fixQuality(epoch.ggaQuality.value_or(NMEA::GgaQuality::GPS), autonomous);
}

void NMEAPositionSource::_processSentence(const NMEASentenceEnvelope& envelope)
{
    const auto& sentence = envelope.sentence();
    const auto type = sentence.type();
    const quint64 receivedAtUs = envelope.receivedAtUs();
    const quint64 sequence = ++_sentenceSequence;

    if (type == "ZDA") {
        _handleDatedSentence(envelope);
        return;
    }

    if (const auto navigation = NMEA::navigationStatus(sentence)) {
        QDate date;
        if (navigation->utcMilliseconds) {
            if (type == "RMC") {
                if (const auto dateField = NMEA::rmcDate(sentence.fields[NMEA::Field::RMC_DATE])) {
                    date = qtDate(*dateField);
                }
            }
            if (!date.isValid()) {
                date = _dateForTime(*navigation->utcMilliseconds);
            }
        }
        if (!_acceptNavigationStatus(navigation->utcMilliseconds, date, receivedAtUs)) {
            return;
        }
        if (navigation->utcMilliseconds && date.isValid() && type == "RMC") {
            _setDateReference(date, *navigation->utcMilliseconds);
        }
        if (!navigation->valid) {
            _navigationValid = false;
            _invalidThroughSequence = sequence;
            GPSObservation loss;
            if (navigation->utcMilliseconds && date.isValid()) {
                loss.position.setTimestamp(QDateTime(date, qtTime(*navigation->utcMilliseconds), QTimeZone::UTC));
            }
            loss.receiverFixValid = false;
            loss.fixQuality = GPSObservation::FixQuality::NoFix;
            loss.monotonicTimestampUs = receivedAtUs;
            loss.sourceId = QStringLiteral("NMEA");
            loss.receivedAt = _receiptTime(receivedAtUs);
            _fixLost(std::move(loss));
            return;
        }
        _navigationValid = true;
    }

    _handlePositionSentence(envelope);
    _handleAccuracy(envelope);
    _handleUntimedMetadata(envelope);
}

void NMEAPositionSource::_handlePositionSentence(const NMEASentenceEnvelope& envelope)
{
    const auto& sentence = envelope.sentence();
    const quint64 receivedAtUs = envelope.receivedAtUs();
    std::optional<int> timeMs;

    if (const auto rmc = NMEA::rmc(sentence)) {
        if (!rmc->utcMilliseconds) {
            return;
        }
        timeMs = rmc->utcMilliseconds;
        if (rmc->date) {
            _setDateReference(qtDate(*rmc->date), *timeMs);
        }
        auto& epoch = _epochs[*timeMs];
        const QDateTime timestamp = _timestamp(*timeMs);
        _resetExpiredEpoch(epoch, timestamp, receivedAtUs);
        epoch.navigationSequence = _sentenceSequence;
        auto& observation = epoch.observation;
        observation.position.setTimestamp(timestamp);
        mergeCoordinate(observation.position, rmc->latitude, rmc->longitude, std::nullopt);
        setFiniteAttribute(observation.position, QGeoPositionInfo::GroundSpeed, rmc->speedMetersPerSecond);
        setFiniteAttribute(observation.position, QGeoPositionInfo::Direction, rmc->courseDegrees);
        observation.receiverFixValid = true;
        observation.fixQuality = _fixQuality(epoch);
        markReceipt(observation, receivedAtUs);
        _currentEpochMs = timeMs;
    } else if (const auto fix = NMEA::gga(sentence)) {
        timeMs = NMEA::utcMilliseconds(sentence.fields[NMEA::Field::UTC_TIME]);
        if (!timeMs || fix->quality == NMEA::GgaQuality::INVALID) {
            return;
        }
        auto& epoch = _epochs[*timeMs];
        const QDateTime timestamp = _timestamp(*timeMs);
        _resetExpiredEpoch(epoch, timestamp, receivedAtUs);
        epoch.navigationSequence = _sentenceSequence;
        epoch.ggaQuality = fix->quality;
        auto& observation = epoch.observation;
        observation.position.setTimestamp(timestamp);
        mergeCoordinate(observation.position, fix->latitude, fix->longitude,
                        std::isfinite(fix->altitude) ? std::optional<double>(fix->altitude) : std::nullopt);
        observation.receiverFixValid = true;
        observation.fixQuality = _fixQuality(epoch);
        observation.satellitesUsed = fix->satellitesUsed;
        observation.horizontalDop =
            std::isfinite(fix->hdop) && fix->hdop > 0.0 ? std::optional<double>(fix->hdop) : std::nullopt;
        observation.dopTimestampUs = receivedAtUs;
        observation.altitudeEllipsoidMeters.reset();
        observation.altitudeDatum = GPSAltitudeDatum::Unknown;
        if (std::isfinite(fix->altitude)) {
            observation.altitudeDatum = GPSAltitudeDatum::MeanSeaLevel;
            if (std::isfinite(fix->geoidSeparation)) {
                observation.altitudeEllipsoidMeters = fix->altitude + fix->geoidSeparation;
            }
        }
        if (!observation.accuracyTimestampUs ||
            !NMEA::freshAt(observation.accuracyTimestampUs, receivedAtUs, METADATA_MAX_AGE_US)) {
            observation.accuracyTimestampUs = 0;
            applyHorizontalFallback(observation.position, observation.horizontalDop);
            applyVerticalFallback(observation.position, observation.verticalDop);
        }
        markReceipt(observation, receivedAtUs);
        _currentEpochMs = timeMs;
    } else if (const auto gll = NMEA::gll(sentence)) {
        if (!gll->utcMilliseconds) {
            return;
        }
        timeMs = gll->utcMilliseconds;
        auto& epoch = _epochs[*timeMs];
        const QDateTime timestamp = _timestamp(*timeMs);
        _resetExpiredEpoch(epoch, timestamp, receivedAtUs);
        epoch.navigationSequence = _sentenceSequence;
        auto& observation = epoch.observation;
        observation.position.setTimestamp(timestamp);
        mergeCoordinate(observation.position, gll->latitude, gll->longitude, std::nullopt);
        observation.receiverFixValid = true;
        observation.fixQuality = _fixQuality(epoch);
        markReceipt(observation, receivedAtUs);
        _currentEpochMs = timeMs;
    } else {
        return;
    }

    _queueEpoch(*timeMs);
    _trimEpochs();
}

void NMEAPositionSource::_handleAccuracy(const NMEASentenceEnvelope& envelope)
{
    const auto accuracy = NMEA::gst(envelope.sentence());
    if (!accuracy) {
        return;
    }
    const auto timeMs = NMEA::utcMilliseconds(envelope.sentence().fields[NMEA::Field::UTC_TIME]);
    if (!timeMs) {
        return;
    }
    auto& epoch = _epochs[*timeMs];
    const QDateTime timestamp = _timestamp(*timeMs);
    _resetExpiredEpoch(epoch, timestamp, envelope.receivedAtUs());
    auto& observation = epoch.observation;
    if (timestamp.isValid()) {
        observation.position.setTimestamp(timestamp);
    }
    setFiniteAttribute(observation.position, QGeoPositionInfo::HorizontalAccuracy, accuracy->horizontalAccuracy);
    setFiniteAttribute(observation.position, QGeoPositionInfo::VerticalAccuracy, accuracy->verticalAccuracy);
    observation.accuracyTimestampUs = envelope.receivedAtUs();
    markReceipt(observation, envelope.receivedAtUs());
    if (_currentEpochMs == timeMs) {
        _queueEpoch(*timeMs);
    }
    _trimEpochs();
}

void NMEAPositionSource::_handleUntimedMetadata(const NMEASentenceEnvelope& envelope)
{
    if (!_currentEpochMs) {
        return;
    }
    auto epoch = _epochs.find(*_currentEpochMs);
    if (epoch == _epochs.end() || !NMEA::freshAt(epoch->observation.monotonicTimestampUs, envelope.receivedAtUs(),
                                                 UNTIMED_METADATA_MAX_AGE_US - 1)) {
        return;
    }

    const auto& sentence = envelope.sentence();
    const auto type = sentence.type();
    auto& observation = epoch->observation;
    if (type == "GSA") {
        if (sentence.count < NMEA::Field::GSA_MIN_FIELDS) {
            return;
        }
        const auto hdop = nonnegativeNumber(sentence.fields[NMEA::Field::GSA_HDOP]);
        const auto vdop = nonnegativeNumber(sentence.fields[NMEA::Field::GSA_VDOP]);
        if (!observation.horizontalDop) {
            observation.horizontalDop = hdop;
        }
        observation.verticalDop = vdop;
        observation.dopTimestampUs = envelope.receivedAtUs();
        epoch->dimension = NMEA::number<unsigned>(sentence.fields[NMEA::Field::GSA_DIMENSION]);
        observation.fixQuality = _fixQuality(*epoch);
        if (!observation.accuracyTimestampUs) {
            applyHorizontalFallback(observation.position, hdop);
            applyVerticalFallback(observation.position, vdop);
        }
    } else if (const auto velocity = NMEA::vtg(sentence)) {
        setFiniteAttribute(observation.position, QGeoPositionInfo::GroundSpeed, velocity->speedMetersPerSecond);
        setFiniteAttribute(observation.position, QGeoPositionInfo::Direction, velocity->courseDegrees);
    } else {
        return;
    }
    _queueEpoch(*_currentEpochMs);
}

void NMEAPositionSource::_queueEpoch(int timeMs)
{
    auto epoch = _epochs.find(timeMs);
    if (epoch == _epochs.end() || epoch->published || !_navigationValid ||
        epoch->navigationSequence <= _invalidThroughSequence || !epoch->observation.position.isValid() ||
        !epoch->observation.receiverFixValid.value_or(true) ||
        epoch->observation.fixQuality == GPSObservation::FixQuality::NoFix) {
        return;
    }

    GPSObservation observation = epoch->observation;
    observation.sourceId = QStringLiteral("NMEA");
    observation.receivedAt = _receiptTime(observation.monotonicTimestampUs);
    if (_pendingFix.requested && _publicationTask.active() && _pendingFix.epochTimeMs &&
        *_pendingFix.epochTimeMs != timeMs) {
        return;
    }
    _lastKnownPosition = observation.position;
    _pendingFix.requested |= _requestTask.active();
    _pendingFix.observation = observation;
    _pendingFix.position = observation.position;
    _pendingFix.epochTimeMs = timeMs;
    if (_pendingFix.requested) {
        _publicationTask.cancel();
    }
    _schedulePublication();
}

void NMEAPositionSource::_trimEpochs()
{
    while (_epochs.size() > MAX_RETAINED_EPOCHS) {
        const auto oldest = std::min_element(_epochs.cbegin(), _epochs.cend(), [](const auto& lhs, const auto& rhs) {
            return lhs.observation.monotonicTimestampUs < rhs.observation.monotonicTimestampUs;
        });
        _epochs.erase(oldest);
    }
}

void NMEAPositionSource::_fixLost(GPSObservation observation)
{
    if (!_started && !_requestTask.active()) {
        return;
    }
    _publicationTask.cancel();
    _pendingFix = {};
    _pendingLoss = std::move(observation);
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
    _pendingFix = {};
    if (!observation.receiverFixValid.value_or(true)) {
        return;
    }
    if (_started || requested) {
        _requestTask.cancel();
        _error = NoError;
        _lastObservation = observation;
        if (epochTimeMs) {
            if (auto epoch = _epochs.find(*epochTimeMs); epoch != _epochs.end()) {
                epoch->published = true;
            }
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
