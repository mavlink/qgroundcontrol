#include "GPSSatelliteModel.h"

#include <tuple>

#include "GPSReceiverMessages.h"

namespace {

bool sameSatellite(const GPSSatellite& first, const GPSSatellite& second)
{
    return first.constellation == second.constellation && first.id == second.id;
}

/// The order of a report's satellites.
bool precedes(const GPSSatellite& first, const GPSSatellite& second)
{
    return std::tuple(first.constellation, first.id) < std::tuple(second.constellation, second.id);
}

}  // namespace

GPSSatelliteModel::GPSSatelliteModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int GPSSatelliteModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant GPSSatelliteModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= _rows.size()) {
        return {};
    }
    const GPSSatellite& satellite = _rows.at(index.row()).satellite;
    switch (role) {
        case ConstellationRole:
            return static_cast<int>(satellite.constellation);
        case ConstellationNameRole:
            return GPSReceiverMessages::constellationName(satellite.constellation);
        case StartsConstellationRole:
            return _rows.at(index.row()).startsConstellation;
        case SatelliteIdRole:
            return satellite.id;
        case ElevationRole:
            return satellite.elevationDegrees;
        case AzimuthRole:
            return satellite.azimuthDegrees;
        case DirectionKnownRole:
            return satellite.directionKnown;
        case SignalStrengthRole:
            return satellite.signalDbHz;
        case UsedRole:
            return satellite.use == GPSSatellite::Use::Used;
        default:
            return {};
    }
}

QHash<int, QByteArray> GPSSatelliteModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{{ConstellationRole, "constellation"},
                                              {ConstellationNameRole, "constellationName"},
                                              {StartsConstellationRole, "startsConstellation"},
                                              {SatelliteIdRole, "satelliteId"},
                                              {ElevationRole, "elevation"},
                                              {AzimuthRole, "azimuth"},
                                              {DirectionKnownRole, "directionKnown"},
                                              {SignalStrengthRole, "signalStrength"},
                                              {UsedRole, "used"}};
    return roles;
}

bool GPSSatelliteModel::_startsConstellation(qsizetype row, const GPSSatellite& satellite) const
{
    return row == 0 || _rows.at(row - 1).satellite.constellation != satellite.constellation;
}

void GPSSatelliteModel::setSatellites(std::span<const GPSSatellite> satellites)
{
    const qsizetype previousCount = _rows.size();
    // The rows before `row` are the satellites before `next`.
    int row = 0;
    size_t next = 0;
    while (row < _rows.size() || next < satellites.size()) {
        const bool more = next < satellites.size();
        if (row < _rows.size() && more && sameSatellite(_rows.at(row).satellite, satellites[next])) {
            QList<int> roles;
            if (_rows.at(row).satellite != satellites[next]) {
                _rows[row].satellite = satellites[next];
                roles = {ElevationRole, AzimuthRole, DirectionKnownRole, SignalStrengthRole, UsedRole};
            }
            // A row that was removed or inserted before this one may have changed where its constellation starts.
            if (const bool starts = _startsConstellation(row, satellites[next]);
                starts != _rows.at(row).startsConstellation) {
                _rows[row].startsConstellation = starts;
                roles.append(StartsConstellationRole);
            }
            if (!roles.isEmpty()) {
                emit dataChanged(index(row), index(row), roles);
            }
            ++row;
            ++next;
        } else if (row < _rows.size() && (!more || precedes(_rows.at(row).satellite, satellites[next]))) {
            beginRemoveRows(QModelIndex(), row, row);
            _rows.removeAt(row);
            endRemoveRows();
        } else {
            beginInsertRows(QModelIndex(), row, row);
            _rows.insert(row, {.satellite = satellites[next],
                               .startsConstellation = _startsConstellation(row, satellites[next])});
            endInsertRows();
            ++row;
            ++next;
        }
    }
    if (_rows.size() != previousCount) {
        emit countChanged();
    }
}
