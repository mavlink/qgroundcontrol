#include "NTRIPSourceTable.h"

#include <QtCore/QPointer>

#include <algorithm>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(NTRIPSourceTableLog, "GPS.NTRIP.NTRIPSourceTable")

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
    bool latitudeOk = false;
    bool longitudeOk = false;
    const double latitude = fields.at(9).trimmed().toDouble(&latitudeOk);
    const double longitude = fields.at(10).trimmed().toDouble(&longitudeOk);
    const QGeoCoordinate coordinate(latitude, longitude);
    if (latitudeOk && longitudeOk && coordinate.isValid()) {
        mp.coordinate = coordinate;
    }
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

double NTRIPMountpoint::distanceFrom(const QGeoCoordinate& from) const
{
    return from.isValid() && coordinate.isValid() ? from.distanceTo(coordinate) / 1000.0 : -1.0;
}

// ---------------------------------------------------------------------------
// NTRIPSourceTableModel
// ---------------------------------------------------------------------------

NTRIPSourceTableModel::NTRIPSourceTableModel(QObject* parent) : QAbstractListModel(parent)
{
    qCDebug(NTRIPSourceTableLog) << this;
}

NTRIPSourceTableModel::~NTRIPSourceTableModel()
{
    qCDebug(NTRIPSourceTableLog) << this;
}

int NTRIPSourceTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant NTRIPSourceTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.model() != this || index.column() != 0 || index.row() >= count()) {
        return {};
    }
    const ProjectedRow& row = _projection.at(index.row());
    const NTRIPMountpoint& mp = _catalog.at(row.catalogIndex);
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
            return mp.coordinate.isValid() ? QVariant(mp.coordinate.latitude()) : QVariant();
        case LongitudeRole:
            return mp.coordinate.isValid() ? QVariant(mp.coordinate.longitude()) : QVariant();
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
            return row.distanceKm;
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
    QList<NTRIPMountpoint> catalog;
    const QStringList lines = raw.split('\n');
    for (const QString& line : lines) {
        NTRIPMountpoint mp;
        if (NTRIPMountpoint::fromSourceTableLine(line.trimmed(), mp)) {
            catalog.append(mp);
        }
    }
    const bool countDiffers = count() != catalog.size();
    const QPointer<NTRIPSourceTableModel> guard(this);
    beginResetModel();
    _catalog = std::move(catalog);
    _projection.clear();
    for (int i = 0; i < _catalog.size(); ++i) {
        _projection.append({i, -1.0});
    }
    endResetModel();
    if (guard && countDiffers) {
        emit countChanged();
    }
}

void NTRIPSourceTableModel::updateDistances(const QGeoCoordinate& from)
{
    QList<ProjectedRow> projection;
    for (int i = 0; i < _catalog.size(); ++i) {
        projection.append({i, _catalog.at(i).distanceFrom(from)});
    }
    std::stable_sort(projection.begin(), projection.end(), [](const ProjectedRow& a, const ProjectedRow& b) {
        if (a.distanceKm < 0) {
            return false;
        }
        return b.distanceKm < 0 || a.distanceKm < b.distanceKm;
    });
    bool reordered = false;
    bool distanceChanged = false;
    for (int i = 0; i < projection.size(); ++i) {
        reordered |= projection.at(i).catalogIndex != _projection.at(i).catalogIndex;
        distanceChanged |= projection.at(i).distanceKm != _projection.at(i).distanceKm;
    }
    if (reordered) {
        beginResetModel();
        _projection = std::move(projection);
        endResetModel();
    } else if (distanceChanged) {
        _projection = std::move(projection);
        emit dataChanged(index(0), index(count() - 1), {DistanceKmRole});
    }
}

void NTRIPSourceTableModel::clear()
{
    if (!_catalog.isEmpty()) {
        parseSourceTable({});
    }
}
