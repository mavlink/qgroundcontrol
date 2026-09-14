#include "NMEAPositionSource.h"

#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtCore/QTimeZone>
#include <QtPositioning/QNmeaPositionInfoSource>

#include <algorithm>
#include <chrono>
#include <cmath>

#include "MonotonicClock.h"
#include "NMEASentenceEnvelope.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"
#include "ReadTimestamp.h"

namespace {
constexpr int DEFAULT_REQUEST_TIMEOUT_MS = std::chrono::milliseconds(std::chrono::minutes(5)).count();
constexpr quint64 METADATA_MAX_AGE_US = std::chrono::microseconds(std::chrono::seconds(2)).count();
constexpr quint64 UNTIMED_METADATA_MAX_AGE_US = std::chrono::microseconds(std::chrono::seconds(1)).count();
constexpr qsizetype MAX_RETAINED_EPOCHS = 32;
constexpr double USER_EQUIVALENT_RANGE_ERROR_METERS = 5.1;
constexpr int HALF_DAY_MS = std::chrono::milliseconds(std::chrono::hours(12)).count();
constexpr int DAY_MS = std::chrono::milliseconds(std::chrono::days(1)).count();
}  // namespace

QGC_LOGGING_CATEGORY(NMEAPositionSourceLog, "GPS.NMEA.NMEAPositionSource")
QGC_LOGGING_CATEGORY(NMEATimestampedPositionDecoderLog, "GPS.NMEA.NMEATimestampedPositionDecoder")

class NMEATimestampedPositionDecoder : public QNmeaPositionInfoSource
{
public:
    explicit NMEATimestampedPositionDecoder(QIODevice* device, RuntimeScheduler* scheduler,
                                            std::function<void(GPSObservation)> fixLost)
        : QNmeaPositionInfoSource(RealTimeMode), _input(device), _scheduler(scheduler), _fixLost(std::move(fixLost))
    {
        qCDebug(NMEATimestampedPositionDecoderLog) << this;
        if (device) {
            setDevice(device);
        }
    }

    ~NMEATimestampedPositionDecoder() override { qCDebug(NMEATimestampedPositionDecoderLog) << this; }

