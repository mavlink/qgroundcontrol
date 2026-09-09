#include "GPSSatelliteModel.h"

#include <QtCore/QPointer>
#include <QtCore/QSet>

#include <algorithm>
#include <cmath>
#include <tuple>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSatelliteModelLog, "GPS.Models.GPSSatelliteModel")

namespace {
GPSSatellite::Constellation constellationFor(QGeoSatelliteInfo::SatelliteSystem system)
{
    switch (system) {
        case QGeoSatelliteInfo::GPS:
            return GPSSatellite::Constellation::GPS;
        case QGeoSatelliteInfo::GLONASS:
            return GPSSatellite::Constellation::GLONASS;
        case QGeoSatelliteInfo::GALILEO:
            return GPSSatellite::Constellation::Galileo;
        case QGeoSatelliteInfo::BEIDOU:
            return GPSSatellite::Constellation::BeiDou;
        case QGeoSatelliteInfo::QZSS:
            return GPSSatellite::Constellation::QZSS;
        default:
            return GPSSatellite::Constellation::Unknown;
    }
}

auto satelliteKey(const GPSSatellite& satellite)
{
    return std::make_tuple(satellite.constellation, satellite.id, satellite.prn);
}
}  // namespace

GPSSatelliteModel::GPSSatelliteModel(QObject* parent, int freshnessTimeoutMs)
    : QAbstractListModel(parent)
    , _expiryTimer(this)
    , _freshnessTimeoutMs(std::max(1, freshnessTimeoutMs))
{
    qCDebug(GPSSatelliteModelLog) << this;
    _expiryTimer.setSingleShot(true);
    _expiryTimer.setTimerType(Qt::PreciseTimer);
    connect(&_expiryTimer, &QTimer::timeout, this, &GPSSatelliteModel::_expire);
}

GPSSatelliteModel::~GPSSatelliteModel()
{
    qCDebug(GPSSatelliteModelLog) << this;
}

int GPSSatelliteModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant GPSSatelliteModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0 || index.row() < 0 || index.row() >= count()) {
        return {};
    }
    const auto& satellite = _current.satellites[index.row()];
    switch (role) {
        case SatelliteIdRole:
            return satellite.id;
        case PrnRole:
            return satellite.prn > 0 ? QVariant(satellite.prn) : QVariant();
        case ConstellationRole:
            return _constellationName(satellite.constellation);
        case UsedRole:
            return satellite.used ? QVariant(*satellite.used) : QVariant();
        case ElevationRole:
            return satellite.elevationDegrees && std::isfinite(*satellite.elevationDegrees) &&
                           *satellite.elevationDegrees >= -90 && *satellite.elevationDegrees <= 90
                       ? QVariant(*satellite.elevationDegrees)
                       : QVariant();
        case SignalStrengthRole:
            return satellite.signalStrength && *satellite.signalStrength >= 0 ? QVariant(*satellite.signalStrength)
                                                                              : QVariant();
        case AzimuthRole: {
            const auto azimuth = satellite.azimuthDegrees();
            return azimuth ? QVariant(*azimuth) : QVariant();
        }
        case SourceIdRole:
            return _current.sourceId;
        default:
            return {};
    }
}

QHash<int, QByteArray> GPSSatelliteModel::roleNames() const
{
    return {{SatelliteIdRole, "satelliteId"},
            {PrnRole, "prn"},
            {ConstellationRole, "constellation"},
            {UsedRole, "used"},
            {ElevationRole, "elevation"},
            {SignalStrengthRole, "signalStrength"},
            {AzimuthRole, "azimuth"},
            {SourceIdRole, "sourceId"}};
}

QString GPSSatelliteModel::_constellationName(GPSSatellite::Constellation constellation)
{
    switch (constellation) {
        case GPSSatellite::Constellation::GPS:
            return QStringLiteral("GPS");
        case GPSSatellite::Constellation::GLONASS:
            return QStringLiteral("GLONASS");
        case GPSSatellite::Constellation::Galileo:
            return QStringLiteral("Galileo");
        case GPSSatellite::Constellation::BeiDou:
            return QStringLiteral("BeiDou");
        case GPSSatellite::Constellation::QZSS:
            return QStringLiteral("QZSS");
        case GPSSatellite::Constellation::SBAS:
            return QStringLiteral("SBAS");
        case GPSSatellite::Constellation::NavIC:
            return QStringLiteral("NavIC");
        default:
            return tr("Unknown");
    }
}

void GPSSatelliteModel::beginSession(const QString& sourceId, quint64 sessionId)
{
    if (_pending.sourceId == sourceId && _pending.sessionId == sessionId) {
        return;
    }
    _pending = {};
    _pending.sourceId = sourceId;
    _pending.sessionId = sessionId;
    _publish();
}

