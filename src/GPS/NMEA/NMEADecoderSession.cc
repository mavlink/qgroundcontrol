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
        const QPointer<NMEADecoderSession> guard(this);
        _expireSatellites();
        if (guard && _satelliteSource) {
            // One-shot requests also report unchanged lists, unlike continuous Qt satellite updates.
            _satelliteSource->requestUpdate(5000);
        }
    });
    connect(&_health, &GPSSourceHealth::satellitesChanged, this, [this]() {
        if (!_refreshingSatellites && _satelliteAdapter &&
            (_health.satellitesInViewCount() < 0 || _health.satellitesInUseCount() < 0)) {
            _expireSatellites();
            return;
        }
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
                const auto snapshot =
                    _satelliteAdapter->freshSatellites(satellites, false, GPSObservation::monotonicNowUs());
                QMetaObject::invokeMethod(
                    this,
                    [this, current, snapshot]() {
                        if (!current || _satelliteSource.get() != current) {
                            return;
                        }
                        _viewSnapshot = snapshot;
                        _expireSatellites();
                    },
                    Qt::QueuedConnection);
            });
    connect(_satelliteSource.get(), &QGeoSatelliteInfoSource::satellitesInUseUpdated, this,
            [this, current](const QList<QGeoSatelliteInfo>& satellites) {
                const auto snapshot =
                    _satelliteAdapter->freshSatellites(satellites, true, GPSObservation::monotonicNowUs());
                QMetaObject::invokeMethod(
                    this,
                    [this, current, snapshot]() {
                        if (!current || _satelliteSource.get() != current) {
                            return;
                        }
                        _useSnapshot = snapshot;
                        _expireSatellites();
                    },
                    Qt::QueuedConnection);
            });
    connect(
        _satelliteSource.get(), &QGeoSatelliteInfoSource::errorOccurred, this,
        [this, current](QGeoSatelliteInfoSource::Error error) {
            if (current && _satelliteSource.get() == current && error != QGeoSatelliteInfoSource::NoError) {
                _viewSnapshot = {};
                _useSnapshot = {};
                _health.clearSatelliteReports();
            }
        },
        Qt::QueuedConnection);
    _satelliteSource->requestUpdate(5000);
    _satellitePollTimer.start();
    _positionSource = std::make_unique<NMEAPositionSource>(_stream->positionDevice());
    connect(_positionSource.get(), &QGeoPositionInfoSource::positionUpdated, &_health,
            [this](const QGeoPositionInfo&) { _health.updateObservation(_positionSource->lastObservation()); });
    connect(_positionSource.get(), &QGeoPositionInfoSource::errorOccurred, &_health,
            [this](QGeoPositionInfoSource::Error error) {
                if (error != QGeoPositionInfoSource::NoError) {
                    _health.invalidatePosition();
                }
            });
    return true;
}

void NMEADecoderSession::_expireSatellites()
{
    if (!_satelliteAdapter || _refreshingSatellites) {
        return;
    }
    const auto view = NMEASatelliteAdapter::expireSnapshot(_viewSnapshot, GPSObservation::monotonicNowUs());
    const auto used = NMEASatelliteAdapter::expireSnapshot(_useSnapshot, GPSObservation::monotonicNowUs());
    const QPointer<NMEADecoderSession> guard(this);
    const QPointer<NMEASatelliteAdapter> adapter(_satelliteAdapter.get());
    _refreshingSatellites = true;
    _satellitesInView = view.satellites;
    _health.updateSatellitesInView(view.satellites.size(),
                                   view.receivedAtUs ? GPSObservation::ageMilliseconds(view.receivedAtUs) : -1);
    if (guard && adapter && adapter == _satelliteAdapter.get()) {
        _satellitesInUse = used.satellites;
        _health.updateSatellitesInUse(used.satellites.size(),
                                      used.receivedAtUs ? GPSObservation::ageMilliseconds(used.receivedAtUs) : -1);
        if (guard) {
            _refreshingSatellites = false;
        }
    }
}

void NMEADecoderSession::stop()
{
    _satellitePollTimer.stop();
    _positionSource.reset();
    _satelliteSource.reset();
    _satelliteAdapter.reset();
    _stream.reset();
    _viewSnapshot = {};
    _useSnapshot = {};
    _refreshingSatellites = false;
    _health.reset();
}