    GPSObservation observation(const QGeoPositionInfo& position) const
    {
        GPSObservation result;
        const auto epoch = _epochs.constFind(position.timestamp().time());
        if (epoch != _epochs.cend()) {
            result = epoch->observation;
        }
        // Qt propagates attributes between epochs. Retain only attributes decoded for this epoch.
        result.position.setCoordinate(position.coordinate());
        result.position.setTimestamp(position.timestamp());
        if (_invalidThroughSequence &&
            (!_navigationValid || epoch == _epochs.cend() || epoch->navigationSequence <= _invalidThroughSequence)) {
            result.receiverFixValid = false;
            result.fixQuality = GPSObservation::FixQuality::NoFix;
        }
        result.sourceId = QStringLiteral("NMEA");
        const auto nowUs = _scheduler ? _scheduler->nowUs() : ReadTimestamp::nowUs();
        const auto ageMs = MonotonicClock::ageMilliseconds(result.monotonicTimestampUs, nowUs);
        if (ageMs >= 0) {
            result.receivedAt = QDateTime::currentDateTimeUtc().addMSecs(-ageMs);
        }
        return result;
    }

protected:
    bool parsePosInfoFromNmeaData(const char* data, int size, QGeoPositionInfo* position, bool* hasFix) override
    {
        auto envelope = std::optional<NMEASentenceEnvelope>();
        if (const auto* provider = dynamic_cast<NMEASentenceProvider*>(_input.data())) {
            envelope = provider->lastReadSentence();
            if (envelope && QByteArrayView(envelope->bytes()) != QByteArrayView(data, size)) {
                envelope.reset();
            }
        }
        if (!envelope) {
            envelope = NMEASentenceEnvelope::parse(
                QByteArray(data, size),
                ReadTimestamp::from(_input, _scheduler ? _scheduler->nowUs() : ReadTimestamp::nowUs()));
        }
        if (!envelope) {
            return false;
        }
        bool parsed = QNmeaPositionInfoSource::parsePosInfoFromNmeaData(data, size, position, hasFix);
        const auto& decoded = envelope->sentence();
        const auto& fields = decoded.fields;
        const auto type = decoded.type();
        const quint64 receivedAtUs = envelope->receivedAtUs();
        const auto accuracy = NMEA::gst(decoded);
        const quint64 sequence = ++_sentenceSequence;
        std::optional<bool> navigationValid;
        if (type == "GGA" && decoded.count > NMEA::Field::GGA_QUALITY) {
            const auto quality = NMEA::number<unsigned>(fields[NMEA::Field::GGA_QUALITY]);
            if (quality && *quality <= NMEA::GgaQuality::MAX_VALUE) {
                navigationValid = *quality != NMEA::GgaQuality::INVALID;
            }
        } else if (type == "RMC" && decoded.count > NMEA::Field::RMC_STATUS &&
                   (fields[NMEA::Field::RMC_STATUS] == "A" || fields[NMEA::Field::RMC_STATUS] == "V")) {
            navigationValid = fields[NMEA::Field::RMC_STATUS] == "A";
        } else if (type == "GLL" && decoded.count > NMEA::Field::GLL_STATUS &&
                   (fields[NMEA::Field::GLL_STATUS] == "A" || fields[NMEA::Field::GLL_STATUS] == "V")) {
            navigationValid = fields[NMEA::Field::GLL_STATUS] == "A";
        } else if (type == "GSA" && decoded.count > NMEA::Field::GSA_DIMENSION) {
            const auto dimension = NMEA::number<unsigned>(fields[NMEA::Field::GSA_DIMENSION]);
            if (dimension && *dimension >= NMEA::FixDimension::NO_FIX && *dimension <= NMEA::FixDimension::THREE_D) {
                navigationValid = *dimension != NMEA::FixDimension::NO_FIX;
            }
        }
        if (navigationValid) {
            const auto time =
                type == "GSA"
                    ? std::nullopt
                    : NMEA::utcMilliseconds(fields[type == "GLL" ? NMEA::Field::GLL_TIME : NMEA::Field::UTC_TIME]);
            const QTime epoch = time ? QTime::fromMSecsSinceStartOfDay(*time) : QTime();
            const QDate date = position->timestamp().date();
            if (!_acceptNavigationStatus(epoch, date, receivedAtUs)) {
                return false;
            }
            if (!*navigationValid) {
                _navigationValid = false;
                _invalidThroughSequence = sequence;
                GPSObservation loss;
                loss.position = *position;
                loss.receiverFixValid = false;
                loss.fixQuality = GPSObservation::FixQuality::NoFix;
                loss.monotonicTimestampUs = receivedAtUs;
                loss.sourceId = QStringLiteral("NMEA");
                loss.receivedAt = QDateTime::currentDateTimeUtc();
                _fixLost(std::move(loss));
            } else if (parsed && *hasFix && epoch.isValid()) {
                _navigationValid = true;
                _epochs[epoch].navigationSequence = sequence;
            }
        }
        if (accuracy) {
            const auto time = NMEA::utcMilliseconds(fields[NMEA::Field::UTC_TIME]);
            if (!time) {
                return false;
            }
            position->setTimestamp(QDateTime(QDate(), QTime::fromMSecsSinceStartOfDay(*time), QTimeZone::UTC));
            if (std::isfinite(accuracy->horizontalAccuracy)) {
                position->setAttribute(QGeoPositionInfo::HorizontalAccuracy, accuracy->horizontalAccuracy);
            }
            if (std::isfinite(accuracy->verticalAccuracy)) {
                position->setAttribute(QGeoPositionInfo::VerticalAccuracy, accuracy->verticalAccuracy);
            }
            *hasFix = false;
            parsed = true;
        }
        if (parsed && position->timestamp().time().isValid() &&
            (type == "GGA" || type == "RMC" || type == "GLL" || type == "GST")) {
            const QTime epoch = position->timestamp().time();
            _currentEpoch = epoch;
            auto& epochData = _epochs[epoch];
            auto& metadata = epochData.observation;
            const QDate date = position->timestamp().date();
            const bool expired = metadata.monotonicTimestampUs && receivedAtUs > metadata.monotonicTimestampUs &&
                                 receivedAtUs - metadata.monotonicTimestampUs > METADATA_MAX_AGE_US;
            if (expired || (date.isValid() && metadata.position.timestamp().date().isValid() &&
                            metadata.position.timestamp().date() != date)) {
                // Measurement expiry must preserve navigation ordering, including a fix just decoded above.
                const auto navigationSequence = epochData.navigationSequence;
                epochData = {};
                epochData.navigationSequence = navigationSequence;
            }
            if (date.isValid()) {
                metadata.position.setTimestamp(position->timestamp());
            }
            _mergeAttributes(metadata.position, *position, metadata.accuracyTimestampUs && !accuracy);
            if (accuracy) {
                metadata.accuracyTimestampUs = receivedAtUs;
            }
            // The first contributing sentence owns receipt age, including fragmented arrivals.
            metadata.monotonicTimestampUs = metadata.monotonicTimestampUs == 0
                                                ? receivedAtUs
                                                : std::min(metadata.monotonicTimestampUs, receivedAtUs);
            if (type == "GGA" && decoded.count >= NMEA::Field::GGA_MIN_FIELDS) {
                const auto fix = NMEA::gga(decoded);
                if (fix) {
                    epochData.ggaQuality = fix->quality;
                    metadata.fixQuality = _fixQuality(epochData);
                }
                metadata.satellitesUsed = fix ? fix->satellitesUsed : std::nullopt;
                metadata.horizontalDop =
                    fix && std::isfinite(fix->hdop) && fix->hdop > 0 ? std::optional<double>(fix->hdop) : std::nullopt;
                metadata.dopTimestampUs = receivedAtUs;
                metadata.altitudeEllipsoidMeters.reset();
                metadata.altitudeDatum = GPSObservation::AltitudeDatum::Unknown;
                if (fix && std::isfinite(fix->altitude)) {
                    metadata.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
                    if (std::isfinite(fix->geoidSeparation))
                        metadata.altitudeEllipsoidMeters = fix->altitude + fix->geoidSeparation;
                }
            }
            if (_epochs.size() > MAX_RETAINED_EPOCHS) {
                const auto oldest =
                    std::min_element(_epochs.cbegin(), _epochs.cend(), [](const auto& lhs, const auto& rhs) {
                        return lhs.observation.monotonicTimestampUs < rhs.observation.monotonicTimestampUs;
                    });
                _epochs.erase(oldest);
            }
        } else if (parsed && (type == "GSA" || type == "VTG") && _currentEpoch.isValid()) {
            auto epoch = _epochs.find(_currentEpoch);
            // GSA has no UTC field. Associate only with the preceding, fresh epoch in this stream;
            // never carry its DOP forward into the next timed fix.
            if (epoch != _epochs.end() && receivedAtUs >= epoch->observation.monotonicTimestampUs &&
                receivedAtUs - epoch->observation.monotonicTimestampUs < UNTIMED_METADATA_MAX_AGE_US) {
                _mergeAttributes(epoch->observation.position, *position, epoch->observation.accuracyTimestampUs != 0);
                if (type != "GSA" || decoded.count < NMEA::Field::GSA_MIN_FIELDS) {
                    return parsed;
                }
                if (!epoch->observation.horizontalDop) {
                    epoch->observation.horizontalDop = _nonnegativeNumber(fields[NMEA::Field::GSA_HDOP]);
                }
                epoch->observation.verticalDop = _nonnegativeNumber(fields[NMEA::Field::GSA_VDOP]);
                epoch->dimension = NMEA::number<unsigned>(fields[NMEA::Field::GSA_DIMENSION]);
                epoch->observation.fixQuality = _fixQuality(*epoch);
            }
        }
        return parsed;
    }

private:
    struct EpochMetadata
    {
        GPSObservation observation;
        std::optional<unsigned> ggaQuality;
        std::optional<unsigned> dimension;
        quint64 navigationSequence = 0;
    };

