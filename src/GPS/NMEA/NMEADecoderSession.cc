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
        if (_refreshingSatellites) {
            return;
        }
        if (_satelliteAdapter && (_health.satellitesInViewCount() < 0 || _health.satellitesInUseCount() < 0)) {
            _expireSatellites();
            return;
        }
        if (_health.satellitesInViewCount() < 0) {
            _satellitesInView.clear();
            _satellitesReceivedAtUs = 0;
        }
        if (_health.satellitesInUseCount() < 0) {
            _satellitesInUse.clear();
            _satellitesUsedKnown = false;
            _satelliteUseSystems.clear();
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
                _viewRejectedThroughUs = GPSObservation::monotonicNowUs();
                _useRejectedThroughUs = _viewRejectedThroughUs;
                _viewSnapshot = {};
                _useSnapshot = {};
                _expireSatellites();
            }
        },
        Qt::QueuedConnection);
    _satelliteSource->requestUpdate(5000);
    _satellitePollTimer.start();
    _positionSource = std::make_unique<NMEAPositionSource>(_stream->positionDevice());
    connect(_positionSource.get(), &QGeoPositionInfoSource::positionUpdated, &_health, [this](const QGeoPositionInfo&) {
        auto observation = _positionSource->lastObservation();
        observation.sessionId = _sessionId;
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

void NMEADecoderSession::_expireSatellites()
{
    if (!_satelliteAdapter || _refreshingSatellites) {
        return;
    }
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    const quint64 freshnessUs = static_cast<quint64>(_health.freshnessTimeoutMs()) * 1000;
    const auto expire = [nowUs, freshnessUs](NMEASatelliteAdapter::Snapshot snapshot, quint64& rejectedThroughUs) {
        for (auto it = snapshot.constellationReceipts.begin(); it != snapshot.constellationReceipts.end();) {
            const quint64 receipt = it.value();
            if (!receipt || receipt <= rejectedThroughUs || receipt > nowUs || nowUs - receipt >= freshnessUs) {
                if (receipt <= nowUs && receipt > rejectedThroughUs) {
                    rejectedThroughUs = receipt;
                }
                it = snapshot.constellationReceipts.erase(it);
            } else {
                ++it;
            }
        }
        return NMEASatelliteAdapter::expireSnapshot(snapshot, nowUs);
    };
    // Retire expired reports so a later health-policy change cannot revive cached metadata.
    _viewSnapshot = expire(_viewSnapshot, _viewRejectedThroughUs);
    _useSnapshot = expire(_useSnapshot, _useRejectedThroughUs);
    const auto view = _viewSnapshot;
    const auto used = _useSnapshot;
    const QPointer<NMEADecoderSession> guard(this);
    const QPointer<NMEASatelliteAdapter> adapter(_satelliteAdapter.get());
    _refreshingSatellites = true;
    _satellitesInView = view.satellites;
    _satellitesInUse = used.satellites;
    _satellitesReceivedAtUs = view.receivedAtUs;
    _satellitesUsedKnown = used.receivedAtUs != 0;
    _satelliteUseSystems.clear();
    static const QMap<QByteArray, QGeoSatelliteInfo::SatelliteSystem> systems = {{"GP", QGeoSatelliteInfo::GPS},
                                                                                 {"GL", QGeoSatelliteInfo::GLONASS},
                                                                                 {"GA", QGeoSatelliteInfo::GALILEO},
                                                                                 {"GB", QGeoSatelliteInfo::BEIDOU},
                                                                                 {"GQ", QGeoSatelliteInfo::QZSS}};
    for (auto it = used.constellationReceipts.cbegin(); it != used.constellationReceipts.cend(); ++it) {
        const auto system = systems.constFind(it.key());
        if (system != systems.cend()) {
            _satelliteUseSystems.insert(system.value());
        }
    }
    _health.updateSatellitesInView(view.satellites.size(),
                                   view.receivedAtUs ? static_cast<qint64>((nowUs - view.receivedAtUs) / 1000) : -1);
    if (guard && adapter && adapter == _satelliteAdapter.get()) {
        _health.updateSatellitesInUse(used.satellites.size(),
                                      used.receivedAtUs ? static_cast<qint64>((nowUs - used.receivedAtUs) / 1000) : -1);
        if (guard) {
            _refreshingSatellites = false;
            emit satellitesChanged();
        }
    }
}

void NMEADecoderSession::stop()
{
    ++_sessionId;
    _satellitePollTimer.stop();
    _positionSource.reset();
    _satelliteSource.reset();
    _satelliteAdapter.reset();
    _stream.reset();
    _viewSnapshot = {};
    _useSnapshot = {};
    _refreshingSatellites = false;
    _satellitesInView.clear();
    _satellitesInUse.clear();
    _satellitesReceivedAtUs = 0;
    _viewRejectedThroughUs = 0;
    _useRejectedThroughUs = 0;
    _satellitesUsedKnown = false;
    _satelliteUseSystems.clear();
    const QPointer<NMEADecoderSession> guard(this);
    _health.reset();
    if (guard) {
        emit satellitesChanged();
    }
}
