#include "NMEAPositionSource.h"

#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtPositioning/QNmeaPositionInfoSource>

#include <algorithm>
#include <cmath>

#include "GPSQtRuntimeScheduler.h"
#include "GPSReadTimestamp.h"
#include "NMEASentence.h"
#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEAPositionSourceLog, "GPS.NMEA.NMEAPositionSource")
QGC_LOGGING_CATEGORY(NMEATimestampedPositionDecoderLog, "GPS.NMEA.NMEATimestampedPositionDecoder")

class NMEATimestampedPositionDecoder : public QNmeaPositionInfoSource
{
public:
    explicit NMEATimestampedPositionDecoder(QIODevice* device, GPSRuntimeScheduler* scheduler)
        : QNmeaPositionInfoSource(RealTimeMode)
        , _input(device)
        , _scheduler(scheduler)
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
        const bool parsed = QNmeaPositionInfoSource::parsePosInfoFromNmeaData(data, size, position, hasFix);
        const QByteArray sentence(data, size);
        if (!NMEAUtils::verifyChecksum(sentence)) {
            return parsed;
        }
        const auto fields = sentence.first(sentence.indexOf('*')).split(',');
        const QByteArray type = fields[0].right(3);
        const quint64 receivedAtUs = GPSReadTimestamp::from(_input);
        if (parsed && position->timestamp().time().isValid() &&
            (type == "GGA" || type == "RMC" || type == "GLL" || type == "GST")) {
            const QTime epoch = position->timestamp().time();
            _currentEpoch = epoch;
            auto& metadata = _epochs[epoch];
            if (position->timestamp().date().isValid()) {
                if (metadata.position.timestamp().date().isValid() &&
                    metadata.position.timestamp().date() != position->timestamp().date()) {
                    metadata = {};
                }
                metadata.position.setTimestamp(position->timestamp());
            }
            _mergeAttributes(metadata.position, *position);
            // The first contributing sentence owns receipt age, including fragmented arrivals.
            metadata.monotonicTimestampUs = metadata.monotonicTimestampUs == 0
                                                ? receivedAtUs
                                                : std::min(metadata.monotonicTimestampUs, receivedAtUs);
            if (type == "GGA" && fields.size() >= 15) {
                const auto parsedSentence = NMEA::sentence({data, static_cast<size_t>(size)});
                const auto fix = parsedSentence ? NMEA::gga(*parsedSentence) : std::nullopt;
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
                    std::min_element(_epochs.begin(), _epochs.end(), [](const auto& lhs, const auto& rhs) {
                        return lhs.monotonicTimestampUs < rhs.monotonicTimestampUs;
                    });
                _epochs.erase(oldest);
            }
        } else if (parsed && (type == "GSA" || type == "VTG") && _currentEpoch.isValid()) {
            auto epoch = _epochs.find(_currentEpoch);
            // GSA has no UTC field. Associate only with the preceding, fresh epoch in this stream;
            // never carry its DOP forward into the next timed fix.
            if (epoch != _epochs.end() && receivedAtUs >= epoch->monotonicTimestampUs &&
                receivedAtUs - epoch->monotonicTimestampUs < 1000000) {
                _mergeAttributes(epoch->position, *position);
                if (type != "GSA" || fields.size() < 18) {
                    return parsed;
                }
                if (!epoch->horizontalDop) {
                    epoch->horizontalDop = _number(fields[16], true);
                }
                epoch->verticalDop = _number(fields[17], true);
                if (fields[2] == "2" && epoch->fixQuality == GPSObservation::FixQuality::Fix3D) {
                    epoch->fixQuality = GPSObservation::FixQuality::Fix2D;
                }
            }
        }
        return parsed;
    }

private:
    static void _mergeAttributes(QGeoPositionInfo& target, const QGeoPositionInfo& source)
    {
        for (auto attribute :
             {QGeoPositionInfo::Direction, QGeoPositionInfo::GroundSpeed, QGeoPositionInfo::VerticalSpeed,
              QGeoPositionInfo::MagneticVariation, QGeoPositionInfo::HorizontalAccuracy,
              QGeoPositionInfo::VerticalAccuracy, QGeoPositionInfo::DirectionAccuracy}) {
            if (source.hasAttribute(attribute)) {
                target.setAttribute(attribute, source.attribute(attribute));
            }
        }
    }

    static std::optional<double> _number(const QByteArray& field, bool nonnegative = false)
    {
        bool ok = false;
        const double value = field.toDouble(&ok);
        return ok && std::isfinite(value) && (!nonnegative || value >= 0) ? std::optional<double>(value) : std::nullopt;
    }

    QPointer<QIODevice> _input;
    QPointer<GPSRuntimeScheduler> _scheduler;
    QHash<QTime, GPSObservation> _epochs;
    QTime _currentEpoch;
};

