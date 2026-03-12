#include "RTKSatelliteModel.h"

#include <QtCore/QMap>

RTKSatelliteModel::RTKSatelliteModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RTKSatelliteModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _satellites.size();
}

QVariant RTKSatelliteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= _satellites.size()) {
        return {};
    }
    const auto &s = _satellites.at(index.row());
    switch (role) {
    case SvidRole:          return s.svid;
    case UsedRole:          return s.used != 0;
    case ElevationRole:     return s.elevation;
    case AzimuthRole:       return s.azimuth;
    case SnrRole:           return s.snr;
    case ConstellationRole: return s.constellation;
    default:                return {};
    }
}

QHash<int, QByteArray> RTKSatelliteModel::roleNames() const
{
    return {
        {SvidRole,          "svid"},
        {UsedRole,          "used"},
        {ElevationRole,     "elevation"},
        {AzimuthRole,       "azimuth"},
        {SnrRole,           "snr"},
        {ConstellationRole, "constellation"},
    };
}

QString RTKSatelliteModel::_constellation(uint8_t svid, uint8_t prn)
{
    // NMEA/u-blox SVID ranges
    if (svid >= 1 && svid <= 32)   return QStringLiteral("GPS");
    if (svid >= 33 && svid <= 64)  return QStringLiteral("SBAS");
    if (svid >= 65 && svid <= 96)  return QStringLiteral("GLONASS");
    if (svid >= 120 && svid <= 158) return QStringLiteral("SBAS");
    if (svid >= 159 && svid <= 163) return QStringLiteral("BeiDou");
    if (svid >= 193 && svid <= 202) return QStringLiteral("QZSS");
    if (svid >= 211 && svid <= 246) return QStringLiteral("Galileo");
    if (svid >= 247) return QStringLiteral("BeiDou");
    // Fallback: use PRN-based detection
    if (prn >= 1 && prn <= 32)     return QStringLiteral("GPS");
    if (prn >= 65 && prn <= 96)    return QStringLiteral("GLONASS");
    if (prn >= 120 && prn <= 158)  return QStringLiteral("SBAS");
    if (prn >= 159 && prn <= 195)  return QStringLiteral("BeiDou");
    if (prn >= 211 && prn <= 246)  return QStringLiteral("Galileo");
    return QStringLiteral("Other");
}

void RTKSatelliteModel::update(const satellite_info_s &info)
{
    beginResetModel();
    _satellites.clear();
    _satellites.reserve(info.count);

    QMap<QString, int> counts;
    _usedCount = 0;

    for (int i = 0; i < qMin(static_cast<int>(info.count), static_cast<int>(satellite_info_s::SAT_INFO_MAX_SATELLITES)); ++i) {
        SatEntry e;
        e.svid = info.svid[i];
        e.used = info.used[i];
        e.elevation = info.elevation[i];
        e.azimuth = info.azimuth[i];
        e.snr = info.snr[i];
        e.constellation = _constellation(info.svid[i], info.prn[i]);
        _satellites.append(e);
        if (e.used) {
            ++_usedCount;
            counts[e.constellation]++;
        }
    }

    // Build summary like "GPS:10 GLO:8 GAL:6"
    QStringList parts;
    static const QStringList order = {
        QStringLiteral("GPS"), QStringLiteral("GLONASS"), QStringLiteral("Galileo"),
        QStringLiteral("BeiDou"), QStringLiteral("SBAS"), QStringLiteral("QZSS"), QStringLiteral("Other")
    };
    for (const auto &name : order) {
        if (counts.contains(name)) {
            QString abbrev = name;
            if (name == QStringLiteral("GLONASS")) abbrev = QStringLiteral("GLO");
            else if (name == QStringLiteral("Galileo")) abbrev = QStringLiteral("GAL");
            else if (name == QStringLiteral("BeiDou")) abbrev = QStringLiteral("BDS");
            parts.append(abbrev + QStringLiteral(":") + QString::number(counts[name]));
        }
    }
    _constellationSummary = parts.join(QStringLiteral("  "));

    endResetModel();
    emit satelliteDataChanged();
}

void RTKSatelliteModel::clear()
{
    update(satellite_info_s{});
}