    static GPSObservation::FixQuality _fixQuality(const EpochMetadata& epoch)
    {
        using Quality = GPSObservation::FixQuality;
        switch (epoch.ggaQuality.value_or(NMEA::GgaQuality::GPS)) {
            case NMEA::GgaQuality::INVALID:
                return Quality::NoFix;
            case NMEA::GgaQuality::GPS:
                if (epoch.dimension == NMEA::FixDimension::TWO_D)
                    return Quality::Fix2D;
                if (epoch.dimension == NMEA::FixDimension::THREE_D)
                    return Quality::Fix3D;
                return Quality::Unknown;
            case NMEA::GgaQuality::DIFFERENTIAL:
                return Quality::Differential;
            case NMEA::GgaQuality::RTK_FIXED:
                return Quality::RTKFixed;
            case NMEA::GgaQuality::RTK_FLOAT:
                return Quality::RTKFloat;
            case NMEA::GgaQuality::ESTIMATED:
                return Quality::Extrapolated;
            default:
                return Quality::Unknown;
        }
    }

    static void _mergeAttributes(QGeoPositionInfo& target, const QGeoPositionInfo& source,
                                 bool preserveAccuracy = false)
    {
        for (auto attribute :
             {QGeoPositionInfo::Direction, QGeoPositionInfo::GroundSpeed, QGeoPositionInfo::VerticalSpeed,
              QGeoPositionInfo::MagneticVariation, QGeoPositionInfo::HorizontalAccuracy,
              QGeoPositionInfo::VerticalAccuracy, QGeoPositionInfo::DirectionAccuracy}) {
            if (preserveAccuracy && target.hasAttribute(attribute) &&
                (attribute == QGeoPositionInfo::HorizontalAccuracy ||
                 attribute == QGeoPositionInfo::VerticalAccuracy)) {
                continue;
            }
            if (source.hasAttribute(attribute)) {
                target.setAttribute(attribute, source.attribute(attribute));
            }
        }
    }

