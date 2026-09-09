#include "NMEAPositionSource.h"

#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtPositioning/QNmeaPositionInfoSource>

#include <algorithm>
#include <cmath>

#include "GPSReadTimestamp.h"
#include "GPSSourceHealth.h"
#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEAPositionSourceLog, "GPS.NMEA.NMEAPositionSource")
QGC_LOGGING_CATEGORY(NMEATimestampedPositionDecoderLog, "GPS.NMEA.NMEATimestampedPositionDecoder")

class NMEATimestampedPositionDecoder : public QNmeaPositionInfoSource
{
public:
    explicit NMEATimestampedPositionDecoder(QIODevice* device)
        : QNmeaPositionInfoSource(RealTimeMode)
        , _input(device)
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
        if (result.monotonicTimestampUs) {
            result.receivedAt = QDateTime::currentDateTimeUtc().addMSecs(-result.ageMilliseconds());
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
                bool qualityOk = false;
                const int quality = fields[6].toInt(&qualityOk);
                if (qualityOk) {
                    switch (quality) {
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
                bool countOk = false;
                const int count = fields[7].toInt(&countOk);
                metadata.satellitesUsed =
                    countOk && count >= 0 && count <= 256 ? std::optional<int>(count) : std::nullopt;
                metadata.horizontalDop = _number(fields[8], true);
                metadata.altitudeEllipsoidMeters.reset();
                metadata.altitudeDatum = GPSObservation::AltitudeDatum::Unknown;
                const auto altitude = _number(fields[9]);
                const auto geoid = _number(fields[11]);
                if (altitude && fields[10] == "M") {
                    metadata.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
                    if (geoid && fields[12] == "M") {
                        metadata.altitudeEllipsoidMeters = *altitude + *geoid;
                    }
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
    QHash<QTime, GPSObservation> _epochs;
    QTime _currentEpoch;
};

NMEAPositionSource::NMEAPositionSource(QIODevice* device, QObject* parent)
    : QGeoPositionInfoSource(parent)
    , _device(device)
{
    qCDebug(NMEAPositionSourceLog) << this;
    _resetDecoder();
}

NMEAPositionSource::~NMEAPositionSource()
{
    qCDebug(NMEAPositionSourceLog) << this;
}

void NMEAPositionSource::_resetDecoder()
{
    ++_generation;
    _lastObservation = {};
    _lastUpdateReceivedUs = 0;
    _requestDeadline = QDeadlineTimer::Forever;
    _decoder = std::make_unique<NMEATimestampedPositionDecoder>(_device);
    _decoder->setUserEquivalentRangeError(5.1);
    _decoder->setUpdateInterval(updateInterval());
    const quint64 generation = _generation;
    connect(_decoder.get(), &QGeoPositionInfoSource::positionUpdated, this,
            [this, generation](const QGeoPositionInfo& update) {
                const bool requested = !_requestDeadline.isForever();
                _requestDeadline = QDeadlineTimer::Forever;
                const GPSObservation observation =
                    static_cast<NMEATimestampedPositionDecoder*>(_decoder.get())->observation(update);
                // Leave Qt's parser stack before a consumer can tear down the session.
                QMetaObject::invokeMethod(
                    this,
                    [this, generation, requested, observation]() {
                        if (generation == _generation && (_started || requested)) {
                            _lastObservation = observation;
                            _lastUpdateReceivedUs = observation.monotonicTimestampUs;
                            emit positionUpdated(observation.position);
                        }
                    },
                    Qt::QueuedConnection);
            });
    connect(_decoder.get(), &QGeoPositionInfoSource::errorOccurred, this, [this, generation](Error error) {
        if (_requestDeadline.hasExpired()) {
            _requestDeadline = QDeadlineTimer::Forever;
        }
        QMetaObject::invokeMethod(
            this,
            [this, generation, error]() {
                if (generation == _generation) {
                    emit errorOccurred(error);
                }
            },
            Qt::QueuedConnection);
    });
}

qint64 NMEAPositionSource::lastUpdateAgeMs() const
{
    return _lastUpdateReceivedUs ? GPSSourceHealth::ageMilliseconds(_lastUpdateReceivedUs)
                                 : GPSSourceHealth::FRESHNESS_TIMEOUT_MS;
}

void NMEAPositionSource::setUpdateInterval(int msec)
{
    _decoder->setUpdateInterval(msec);
    QGeoPositionInfoSource::setUpdateInterval(_decoder->updateInterval());
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
    return _decoder->error();
}

void NMEAPositionSource::startUpdates()
{
    if (_started) {
        return;
    }
    // Preserve a pending one-shot request when entering continuous mode.
    if (_requestDeadline.isForever() || _requestDeadline.hasExpired()) {
        _resetDecoder();
    }
    _started = true;
    _decoder->startUpdates();
}

void NMEAPositionSource::stopUpdates()
{
    _started = false;
    _decoder->stopUpdates();
}

void NMEAPositionSource::requestUpdate(int timeout)
{
    if (!_requestDeadline.isForever() && !_requestDeadline.hasExpired()) {
        return;
    }
    if (timeout == 0 || timeout >= minimumUpdateInterval()) {
        _requestDeadline.setRemainingTime(timeout == 0 ? 300000 : timeout);
    }
    _decoder->requestUpdate(timeout);
    if (_decoder->error() != NoError) {
        _requestDeadline = QDeadlineTimer::Forever;
    }
}
