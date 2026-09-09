#include "NMEADecoderSession.h"

#include <QtCore/QIODevice>
#include <QtCore/QPointer>
#include <QtPositioning/QNmeaSatelliteInfoSource>

#include "NMEAPositionSource.h"
#include "NMEASatelliteAdapter.h"
#include "NMEAStreamSplitter.h"
#include "NMEAUtils.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NMEADecoderSessionLog, "GPS.NMEA.NMEADecoderSession")

NMEADecoderSession::NMEADecoderSession(QObject* parent)
    : QObject(parent)
    , _satellitePollTimer(this)
    , _health(this)
    , _satellites(this)
{
    qCDebug(NMEADecoderSessionLog) << this;
    _satellitePollTimer.setInterval(1000);
    connect(&_satellitePollTimer, &QTimer::timeout, this, [this]() {
        const QPointer<NMEADecoderSession> guard(this);
        _satellites.setFreshnessTimeoutMs(_health.freshnessTimeoutMs());
        if (guard && _satelliteSource) {
            // One-shot requests also report unchanged lists, unlike continuous Qt satellite updates.
            _satelliteSource->requestUpdate(5000);
        }
    });
    connect(&_health, &GPSSourceHealth::satellitesChanged, this, &NMEADecoderSession::satellitesChanged);
    connect(&_satellites, &GPSSatelliteStore::observationChanged, this,
            [this](const GPSSatelliteObservation& observation) {
                const QPointer<NMEADecoderSession> guard(this);
                _health.applySatelliteObservation(observation);
                if (guard && observation.sessionId == _sessionId) {
                    emit satellitesReceived(observation);
                }
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
                const auto snapshot = _satelliteAdapter->satelliteSnapshot(satellites, false);
                QMetaObject::invokeMethod(
                    this,
                    [this, current, snapshot]() {
                        if (!current || _satelliteSource.get() != current) {
                            return;
                        }
                        _viewSnapshot = snapshot;
                        _updateSatellites();
                    },
                    Qt::QueuedConnection);
                // Keep the next one-shot request armed while Qt parses bursts between polls.
                if (current) {
                    current->requestUpdate(5000);
                }
            });
    connect(_satelliteSource.get(), &QGeoSatelliteInfoSource::satellitesInUseUpdated, this,
            [this, current](const QList<QGeoSatelliteInfo>& satellites) {
                const auto snapshot = _satelliteAdapter->satelliteSnapshot(satellites, true);
                QMetaObject::invokeMethod(
                    this,
                    [this, current, snapshot]() {
                        if (!current || _satelliteSource.get() != current) {
                            return;
                        }
                        _useSnapshot = snapshot;
                        _updateSatellites();
                    },
                    Qt::QueuedConnection);
                // Keep the next one-shot request armed while Qt parses bursts between polls.
                if (current) {
                    current->requestUpdate(5000);
                }
            });
    connect(
        _satelliteSource.get(), &QGeoSatelliteInfoSource::errorOccurred, this,
        [this, current](QGeoSatelliteInfoSource::Error error) {
            if (current && _satelliteSource.get() == current && error != QGeoSatelliteInfoSource::NoError) {
                _viewSnapshot = {};
                _useSnapshot = {};
                _satellites.clear();
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

void NMEADecoderSession::_updateSatellites()
{
    GPSSatelliteObservation observation;
    observation.sessionId = _sessionId;
    observation.sourceId = QStringLiteral("nmeaReceiver");
    std::map<GPSSatellite::Constellation, GPSSatelliteProvenance> reports;
    for (auto it = _viewSnapshot.constellationReceipts.cbegin(); it != _viewSnapshot.constellationReceipts.cend();
         ++it) {
        const auto constellation = NMEAUtils::satelliteConstellation(it.key());
        if (constellation != GPSSatellite::Constellation::Unknown) {
            reports[constellation].constellation = constellation;
            reports[constellation].inViewTimestampUs = it.value();
        }
    }
    std::map<GPSSatellite::Constellation, QSet<int>> usedIds;
    for (auto it = _useSnapshot.constellationReceipts.cbegin(); it != _useSnapshot.constellationReceipts.cend(); ++it) {
        const auto constellation = NMEAUtils::satelliteConstellation(it.key());
        if (constellation == GPSSatellite::Constellation::Unknown) {
            continue;
        }
        auto& report = reports[constellation];
        report.constellation = constellation;
        report.inUseTimestampUs = it.value();
        usedIds[constellation] = _useSnapshot.usedIds.value(it.key());
        report.satellitesUsed = static_cast<int>(usedIds[constellation].size());
    }
    for (const auto& satellite : _viewSnapshot.satellites) {
        GPSSatellite converted;
        converted.id = satellite.satelliteIdentifier();
        converted.constellation = NMEAUtils::satelliteConstellation(satellite.satelliteSystem());
        const auto report = reports.find(converted.constellation);
        if (report == reports.cend() || !report->second.inViewTimestampUs) {
            continue;
        }
        if (report->second.inUseTimestampUs) {
            converted.used = usedIds[converted.constellation].contains(satellite.satelliteIdentifier());
        }
        if (satellite.signalStrength() >= 0)
            converted.signalStrength = satellite.signalStrength();
        if (satellite.hasAttribute(QGeoSatelliteInfo::Elevation))
            converted.elevationDegrees = satellite.attribute(QGeoSatelliteInfo::Elevation);
        if (satellite.hasAttribute(QGeoSatelliteInfo::Azimuth))
            converted.normalizedAzimuthDegrees = satellite.attribute(QGeoSatelliteInfo::Azimuth);
        observation.satellites.append(converted);
    }
    for (const auto& [constellation, report] : reports) {
        observation.provenance.append(report);
    }
    const QPointer<NMEADecoderSession> guard(this);
    const quint64 session = _sessionId;
    _satellites.setFreshnessTimeoutMs(_health.freshnessTimeoutMs());
    if (guard && session == _sessionId) {
        _satellites.updateObservation(observation);
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
    const QPointer<NMEADecoderSession> guard(this);
    _health.reset();
    if (guard) {
        _satellites.beginSession(QStringLiteral("nmeaReceiver"), _sessionId);
    }
}