    static std::optional<double> _nonnegativeNumber(std::string_view field)
    {
        const auto value = NMEA::number<double>(field);
        return value && *value >= 0 ? value : std::nullopt;
    }

    bool _acceptNavigationStatus(const QTime& time, const QDate& date, quint64 receivedAtUs)
    {
        if (!receivedAtUs || receivedAtUs < _statusReceiptUs) {
            return false;
        }
        if (time.isValid() && _statusTime.isValid()) {
            if (date.isValid() && _statusDate.isValid()) {
                if (QDateTime(date, time, QTimeZone::UTC) < QDateTime(_statusDate, _statusTime, QTimeZone::UTC)) {
                    return false;
                }
            } else {
                int difference = _statusTime.msecsTo(time);
                // Undated GGA timestamps wrap at midnight; ordinary negative deltas are late sentences.
                if (difference < -HALF_DAY_MS) {
                    difference += DAY_MS;
                } else if (difference > HALF_DAY_MS) {
                    difference -= DAY_MS;
                }
                if (difference < 0) {
                    return false;
                }
            }
        }
        _statusReceiptUs = receivedAtUs;
        if (time.isValid()) {
            _statusTime = time;
            _statusDate = date;
        }
        return true;
    }

    QPointer<QIODevice> _input;
    QPointer<RuntimeScheduler> _scheduler;
    QHash<QTime, EpochMetadata> _epochs;
    QTime _currentEpoch;
    std::function<void(GPSObservation)> _fixLost;
    quint64 _sentenceSequence = 0;
    quint64 _invalidThroughSequence = 0;
    quint64 _statusReceiptUs = 0;
    QTime _statusTime;
    QDate _statusDate;
    bool _navigationValid = true;
};

NMEAPositionSource::NMEAPositionSource(QIODevice* device, QObject* parent, RuntimeScheduler* scheduler)
    : QGeoPositionInfoSource(parent),
      _device(device),
      _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this)),
      _requestTask(_scheduler, this),
      _publicationTask(_scheduler, this),
      _lossTask(_scheduler, this),
      _errorTask(_scheduler, this)
{
    qCDebug(NMEAPositionSourceLog) << this;
    _resetDecoder();
}

NMEAPositionSource::~NMEAPositionSource()
{
    qCDebug(NMEAPositionSourceLog) << this;
    _requestTask.cancel();
    _publicationTask.cancel();
}

void NMEAPositionSource::_resetDecoder()
{
    ++_generation;
    _requestTask.cancel();
    _publicationTask.cancel();
    _lossTask.cancel();
    _errorTask.cancel();
    _pendingLoss.reset();
    _pendingObservation.reset();
    _pendingRequested = false;
    _lastObservation = {};
    _error = NoError;
    _decoder = std::make_unique<NMEATimestampedPositionDecoder>(
        _device, _scheduler, [this](GPSObservation loss) { _fixLost(std::move(loss)); });
    _decoder->setUserEquivalentRangeError(USER_EQUIVALENT_RANGE_ERROR_METERS);
    // Qt owns epoch merging; the outer source owns requested publication cadence.
    _decoder->setUpdateInterval(0);
    const quint64 generation = _generation;
    connect(_decoder.get(), &QGeoPositionInfoSource::positionUpdated, this,
            [this, generation](const QGeoPositionInfo& update) {
                if (generation != _generation || (!_started && !_requestTask.active())) {
                    return;
                }
                _pendingRequested |= _requestTask.active();
                const auto observation =
                    static_cast<NMEATimestampedPositionDecoder*>(_decoder.get())->observation(update);
                if (!observation.receiverFixValid.value_or(true)) {
                    if (!_started && _requestTask.active()) {
                        _decoder->requestUpdate(DEFAULT_REQUEST_TIMEOUT_MS);
                    }
                    return;
                }
                _pendingObservation = observation;
                if (_pendingRequested) {
                    _publicationTask.cancel();
                }
                _schedulePublication();
            });
    connect(_decoder.get(), &QGeoPositionInfoSource::errorOccurred, this, [this, generation](Error error) {
        // The outer request task owns deadlines, including after Qt rejects a retained fix.
        if (error == UpdateTimeoutError) {
            return;
        }
        if (_scheduler) {
            _errorTask.schedule(std::chrono::microseconds::zero(), [this, generation, error]() {
                if (generation == _generation) {
                    _error = error;
                    emit errorOccurred(error);
                }
            });
        }
    });
}

