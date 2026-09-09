#include "NMEADecoderSession.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>
#include <QtPositioning/QNmeaSatelliteInfoSource>

#include "NMEAPositionSource.h"
#include "NMEASatelliteAdapter.h"
#include "NMEAStreamSplitter.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEADecoderSessionLog, "GPS.NMEA.NMEADecoderSession")

NMEADecoderSession::NMEADecoderSession(QObject* parent)
    : QObject(parent)
    , _satellitePollTimer(this)
    , _health(this)
{
    qCDebug(NMEADecoderSessionLog) << this;
    _satellitePollTimer.setInterval(1000);
    connect(&_satellitePollTimer, &QTimer::timeout, this, [this]() {
        if (_satelliteSource) {
            // One-shot requests also report unchanged lists, unlike continuous Qt satellite updates.
            _satelliteSource->requestUpdate(5000);
        }
    });
    connect(&_health, &GPSSourceHealth::satellitesChanged, this, [this]() {
        if (_health.satellitesInViewCount() < 0) {
            _satellitesInView.clear();
        }
        if (_health.satellitesInUseCount() < 0) {
            _satellitesInUse.clear();
        }
        emit satellitesChanged();
    });
}

NMEADecoderSession::~NMEADecoderSession()
{
    qCDebug(NMEADecoderSessionLog) << this;
    stop();
}

QGeoPositionInfoSource* NMEADecoderSession::positionSource() const
{
    return _positionSource.get();
}

bool NMEADecoderSession::start(QIODevice* device)
{
    stop();
    if (!device || !device->isReadable()) {
        return false;
    }
    _stream = std::make_unique<NMEAStreamSplitter>(device);
    _satelliteSource = std::make_unique<QNmeaSatelliteInfoSource>(QNmeaSatelliteInfoSource::UpdateMode::RealTimeMode);
    _satelliteAdapter = std::make_unique<NMEASatelliteAdapter>(_stream->satelliteDevice());
    _satelliteSource->setDevice(_satelliteAdapter.get());
    const QPointer<QNmeaSatelliteInfoSource> current = _satelliteSource.get();
    connect(_satelliteSource.get(), &QGeoSatelliteInfoSource::satellitesInViewUpdated, this,
            [this, current](const QList<QGeoSatelliteInfo>& satellites) {
                const quint64 receivedAtUs = _satelliteAdapter->satelliteTimestampUs(false);
                QMetaObject::invokeMethod(
                    this,
                    [this, current, satellites, receivedAtUs]() {
                        if (!current || _satelliteSource.get() != current) {
                            return;
                        }
                        _satellitesInView = satellites;
                        _health.updateSatellitesInView(
                            satellites.size(), receivedAtUs != 0 ? GPSObservation::ageMilliseconds(receivedAtUs) : -1);
                    },
                    Qt::QueuedConnection);
            });
    connect(_satelliteSource.get(), &QGeoSatelliteInfoSource::satellitesInUseUpdated, this,
            [this, current](const QList<QGeoSatelliteInfo>& satellites) {
                const quint64 receivedAtUs = _satelliteAdapter->satelliteTimestampUs(true);
                QMetaObject::invokeMethod(
                    this,
                    [this, current, satellites, receivedAtUs]() {
                        if (!current || _satelliteSource.get() != current) {
                            return;
                        }
                        _satellitesInUse = satellites;
                        _health.updateSatellitesInUse(
                            satellites.size(), receivedAtUs != 0 ? GPSObservation::ageMilliseconds(receivedAtUs) : -1);
                    },
                    Qt::QueuedConnection);
            });
    connect(
        _satelliteSource.get(), &QGeoSatelliteInfoSource::errorOccurred, this,
        [this, current](QGeoSatelliteInfoSource::Error error) {
            if (current && _satelliteSource.get() == current && error != QGeoSatelliteInfoSource::NoError) {
                _health.clearSatellites();
            }
        },
        Qt::QueuedConnection);
    _satelliteSource->requestUpdate(5000);
    _satellitePollTimer.start();
    _positionSource = std::make_unique<NMEAPositionSource>(_stream->positionDevice());
    connect(_positionSource.get(), &QGeoPositionInfoSource::positionUpdated, &_health,
            [this](const QGeoPositionInfo& position) {
                const qint64 age = _positionSource->lastUpdateAgeMs();
                GPSObservation observation;
                observation.position = position;
                observation.receivedAt = QDateTime::currentDateTimeUtc().addMSecs(-age);
                observation.monotonicTimestampUs = GPSObservation::monotonicNowUs() - age * 1000;
                observation.sourceId = QStringLiteral("NMEA");
                // Qt's GGA parser copies the mean-sea-level altitude field directly.
                observation.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
                _health.updateObservation(observation);
            });
    connect(_positionSource.get(), &QGeoPositionInfoSource::errorOccurred, &_health,
            [this](QGeoPositionInfoSource::Error error) {
                if (error != QGeoPositionInfoSource::NoError) {
                    _health.invalidatePosition();
                }
            });
    return true;
}

void NMEADecoderSession::stop()
{
    _satellitePollTimer.stop();
    _positionSource.reset();
    _satelliteSource.reset();
    _satelliteAdapter.reset();
    _stream.reset();
    _health.reset();
}
