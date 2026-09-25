#include "NTRIPSourceTable.h"

#include <utility>

#include <QtCore/qnumeric.h>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableLog, "GPS.NTRIP.NTRIPSourceTable")

bool ntripSourceTableComplete(QByteArrayView body)
{
    static constexpr QByteArrayView terminator("ENDSOURCETABLE");
    for (qsizetype offset = body.indexOf(terminator); offset >= 0; offset = body.indexOf(terminator, offset + 1)) {
        const qsizetype end = offset + terminator.size();
        if ((offset == 0 || body.at(offset - 1) == '\n') &&
            (end == body.size() || body.at(end) == '\r' || body.at(end) == '\n')) {
            return true;
        }
    }
    return false;
}

bool NTRIPMountpoint::fromSourceTableLine(const QString& line, NTRIPMountpoint& out)
{
    // Need >= 18 fields; split keeps empty fields, so a trailing ';' is harmless
    // (the extra empty field parses as 0 / "" for its column).
    const QStringList fields = line.split(';');
    if (fields.size() < 18 || fields.at(0).trimmed().toUpper() != QStringLiteral("STR")) {
        return false;
    }

    NTRIPMountpoint mp;
    mp.mountpoint = fields.at(1).trimmed();
    mp.identifier = fields.at(2).trimmed();
    mp.format = fields.at(3).trimmed();
    mp.formatDetails = fields.at(4).trimmed();
    mp.carrier = fields.at(5).trimmed().toInt();
    mp.navSystem = fields.at(6).trimmed();
    mp.network = fields.at(7).trimmed();
    mp.country = fields.at(8).trimmed();
    const auto parseCoord = [](const QString& s, double limit) -> double {
        bool ok = false;
        const double v = s.trimmed().toDouble(&ok);
        return (ok && qIsFinite(v) && qAbs(v) <= limit) ? v : qQNaN();
    };
    mp.latitude = parseCoord(fields.at(9), 90.0);
    mp.longitude = parseCoord(fields.at(10), 180.0);
    mp.nmea = fields.at(11).trimmed() == QStringLiteral("1");
    mp.solution = fields.at(12).trimmed() == QStringLiteral("1");
    mp.generator = fields.at(13).trimmed();
    mp.compression = fields.at(14).trimmed();
    mp.authentication = fields.at(15).trimmed();
    mp.fee = fields.at(16).trimmed() == QStringLiteral("Y");
    mp.bitrate = fields.at(17).trimmed().toInt();

    out = mp;
    return true;
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
        NTRIPMountpoint mp;
        if (NTRIPMountpoint::fromSourceTableLine(line.trimmed(), mp)) {
            mp.updateDistance(from);
            mountpoints.append(mp);
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
                         {DistanceKmRole});
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