NMEAPositionSource::NMEAPositionSource(QIODevice* device, QObject* parent, GPSRuntimeScheduler* scheduler)
    : QGeoPositionInfoSource(parent)
    , _device(device)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
{
    qCDebug(NMEAPositionSourceLog) << this;
    _resetDecoder();
}

NMEAPositionSource::~NMEAPositionSource()
{
    qCDebug(NMEAPositionSourceLog) << this;
    _cancelTask(_requestTask);
    _cancelTask(_publicationTask);
}

void NMEAPositionSource::_resetDecoder()
{
    ++_generation;
    _cancelTask(_requestTask);
    _cancelTask(_publicationTask);
    _pendingObservation.reset();
    _pendingRequested = false;
    _lastObservation = {};
    _error = NoError;
    _decoder = std::make_unique<NMEATimestampedPositionDecoder>(_device, _scheduler);
    _decoder->setUserEquivalentRangeError(5.1);
    // Qt owns epoch merging; the outer source owns requested publication cadence.
    _decoder->setUpdateInterval(0);
    const quint64 generation = _generation;
    connect(_decoder.get(), &QGeoPositionInfoSource::positionUpdated, this,
            [this, generation](const QGeoPositionInfo& update) {
                if (generation != _generation || (!_started && !_requestTask)) {
                    return;
                }
                _pendingRequested |= _requestTask != 0;
                _cancelTask(_requestTask);
                _pendingObservation = static_cast<NMEATimestampedPositionDecoder*>(_decoder.get())->observation(update);
                if (_pendingRequested) {
                    _cancelTask(_publicationTask);
                }
                _schedulePublication();
            });
    connect(_decoder.get(), &QGeoPositionInfoSource::errorOccurred, this, [this, generation](Error error) {
        if (_scheduler) {
            _scheduler->schedule(this, std::chrono::microseconds::zero(), [this, generation, error]() {
                if (generation == _generation) {
                    _error = error;
                    emit errorOccurred(error);
                }
            });
        }
    });
}

void NMEAPositionSource::_cancelTask(GPSRuntimeScheduler::TaskId& task)
{
    if (_scheduler) {
        _scheduler->cancel(task);
    }
    task = 0;
}

void NMEAPositionSource::_schedulePublication()
{
    if (!_scheduler || _publicationTask || !_pendingObservation) {
        return;
    }
    const auto generation = _generation;
    const auto delay = std::chrono::milliseconds(_pendingRequested ? 0 : updateInterval());
    _publicationTask = _scheduler->schedule(this, delay, [this, generation]() {
        _publicationTask = 0;
        if (generation == _generation) {
            _publishPending();
        }
    });
}

void NMEAPositionSource::_publishPending()
{
    if (!_pendingObservation) {
        return;
    }
    const auto observation = *_pendingObservation;
    const bool requested = _pendingRequested;
    _pendingObservation.reset();
    _pendingRequested = false;
    if (_started || requested) {
        _error = NoError;
        _lastObservation = observation;
        if (!_started) {
            _decoder->stopUpdates();
        }
        emit positionUpdated(observation.position);
    }
}

void NMEAPositionSource::setUpdateInterval(int msec)
{
    QGeoPositionInfoSource::setUpdateInterval(msec == 0 ? 0 : (std::max) (msec, minimumUpdateInterval()));
    _cancelTask(_publicationTask);
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
    if (_started) {
        return;
    }
    if (!_requestTask) {
        _resetDecoder();
    }
    _started = true;
    _decoder->startUpdates();
}

void NMEAPositionSource::stopUpdates()
{
    _started = false;
    if (!_requestTask) {
        _decoder->stopUpdates();
        if (!_pendingRequested) {
            _cancelTask(_publicationTask);
            _pendingObservation.reset();
        }
    }
}

void NMEAPositionSource::requestUpdate(int timeout)
{
    if (_requestTask || !_scheduler) {
        return;
    }
    const auto generation = _generation;
    if (timeout < 0 || (timeout > 0 && timeout < minimumUpdateInterval())) {
        _scheduler->schedule(this, std::chrono::microseconds::zero(), [this, generation]() {
            if (generation == _generation) {
                _error = UpdateTimeoutError;
                emit errorOccurred(_error);
            }
        });
        return;
    }
    _error = NoError;
    _requestTask =
        _scheduler->schedule(this, std::chrono::milliseconds(timeout == 0 ? 300000 : timeout), [this, generation]() {
            _requestTask = 0;
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
