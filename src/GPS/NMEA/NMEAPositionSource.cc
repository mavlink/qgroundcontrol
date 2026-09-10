#include "NMEAPositionSource.h"

#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtCore/QTimeZone>
#include <QtPositioning/QNmeaPositionInfoSource>

#include <algorithm>
#include <cmath>

#include "GPSQtRuntimeScheduler.h"
#include "GPSReadTimestamp.h"
#include "NMEASentenceEnvelope.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEAPositionSourceLog, "GPS.NMEA.NMEAPositionSource")
QGC_LOGGING_CATEGORY(NMEATimestampedPositionDecoderLog, "GPS.NMEA.NMEATimestampedPositionDecoder")

class NMEATimestampedPositionDecoder : public QNmeaPositionInfoSource
{
public:
    explicit NMEATimestampedPositionDecoder(QIODevice* device, GPSRuntimeScheduler* scheduler,
                                            std::function<void(GPSObservation)> fixLost)
        : QNmeaPositionInfoSource(RealTimeMode)
        , _input(device)
        , _scheduler(scheduler)
        , _fixLost(std::move(fixLost))
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
            result = epoch.value();
        }
        // Qt propagates attributes between epochs. Retain only attributes decoded for this epoch.
        result.position.setCoordinate(position.coordinate());
        result.position.setTimestamp(position.timestamp());
        if (_invalidThroughSequence &&
            (!_navigationValid || _epochSequences.value(position.timestamp().time()) <= _invalidThroughSequence)) {
            result.receiverFixValid = false;
            result.fixQuality = GPSObservation::FixQuality::NoFix;
        }
        result.sourceId = QStringLiteral("NMEA");
        const auto nowUs = _scheduler ? _scheduler->nowUs() : GPSObservation::monotonicNowUs();
        if (result.monotonicTimestampUs && result.monotonicTimestampUs <= nowUs) {
            const auto ageMs = static_cast<qint64>((nowUs - result.monotonicTimestampUs) / 1000);
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
            envelope = NMEASentenceEnvelope::parse(QByteArray(data, size), GPSReadTimestamp::from(_input));
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
        if (type == "GGA" && decoded.count > 6) {
            const auto quality = NMEAFields::number<unsigned>(fields[6]);
            if (quality && *quality <= 8) {
                navigationValid = *quality != 0;
            }
        } else if (type == "RMC" && decoded.count > 2 && (fields[2] == "A" || fields[2] == "V")) {
            navigationValid = fields[2] == "A";
        } else if (type == "GLL" && decoded.count > 6 && (fields[6] == "A" || fields[6] == "V")) {
            navigationValid = fields[6] == "A";
        } else if (type == "GSA" && decoded.count > 2) {
            const auto dimension = NMEAFields::number<unsigned>(fields[2]);
            if (dimension && *dimension >= 1 && *dimension <= 3) {
                navigationValid = *dimension != 1;
            }
        }
        if (navigationValid) {
            const auto time = type == "GSA" ? std::nullopt : NMEA::utcMilliseconds(fields[type == "GLL" ? 5 : 1]);
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
                _epochSequences[epoch] = sequence;
            }
        }
        if (accuracy) {
            const auto time = NMEA::utcMilliseconds(fields[1]);
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
            auto& metadata = _epochs[epoch];
            if (metadata.monotonicTimestampUs && receivedAtUs > metadata.monotonicTimestampUs &&
                receivedAtUs - metadata.monotonicTimestampUs > 2000000) {
                metadata = {};
            }
            if (position->timestamp().date().isValid()) {
                if (metadata.position.timestamp().date().isValid() &&
                    metadata.position.timestamp().date() != position->timestamp().date()) {
                    metadata = {};
                }
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
            if (type == "GGA" && decoded.count >= 13) {
                const auto fix = NMEA::gga(decoded);
                if (fix) {
                    switch (fix->quality) {
                        case 0:
                            metadata.fixQuality = GPSObservation::FixQuality::NoFix;
                            break;
                        case 1:
                            metadata.fixQuality = GPSObservation::FixQuality::Fix3D;
                            break;
                        case 2:
                            metadata.fixQuality = GPSObservation::FixQuality::Differential;
                            break;
                        case 4:
                            metadata.fixQuality = GPSObservation::FixQuality::RTKFixed;
                            break;
                        case 5:
                            metadata.fixQuality = GPSObservation::FixQuality::RTKFloat;
                            break;
                        case 6:
                            metadata.fixQuality = GPSObservation::FixQuality::Extrapolated;
                            break;
                        default:
                            metadata.fixQuality = GPSObservation::FixQuality::Unknown;
                            break;
                    }
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
            if (_epochs.size() > 32) {
                const auto oldest =
                    std::min_element(_epochs.cbegin(), _epochs.cend(), [](const auto& lhs, const auto& rhs) {
                        return lhs.monotonicTimestampUs < rhs.monotonicTimestampUs;
                    });
                _epochSequences.remove(oldest.key());
                _epochs.erase(oldest);
            }
        } else if (parsed && (type == "GSA" || type == "VTG") && _currentEpoch.isValid()) {
            auto epoch = _epochs.find(_currentEpoch);
            // GSA has no UTC field. Associate only with the preceding, fresh epoch in this stream;
            // never carry its DOP forward into the next timed fix.
            if (epoch != _epochs.end() && receivedAtUs >= epoch->monotonicTimestampUs &&
                receivedAtUs - epoch->monotonicTimestampUs < 1000000) {
                _mergeAttributes(epoch->position, *position, epoch->accuracyTimestampUs != 0);
                if (type != "GSA" || decoded.count < 18) {
                    return parsed;
                }
                if (!epoch->horizontalDop) {
                    epoch->horizontalDop = _nonnegativeNumber(fields[16]);
                }
                epoch->verticalDop = _nonnegativeNumber(fields[17]);
                if (fields[2] == "2" && epoch->fixQuality == GPSObservation::FixQuality::Fix3D) {
                    epoch->fixQuality = GPSObservation::FixQuality::Fix2D;
                }
            }
        }
        return parsed;
    }

private:
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
        const auto value = NMEAFields::number<double>(field);
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
                if (difference < -12 * 60 * 60 * 1000) {
                    difference += 24 * 60 * 60 * 1000;
                } else if (difference > 12 * 60 * 60 * 1000) {
                    difference -= 24 * 60 * 60 * 1000;
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
    QPointer<GPSRuntimeScheduler> _scheduler;
    QHash<QTime, GPSObservation> _epochs;
    QTime _currentEpoch;
    QHash<QTime, quint64> _epochSequences;
    std::function<void(GPSObservation)> _fixLost;
    quint64 _sentenceSequence = 0;
    quint64 _invalidThroughSequence = 0;
    quint64 _statusReceiptUs = 0;
    QTime _statusTime;
    QDate _statusDate;
    bool _navigationValid = true;
};

NMEAPositionSource::NMEAPositionSource(QIODevice* device, QObject* parent, GPSRuntimeScheduler* scheduler)
    : QGeoPositionInfoSource(parent)
    , _device(device)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
    , _requestTask(_scheduler, this)
    , _publicationTask(_scheduler, this)
    , _lossTask(_scheduler, this)
    , _errorTask(_scheduler, this)
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
    _decoder->setUserEquivalentRangeError(5.1);
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
                    return;
                }
                _pendingObservation = observation;
                if (_pendingRequested) {
                    _publicationTask.cancel();
                }
                _schedulePublication();
            });
    connect(_decoder.get(), &QGeoPositionInfoSource::errorOccurred, this, [this, generation](Error error) {
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
    const auto generation = _generation;
    if (timeout < 0 || (timeout > 0 && timeout < minimumUpdateInterval())) {
        _errorTask.schedule(std::chrono::microseconds::zero(), [this, generation]() {
            if (generation == _generation) {
                _error = UpdateTimeoutError;
                emit errorOccurred(_error);
            }
        });
        return;
    }
    _error = NoError;
    _requestTask.schedule(std::chrono::milliseconds(timeout == 0 ? 300000 : timeout), [this, generation]() {
        if (generation != _generation) {
            return;
        }
        if (!_started) {
            _decoder->stopUpdates();
        }
        _error = UpdateTimeoutError;
        emit errorOccurred(_error);
    });
    _decoder->startUpdates();
}