void GPSSatelliteModel::reset()
{
    _pending = {};
    _publish();
}

void GPSSatelliteModel::updateObservation(const GPSSatelliteObservation& observation)
{
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    if (_pending.sourceId.isEmpty() || observation.sessionId != _pending.sessionId ||
        !observation.monotonicTimestampUs || observation.monotonicTimestampUs > nowUs ||
        observation.monotonicTimestampUs < _pending.timestampUs ||
        (nowUs - observation.monotonicTimestampUs) >= static_cast<quint64>(_freshnessTimeoutMs) * 1000) {
        return;
    }
    _pending.timestampUs = observation.monotonicTimestampUs;
    _pending.fresh = true;
    _pending.satellites = observation.satellites;
    std::stable_sort(_pending.satellites.begin(), _pending.satellites.end(),
                     [](const auto& left, const auto& right) { return satelliteKey(left) < satelliteKey(right); });
    _publish();
}

void GPSSatelliteModel::updateNmeaSatellites(const QList<QGeoSatelliteInfo>& view, const QList<QGeoSatelliteInfo>& used,
                                             bool usedKnown, quint64 receivedAtUs, quint64 sessionId,
                                             const std::optional<QSet<int>>& usedSystems)
{
    if (!receivedAtUs) {
        if (sessionId == _pending.sessionId) {
            _pending.fresh = false;
            _pending.satellites.clear();
            _publish();
        }
        return;
    }
    GPSSatelliteObservation observation;
    observation.monotonicTimestampUs = receivedAtUs;
    observation.sessionId = sessionId;
    QSet<QPair<int, int>> usedIds;
    for (const auto& satellite : used) {
        usedIds.insert({satellite.satelliteSystem(), satellite.satelliteIdentifier()});
    }
    observation.satellites.reserve(view.size());
    for (const auto& satellite : view) {
        GPSSatellite converted;
        converted.id = satellite.satelliteIdentifier();
        converted.constellation = constellationFor(satellite.satelliteSystem());
        if (usedKnown && (!usedSystems || usedSystems->contains(satellite.satelliteSystem()))) {
            converted.used = usedIds.contains({satellite.satelliteSystem(), satellite.satelliteIdentifier()});
        }
        if (satellite.signalStrength() >= 0) {
            converted.signalStrength = satellite.signalStrength();
        }
        if (satellite.hasAttribute(QGeoSatelliteInfo::Elevation)) {
            converted.elevationDegrees = satellite.attribute(QGeoSatelliteInfo::Elevation);
        }
        if (satellite.hasAttribute(QGeoSatelliteInfo::Azimuth)) {
            converted.normalizedAzimuthDegrees = satellite.attribute(QGeoSatelliteInfo::Azimuth);
        }
        observation.satellites.append(converted);
    }
    updateObservation(observation);
}

void GPSSatelliteModel::_armTimer()
{
    if (!_current.fresh) {
        _expiryTimer.stop();
        return;
    }
    const qint64 remaining = _freshnessTimeoutMs - GPSObservation::ageMilliseconds(_current.timestampUs);
    _expiryTimer.start(static_cast<int>(std::max<qint64>(1, remaining)));
}

void GPSSatelliteModel::_expire()
{
    if (_pending.fresh && GPSObservation::ageMilliseconds(_pending.timestampUs) >= _freshnessTimeoutMs) {
        _pending.fresh = false;
        _pending.satellites.clear();
        _publish();
    } else {
        _armTimer();
    }
}

void GPSSatelliteModel::_publish()
{
    if (_publishing) {
        // Coalesce callbacks during model notifications instead of nesting begin/end model operations.
        if (!_publicationQueued) {
            _publicationQueued = true;
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    _publicationQueued = false;
                    _publish();
                },
                Qt::QueuedConnection);
        }
        return;
    }
    const Snapshot next = _pending;
    bool sameRows = next.satellites.size() == _current.satellites.size();
    for (qsizetype index = 0; sameRows && index < next.satellites.size(); ++index) {
        sameRows = satelliteKey(next.satellites[index]) == satelliteKey(_current.satellites[index]);
    }
    const QPointer<GPSSatelliteModel> guard(this);
    _publishing = true;
    if (!sameRows) {
        beginResetModel();
        if (!guard) {
            return;
        }
    }
    _current = next;
    _armTimer();
    if (!sameRows) {
        endResetModel();
    } else if (count() > 0) {
        emit dataChanged(index(0), index(count() - 1));
    }
    if (!guard) {
        return;
    }
    emit stateChanged();
    if (guard) {
        _publishing = false;
    }
}
