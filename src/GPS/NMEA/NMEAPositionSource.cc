#include "NMEAPositionSource.h"

#include <QtCore/QIODevice>
#include <QtPositioning/QNmeaPositionInfoSource>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEAPositionSourceLog, "GPS.NMEA.NMEAPositionSource")

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
    _requestDeadline = QDeadlineTimer::Forever;
    _decoder = std::make_unique<QNmeaPositionInfoSource>(QNmeaPositionInfoSource::RealTimeMode);
    if (_device) {
        _decoder->setDevice(_device);
    }
    _decoder->setUserEquivalentRangeError(5.1);
    _decoder->setUpdateInterval(updateInterval());
    const quint64 generation = _generation;
    connect(_decoder.get(), &QGeoPositionInfoSource::positionUpdated, this,
            [this, generation](const QGeoPositionInfo& update) {
                const bool requested = !_requestDeadline.isForever();
                _requestDeadline = QDeadlineTimer::Forever;
                // Leave Qt's parser stack before a consumer can tear down the session.
                QMetaObject::invokeMethod(
                    this,
                    [this, generation, update, requested]() {
                        if (generation == _generation && (_started || requested)) {
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
