#include "NTRIPSourceTable.h"

#include <QtCore/qnumeric.h>
#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableLog, "GPS.NTRIPSourceTable")

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
    // Caster-supplied coordinates are untrusted; out-of-range/non-finite values
    // collapse to 0.0, which updateDistance() treats as "unknown" and skips.
    const auto parseCoord = [](const QString& s, double limit) -> double {
        bool ok = false;
        const double v = s.trimmed().toDouble(&ok);
        return (ok && qIsFinite(v) && qAbs(v) <= limit) ? v : 0.0;
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
    if (!from.isValid() || (latitude == 0.0 && longitude == 0.0)) {
        return;
    }
    const QGeoCoordinate mountCoord(latitude, longitude);
    distanceKm = from.distanceTo(mountCoord) / 1000.0;
}

// ---------------------------------------------------------------------------
// NTRIPSourceTableModel
// ---------------------------------------------------------------------------

NTRIPSourceTableModel::NTRIPSourceTableModel(QObject* parent) : QAbstractListModel(parent) {}

int NTRIPSourceTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant NTRIPSourceTableModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= _mountpoints.size()) {
        return {};
    }
    const NTRIPMountpoint& mp = _mountpoints.at(index.row());
    switch (role) {
        case MountpointRole:
            return mp.mountpoint;
        case IdentifierRole:
            return mp.identifier;
        case FormatRole:
            return mp.format;
        case FormatDetailsRole:
            return mp.formatDetails;
        case CarrierRole:
            return mp.carrier;
        case NavSystemRole:
            return mp.navSystem;
        case NetworkRole:
            return mp.network;
        case CountryRole:
            return mp.country;
        case LatitudeRole:
            return mp.latitude;
        case LongitudeRole:
            return mp.longitude;
        case NmeaRole:
            return mp.nmea;
        case SolutionRole:
            return mp.solution;
        case GeneratorRole:
            return mp.generator;
        case CompressionRole:
            return mp.compression;
        case AuthenticationRole:
            return mp.authentication;
        case FeeRole:
            return mp.fee;
        case BitrateRole:
            return mp.bitrate;
        case DistanceKmRole:
            return mp.distanceKm;
        default:
            return {};
    }
}

QHash<int, QByteArray> NTRIPSourceTableModel::roleNames() const
{
    return {
        {MountpointRole, "mountpoint"},
        {IdentifierRole, "identifier"},
        {FormatRole, "format"},
        {FormatDetailsRole, "formatDetails"},
        {CarrierRole, "carrier"},
        {NavSystemRole, "navSystem"},
        {NetworkRole, "network"},
        {CountryRole, "country"},
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
        {NmeaRole, "nmea"},
        {SolutionRole, "solution"},
        {GeneratorRole, "generator"},
        {CompressionRole, "compression"},
        {AuthenticationRole, "authentication"},
        {FeeRole, "fee"},
        {BitrateRole, "bitrate"},
        {DistanceKmRole, "distanceKm"},
    };
}

void NTRIPSourceTableModel::parseSourceTable(const QString& raw)
{
    beginResetModel();
    _mountpoints.clear();

    const QStringList lines = raw.split('\n');
    for (const QString& line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("ENDSOURCETABLE"))) {
            continue;
        }
        NTRIPMountpoint mp;
        if (NTRIPMountpoint::fromSourceTableLine(trimmed, mp)) {
            _mountpoints.append(mp);
        }
    }

    endResetModel();
    emit countChanged();
}

void NTRIPSourceTableModel::updateDistances(const QGeoCoordinate& from)
{
    for (NTRIPMountpoint& mp : _mountpoints) {
        mp.updateDistance(from);
    }
    sortByDistance();
}

void NTRIPSourceTableModel::sortByDistance()
{
    if (_mountpoints.size() < 2) {
        return;
    }

    // Distance ordering: known distances ascending, unknown (negative) last.
    const auto less = [](const NTRIPMountpoint& a, const NTRIPMountpoint& b) {
        if (a.distanceKm < 0 && b.distanceKm < 0)
            return false;
        if (a.distanceKm < 0)
            return false;  // a unknown → sorts after known b
        if (b.distanceKm < 0)
            return true;   // b unknown → known a sorts before
        return a.distanceKm < b.distanceKm;
    };

    beginResetModel();
    std::stable_sort(_mountpoints.begin(), _mountpoints.end(), less);
    endResetModel();
    // No countChanged() here: a sort reorders rows but never changes the count.
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
