#include "GPSSatelliteModel.h"

#include <QtCore/QPointer>

#include <algorithm>
#include <cmath>
#include <tuple>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSatelliteModelLog, "GPS.Models.GPSSatelliteModel")

namespace {
auto satelliteKey(const GPSSatellite& satellite)
{
    return std::make_tuple(satellite.constellation, satellite.id, satellite.prn);
}
}  // namespace

GPSSatelliteModel::GPSSatelliteModel(QObject* parent)
    : QAbstractListModel(parent)
{
    qCDebug(GPSSatelliteModelLog) << this;
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
    return _satelliteData(_current.satellites[index.row()], role, _current.sourceId);
}

QVariant GPSSatelliteModel::_satelliteData(const GPSSatellite& satellite, int role, const QString& sourceId)
{
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
            return sourceId;
        default:
            return {};
    }
}

QHash<int, QByteArray> GPSSatelliteModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {{SatelliteIdRole, "satelliteId"},
                                                 {PrnRole, "prn"},
                                                 {ConstellationRole, "constellation"},
                                                 {UsedRole, "used"},
                                                 {ElevationRole, "elevation"},
                                                 {SignalStrengthRole, "signalStrength"},
                                                 {AzimuthRole, "azimuth"},
                                                 {SourceIdRole, "sourceId"}};
    return roles;
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
    if (_pending.sourceId.isEmpty() || observation.sessionId != _pending.sessionId ||
        observation.sourceId != _pending.sourceId || observation.revision < _pending.revision) {
        return;
    }
    _pending.revision = observation.revision;
    _pending.fresh = observation.satellitesInViewCount() >= 0;
    _pending.satellites = observation.satellites;
    std::stable_sort(_pending.satellites.begin(), _pending.satellites.end(),
                     [](const auto& left, const auto& right) { return satelliteKey(left) < satelliteKey(right); });
    _publish();
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
    const bool stateChangedValue = next.sourceId != _current.sourceId || next.sessionId != _current.sessionId ||
                                   next.fresh != _current.fresh || next.satellites.size() != _current.satellites.size();
    QList<std::pair<int, QList<int>>> changes;
    if (sameRows) {
        for (int row = 0; row < next.satellites.size(); ++row) {
            QList<int> roles;
            for (int role = SatelliteIdRole; role <= SourceIdRole; ++role) {
                if (_satelliteData(next.satellites[row], role, next.sourceId) !=
                    _satelliteData(_current.satellites[row], role, _current.sourceId)) {
                    roles.append(role);
                }
            }
            if (!roles.isEmpty()) {
                changes.append({row, roles});
            }
        }
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
    if (!sameRows) {
        endResetModel();
    } else {
        for (const auto& [row, roles] : changes) {
            if (!guard) {
                return;
            }
            emit dataChanged(index(row), index(row), roles);
        }
    }
    if (!guard) {
        return;
    }
    if (stateChangedValue) {
        emit stateChanged();
    }
    if (guard) {
        _publishing = false;
    }
}
