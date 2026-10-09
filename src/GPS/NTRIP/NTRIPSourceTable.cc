#include "NTRIPSourceTable.h"

#include <algorithm>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/qnumeric.h>

bool ntripSourceTableComplete(QByteArrayView body, qsizetype checkedSize)
{
    static constexpr QByteArrayView terminator("ENDSOURCETABLE");
    // A terminator the earlier check missed ends past its data, so it starts no earlier than this.
    const qsizetype from = (std::max) (qsizetype(0), checkedSize - terminator.size());
    for (qsizetype offset = body.indexOf(terminator, from); offset >= 0;
         offset = body.indexOf(terminator, offset + 1)) {
        const qsizetype end = offset + terminator.size();
        if ((offset == 0 || body.at(offset - 1) == '\n') &&
            (end == body.size() || body.at(end) == '\r' || body.at(end) == '\n')) {
            return true;
        }
    }
    return false;
}

std::optional<NTRIPMountpoint> NTRIPMountpoint::fromSourceTableLine(const QString& line)
{
    // Need >= 18 fields; split keeps empty fields, so a trailing ';' is harmless
    // (the extra empty field parses as 0 / "" for its column).
    const QStringList fields = line.split(';');
    if (fields.size() < 18 || fields.at(0).trimmed().toUpper() != QStringLiteral("STR")) {
        return std::nullopt;
    }

    NTRIPMountpoint mp;
    mp.mountpoint = fields.at(1).trimmed();
    mp.format = fields.at(3).trimmed();
    mp.navSystem = fields.at(6).trimmed();
    mp.country = fields.at(8).trimmed();
    const auto parseCoord = [](const QString& s, double limit) -> double {
        bool ok = false;
        const double v = s.trimmed().toDouble(&ok);
        return (ok && qIsFinite(v) && qAbs(v) <= limit) ? v : qQNaN();
    };
    mp.latitude = parseCoord(fields.at(9), 90.0);
    mp.longitude = parseCoord(fields.at(10), 180.0);
    mp.bitrate = fields.at(17).trimmed().toInt();
    return mp;
}

void NTRIPMountpoint::updateDistance(const QGeoCoordinate& from)
{
    const QGeoCoordinate mountCoord(latitude, longitude);
    if (!from.isValid() || !mountCoord.isValid() || (latitude == 0.0 && longitude == 0.0)) {
        distanceKm = -1.0;
        return;
    }
    distanceKm = from.distanceTo(mountCoord) / 1000.0;
}

QString NTRIPMountpoint::details() const
{
    QStringList parts;
    for (const QString& part : {format, navSystem, country}) {
        if (!part.isEmpty()) {
            parts.append(part);
        }
    }
    if (bitrate > 0) {
        //: Mountpoint bitrate in bits per second
        parts.append(QCoreApplication::translate("NTRIPMountpoint", "%1 bps").arg(bitrate));
    }
    if (distanceKm >= 0) {
        //: Distance to the mountpoint in kilometers
        parts.append(QCoreApplication::translate("NTRIPMountpoint", "%1 km").arg(distanceKm, 0, 'f', 1));
    }
    //: Separator between mountpoint details
    return parts.join(QCoreApplication::translate("NTRIPMountpoint", " · "));
}

// ---------------------------------------------------------------------------
// NTRIPSourceTableModel
// ---------------------------------------------------------------------------

// A const range keeps the model read-only; the base only records its address and reads rows after construction.
NTRIPSourceTableModel::NTRIPSourceTableModel(QObject* parent)
    : QRangeModel(&std::as_const(_mountpoints), parent)
{}

void NTRIPSourceTableModel::parseSourceTable(const QString& raw, const QGeoCoordinate& from)
{
    QList<NTRIPMountpoint> mountpoints;
    const QStringList lines = raw.split('\n');
    for (const QString& line : lines) {
        if (auto mp = NTRIPMountpoint::fromSourceTableLine(line.trimmed())) {
            mp->updateDistance(from);
            mountpoints.append(*mp);
        }
    }
    beginResetModel();
    _mountpoints = std::move(mountpoints);
    endResetModel();
    emit countChanged();
}

void NTRIPSourceTableModel::updateDistances(const QGeoCoordinate& from)
{
    qsizetype firstChanged = -1;
    qsizetype lastChanged = -1;
    for (qsizetype row = 0; row < _mountpoints.size(); ++row) {
        NTRIPMountpoint& mountpoint = _mountpoints[row];
        const double previous = mountpoint.distanceKm;
        mountpoint.updateDistance(from);
        if (mountpoint.distanceKm != previous) {
            firstChanged = firstChanged < 0 ? row : firstChanged;
            lastChanged = row;
        }
    }
    if (firstChanged >= 0) {
        emit dataChanged(index(static_cast<int>(firstChanged), 0), index(static_cast<int>(lastChanged), 0),
                         {DistanceKmRole, DetailsRole});
    }
}

void NTRIPSourceTableModel::clear()
{
    if (_mountpoints.isEmpty()) {
        return;
    }
    beginResetModel();
    _mountpoints.clear();
    endResetModel();
    emit countChanged();
}

// ---------------------------------------------------------------------------
// NTRIPSourceTableSortModel
// ---------------------------------------------------------------------------

NTRIPSourceTableSortModel::NTRIPSourceTableSortModel(NTRIPSourceTableModel* source, QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source);
    setSortRole(NTRIPSourceTableModel::DistanceKmRole);
    sort(0);
    // Nothing is filtered, and the source announces its count after the proxy has processed the reset.
    connect(source, &NTRIPSourceTableModel::countChanged, this, &NTRIPSourceTableSortModel::countChanged);
}

bool NTRIPSourceTableSortModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const double leftKm = left.data(sortRole()).toDouble();
    const double rightKm = right.data(sortRole()).toDouble();
    if ((leftKm < 0) != (rightKm < 0)) {
        return rightKm < 0;
    }
    if (leftKm >= 0 && leftKm != rightKm) {
        return leftKm < rightKm;
    }
    // A strict order keeps equal distances in caster order, also when a distance update reinserts rows.
    return left.row() < right.row();
}
