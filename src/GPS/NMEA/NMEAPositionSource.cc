#include "NMEAPositionSource.h"

#include <QtCore/QHash>
#include <QtCore/QIODevice>
#include <QtPositioning/QNmeaPositionInfoSource>

#include <algorithm>

#include "GPSReadTimestamp.h"
#include "GPSSourceHealth.h"
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

    ~NMEATimestampedPositionDecoder() override { qCDebug(NMEAPositionSourceLog) << this; }

    quint64 receiptTimestampUs(const QGeoPositionInfo& position) const
    {
        return _receipts.value(position.timestamp().time());
    }

protected:
    bool parsePosInfoFromNmeaData(const char* data, int size, QGeoPositionInfo* position, bool* hasFix) override
    {
        const bool parsed = QNmeaPositionInfoSource::parsePosInfoFromNmeaData(data, size, position, hasFix);
        if (parsed && position->coordinate().isValid() && position->timestamp().time().isValid()) {
            const QTime epoch = position->timestamp().time();
            const quint64 receivedAtUs = GPSReadTimestamp::from(_input);
            auto receipt = _receipts.find(epoch);
            if (receipt == _receipts.end()) {
                _receipts.insert(epoch, receivedAtUs);
            } else {
                *receipt = std::min(*receipt, receivedAtUs);
            }
            if (_receipts.size() > 32) {
                const auto oldest = std::min_element(_receipts.begin(), _receipts.end());
                _receipts.erase(oldest);
            }
        }
        return parsed;
    }

private:
    QPointer<QIODevice> _input;
    QHash<QTime, quint64> _receipts;
};

NMEAPositionSource::NMEAPositionSource(QIODevice* device, QObject* parent)
    : QGeoPositionInfoSource(parent)
    , _device(device)
{
    qCDebug(NMEATimestampedPositionDecoderLog) << this;
    _resetDecoder();
}

NMEAPositionSource::~NMEAPositionSource()
{
    qCDebug(NMEAPositionSourceLog) << this;
}

void NMEAPositionSource::_resetDecoder()
{
    ++_generation;
    _requestDeadline = QDeadlineTimer::Forever;
    _decoder = std::make_unique<NMEATimestampedPositionDecoder>(_device);
    _decoder->setUserEquivalentRangeError(5.1);
    _decoder->setUpdateInterval(updateInterval());
    const quint64 generation = _generation;
    connect(_decoder.get(), &QGeoPositionInfoSource::positionUpdated, this,
            [this, generation](const QGeoPositionInfo& update) {
                const bool requested = !_requestDeadline.isForever();
                _requestDeadline = QDeadlineTimer::Forever;
                const quint64 receivedAtUs =
                    static_cast<NMEATimestampedPositionDecoder*>(_decoder.get())->receiptTimestampUs(update);
                // Leave Qt's parser stack before a consumer can tear down the session.
                QMetaObject::invokeMethod(
                    this,
                    [this, generation, update, requested, receivedAtUs]() {
                        if (generation == _generation && (_started || requested)) {
                            _lastUpdateReceivedUs = receivedAtUs;
                            emit positionUpdated(update);
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