void NMEAPositionSource::_fixLost(GPSObservation observation)
{
    if (!_started && !_requestTask.active()) {
        return;
    }
    _publicationTask.cancel();
    _pendingObservation.reset();
    _pendingRequested = false;
    _pendingLoss = std::move(observation);
    if (!_lossTask.active()) {
        const auto generation = _generation;
        // Qt is still decoding a line. Notify consumers only after its parser stack unwinds.
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
    if (!_scheduler || _publicationTask.active() || !_pendingObservation) {
        return;
    }
    const auto generation = _generation;
    const auto delay = std::chrono::milliseconds(_pendingRequested ? 0 : updateInterval());
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
    if (!_pendingObservation) {
        return;
    }
    const auto observation =
        static_cast<NMEATimestampedPositionDecoder*>(_decoder.get())->observation(_pendingObservation->position);
    const bool requested = _pendingRequested;
    _pendingObservation.reset();
    _pendingRequested = false;
    if (!observation.receiverFixValid.value_or(true)) {
        return;
    }
    if (_started || requested) {
        _requestTask.cancel();
        _error = NoError;
        _lastObservation = observation;
        if (!_started) {
            _decoder->stopUpdates();
        }
        const QPointer<NMEAPositionSource> guard(this);
        const auto generation = _generation;
        emit observationReceived(observation);
        if (guard && generation == _generation) {
            emit positionUpdated(observation.position);
        }
    }
}

void NMEAPositionSource::setUpdateInterval(int msec)
{
    QGeoPositionInfoSource::setUpdateInterval(msec == 0 ? 0 : (std::max) (msec, minimumUpdateInterval()));
    _publicationTask.cancel();
    _schedulePublication();
}

QGeoPositionInfo NMEAPositionSource::lastKnownPosition(bool satelliteOnly) const
{
    return _decoder->lastKnownPosition(satelliteOnly);
}

QGeoPositionInfoSource::PositioningMethods NMEAPositionSource::supportedPositioningMethods() const
{
    return _decoder->supportedPositioningMethods();
}

int NMEAPositionSource::minimumUpdateInterval() const
{
    return _decoder->minimumUpdateInterval();
}

QGeoPositionInfoSource::Error NMEAPositionSource::error() const
{
    return _error;
}

void NMEAPositionSource::startUpdates()
{
    if (_started || !_scheduler) {
        return;
    }
    if (!_requestTask.active()) {
        _resetDecoder();
    }
    _started = true;
    _decoder->startUpdates();
}

void NMEAPositionSource::stopUpdates()
{
    _started = false;
    if (!_requestTask.active()) {
        _lossTask.cancel();
        _pendingLoss.reset();
        _decoder->stopUpdates();
        if (!_pendingRequested) {
            _publicationTask.cancel();
            _pendingObservation.reset();
        }
    }
}

void NMEAPositionSource::requestUpdate(int timeout)
{
    if (_requestTask.active() || !_scheduler) {
        return;
    }
    if (timeout < 0 || (timeout > 0 && timeout < minimumUpdateInterval())) {
        const auto generation = _generation;
        _errorTask.schedule(std::chrono::microseconds::zero(), [this, generation]() {
            if (generation == _generation) {
                _error = UpdateTimeoutError;
                emit errorOccurred(_error);
            }
        });
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
                              if (!_started) {
                                  _decoder->stopUpdates();
                              }
                              _error = UpdateTimeoutError;
                              emit errorOccurred(_error);
                          });
    if (!_started) {
        _decoder->requestUpdate(timeout == 0 ? DEFAULT_REQUEST_TIMEOUT_MS : timeout);
    }
}
