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
    const ProjectedRow& row = _current.projection.at(index.row());
    const NTRIPMountpoint& mp = _current.catalog.at(row.catalogIndex);
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
    _pending.catalog = std::move(catalog);
    ++_pending.catalogRevision;
    _pending.projection.clear();
    for (int i = 0; i < _pending.catalog.size(); ++i) {
        _pending.projection.append({i, -1.0});
    }
    _publish();
}

void NTRIPSourceTableModel::updateDistances(const QGeoCoordinate& from)
{
    QList<ProjectedRow> projection;
    for (int i = 0; i < _pending.catalog.size(); ++i) {
        projection.append({i, _pending.catalog.at(i).distanceFrom(from)});
    }
    std::stable_sort(projection.begin(), projection.end(), [](const ProjectedRow& a, const ProjectedRow& b) {
        if (a.distanceKm < 0) {
            return false;
        }
        return b.distanceKm < 0 || a.distanceKm < b.distanceKm;
    });
    _pending.projection = std::move(projection);
    _publish();
}

void NTRIPSourceTableModel::clear()
{
    if (!_pending.catalog.isEmpty()) {
        parseSourceTable({});
    }
}

void NTRIPSourceTableModel::_publish()
{
    if (_publishing) {
        // Keep catalog and projection updates together across reentrant model notifications.
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
    const bool countDiffers = _current.projection.size() != next.projection.size();
    bool reset = _current.catalogRevision != next.catalogRevision || countDiffers;
    bool distanceChanged = false;
    for (qsizetype i = 0; !reset && i < next.projection.size(); ++i) {
        reset = next.projection.at(i).catalogIndex != _current.projection.at(i).catalogIndex;
        distanceChanged |= next.projection.at(i).distanceKm != _current.projection.at(i).distanceKm;
    }
    if (!reset && !distanceChanged) {
        return;
    }
    const QPointer<NTRIPSourceTableModel> guard(this);
    _publishing = true;
    if (reset) {
        beginResetModel();
        if (!guard) {
            return;
        }
    }
    _current = next;
    if (reset) {
        endResetModel();
    } else if (distanceChanged) {
        emit dataChanged(index(0), index(count() - 1), {DistanceKmRole});
    }
    if (!guard) {
        return;
    }
    if (countDiffers) {
        emit countChanged();
        if (!guard) {
            return;
        }
    }
    _publishing = false;
}
