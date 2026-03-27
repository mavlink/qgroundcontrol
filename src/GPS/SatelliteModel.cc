#include "SatelliteModel.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QStringList>

QGC_LOGGING_CATEGORY(SatelliteModelLog, "GPS.SatelliteModel")

SatelliteModel::SatelliteModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SatelliteModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : _satellites.size();
}

QVariant SatelliteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= _satellites.size()) {
        return {};
    }
    const SatEntry &s = _satellites.at(index.row());
    switch (role) {
    case ConstellationRole: return s.constellation;
    case SvidRole:          return s.svid;
    case SnrRole:           return s.snr;
    case ElevationRole:     return s.elevation;
    case AzimuthRole:       return s.azimuth;
    case UsedRole:          return s.used;
    default:                return {};
    }
}

QHash<int, QByteArray> SatelliteModel::roleNames() const
{
    return {
        {ConstellationRole, "constellation"},
        {SvidRole,          "svid"},
        {SnrRole,           "snr"},
        {ElevationRole,     "elevation"},
        {AzimuthRole,       "azimuth"},
        {UsedRole,          "used"},
    };
}

void SatelliteModel::updateFromDriverInfo(const satellite_info_s &info)
{
    const int newCount = qMin(static_cast<int>(info.count),
                              static_cast<int>(satellite_info_s::SAT_INFO_MAX_SATELLITES));
    const int oldCount = _satellites.size();

    // Build the new entries and track used-satellite counts for the summary.
    QList<SatEntry> newSats;
    newSats.reserve(newCount);
    QMap<QString, int> usedCounts;
    int usedCount = 0;

    for (int i = 0; i < newCount; ++i) {
        SatEntry e;
        e.svid          = info.svid[i];
        e.used          = info.used[i] != 0;
        e.elevation     = info.elevation[i];
        e.azimuth       = info.azimuth[i];
        e.snr           = info.snr[i];
        e.constellation = QString::fromUtf8(
            satelliteSystemName(satelliteSystemFromSvid(info.svid[i], info.prn[i])));
        newSats.append(e);
        if (e.used) {
            ++usedCount;
            usedCounts[e.constellation]++;
        }
    }

    // Adjust the model row count incrementally so views animate correctly.
    if (newCount > oldCount) {
        // Update existing rows in-place first, then insert the new tail.
        for (int i = 0; i < oldCount; ++i) {
            _satellites[i] = newSats[i];
        }
        if (oldCount > 0) {
            // Emit the base-class signal explicitly to avoid ambiguity with the
            // no-arg SatelliteModel::dataChanged() property-notification signal.
            emit QAbstractListModel::dataChanged(index(0), index(oldCount - 1));
        }
        beginInsertRows(QModelIndex(), oldCount, newCount - 1);
        for (int i = oldCount; i < newCount; ++i) {
            _satellites.append(newSats[i]);
        }
        endInsertRows();
        emit countChanged();
    } else if (newCount < oldCount) {
        // Remove trailing rows first, then update the remaining entries.
        beginRemoveRows(QModelIndex(), newCount, oldCount - 1);
        _satellites.resize(newCount);
        endRemoveRows();
        emit countChanged();
        for (int i = 0; i < newCount; ++i) {
            _satellites[i] = newSats[i];
        }
        if (newCount > 0) {
            emit QAbstractListModel::dataChanged(index(0), index(newCount - 1));
        }
    } else {
        // Row count unchanged — replace data in-place.
        _satellites = newSats;
        if (newCount > 0) {
            emit QAbstractListModel::dataChanged(index(0), index(newCount - 1));
        }
    }

    _usedCount = usedCount;
    _buildSummary(usedCounts);
    emit modelDataChanged();
}

void SatelliteModel::updateFromQtPositioning(const QList<QGeoSatelliteInfo> &inView,
                                             const QList<QGeoSatelliteInfo> &inUse)
{
    // Build the in-use identifier set for O(1) lookup.
    QSet<int> inUseIds;
    inUseIds.reserve(inUse.size());
    for (const QGeoSatelliteInfo &sat : inUse) {
        inUseIds.insert(sat.satelliteIdentifier());
    }

    // Qt Positioning gives full snapshots — reset is simpler and correct.
    const int oldCount = _satellites.size();

    beginResetModel();
    _satellites.clear();
    _satellites.reserve(inView.size());

    QMap<QString, int> usedCounts;
    int usedCount = 0;

    for (const QGeoSatelliteInfo &sat : inView) {
        SatEntry e;
        e.svid          = sat.satelliteIdentifier();
        e.used          = inUseIds.contains(sat.satelliteIdentifier());
        e.snr           = sat.signalStrength();
        e.elevation     = sat.hasAttribute(QGeoSatelliteInfo::Elevation)
                              ? static_cast<int>(sat.attribute(QGeoSatelliteInfo::Elevation))
                              : 0;
        e.azimuth       = sat.hasAttribute(QGeoSatelliteInfo::Azimuth)
                              ? static_cast<int>(sat.attribute(QGeoSatelliteInfo::Azimuth))
                              : 0;
        e.constellation = _qtSystemName(sat.satelliteSystem());
        _satellites.append(e);
        if (e.used) {
            ++usedCount;
            usedCounts[e.constellation]++;
        }
    }
    endResetModel();

    _usedCount = usedCount;
    _buildSummary(usedCounts);

    if (_satellites.size() != oldCount) {
        emit countChanged();
    }
    emit modelDataChanged();
}

void SatelliteModel::clear()
{
    if (_satellites.isEmpty()) {
        return;
    }
    beginRemoveRows(QModelIndex(), 0, _satellites.size() - 1);
    _satellites.clear();
    endRemoveRows();
    _usedCount = 0;
    _constellationSummary.clear();
    emit countChanged();
    emit modelDataChanged();
}

void SatelliteModel::_buildSummary(const QMap<QString, int> &counts)
{
    // Emit in a fixed, human-readable order with abbreviated labels.
    static const QStringList order = {
        QStringLiteral("GPS"),     QStringLiteral("GLONASS"), QStringLiteral("Galileo"),
        QStringLiteral("BeiDou"),  QStringLiteral("SBAS"),    QStringLiteral("QZSS"),
        QStringLiteral("Multi"),   QStringLiteral("Other"),   QStringLiteral("Unknown"),
    };

    QStringList parts;
    for (const QString &name : order) {
        if (!counts.contains(name)) {
            continue;
        }
        QString abbrev = name;
        if (name == QStringLiteral("GLONASS"))  abbrev = QStringLiteral("GLO");
        else if (name == QStringLiteral("Galileo")) abbrev = QStringLiteral("GAL");
        else if (name == QStringLiteral("BeiDou"))  abbrev = QStringLiteral("BDS");
        parts.append(abbrev + QStringLiteral(":") + QString::number(counts[name]));
    }
    _constellationSummary = parts.join(QStringLiteral("  "));
}

QString SatelliteModel::_qtSystemName(QGeoSatelliteInfo::SatelliteSystem system)
{
    switch (system) {
    case QGeoSatelliteInfo::GPS:      return QStringLiteral("GPS");
    case QGeoSatelliteInfo::GLONASS:  return QStringLiteral("GLONASS");
    case QGeoSatelliteInfo::GALILEO:  return QStringLiteral("Galileo");
    case QGeoSatelliteInfo::BEIDOU:   return QStringLiteral("BeiDou");
    case QGeoSatelliteInfo::QZSS:     return QStringLiteral("QZSS");
    case QGeoSatelliteInfo::Multiple: return QStringLiteral("Multi");
    default:                          return QStringLiteral("Unknown");
    }
}
