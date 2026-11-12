#include "NTRIPSourceTable.h"

#include <algorithm>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/qnumeric.h>

namespace {
/// Positions of the STR record fields the mountpoint list uses.
enum class StrField : qsizetype
{
    Type = 0,
    Mountpoint = 1,
    Identifier = 2,
    Format = 3,
    Messages = 4,
    Carrier = 5,
    NavSystem = 6,
    Country = 8,
    Latitude = 9,
    Longitude = 10,
    Nmea = 11,
    Authentication = 15,
    Bitrate = 17,
};

/// A control character, such as a bare carriage return left inside a field. NTRIPConnectionConfig refuses a
/// mountpoint that has one.
bool hasControlCharacter(const QString& text)
{
    return std::ranges::any_of(text, [](QChar ch) { return ch.unicode() < 0x20 || ch.unicode() == 0x7f; });
}

NTRIPMountpoint::Carrier carrierOf(const QString& field)
{
    using Carrier = NTRIPMountpoint::Carrier;
    return field == QLatin1String("0")   ? Carrier::None
           : field == QLatin1String("1") ? Carrier::L1
           : field == QLatin1String("2") ? Carrier::L1L2
                                         : Carrier::Unknown;
}

NTRIPMountpoint::Authentication authenticationOf(const QString& field)
{
    using Authentication = NTRIPMountpoint::Authentication;
    const QString letter = field.toUpper();
    return letter == QLatin1String("N")   ? Authentication::None
           : letter == QLatin1String("B") ? Authentication::Basic
           : letter == QLatin1String("D") ? Authentication::Digest
                                          : Authentication::Unknown;
}

/// Whether the format field names a format other than RTCM 3. Casters write that one as "RTCM 3.2", "RTCM3.1" or
/// "RTCM_3", so only letters and digits count. A row without the field, or with "RTCM" and no version, may send it
/// as well.
bool namesOtherFormat(const QString& format)
{
    QString name;
    for (const QChar ch : format) {
        if (ch.isLetterOrNumber()) {
            name.append(ch.toUpper());
        }
    }
    static const QLatin1String rtcm("RTCM");
    if (!name.startsWith(rtcm)) {
        return !name.isEmpty();
    }
    return name.size() > rtcm.size() && name.at(rtcm.size()).isDigit() && name.at(rtcm.size()) != QLatin1Char('3');
}

QString joinDetails(const QStringList& parts)
{
    //: Separator between mountpoint details
    return parts.join(QCoreApplication::translate("NTRIPMountpoint", " · "));
}
}  // namespace

bool ntripSourceTableComplete(QByteArrayView body, qsizetype checkedSize, bool ended)
{
    static constexpr QByteArrayView terminator("ENDSOURCETABLE");
    // A terminator the earlier check missed has its line ending past that data, so it starts no earlier than this.
    const qsizetype from = (std::max) (qsizetype(0), checkedSize - terminator.size());
    for (qsizetype offset = body.indexOf(terminator, from); offset >= 0;
         offset = body.indexOf(terminator, offset + 1)) {
        const qsizetype end = offset + terminator.size();
        // At the end of the bytes so far, the line may still go on.
        const bool lineEnds = end == body.size() ? ended : body.at(end) == '\r' || body.at(end) == '\n';
        if ((offset == 0 || body.at(offset - 1) == '\n') && lineEnds) {
            return true;
        }
    }
    return false;
}

std::optional<NTRIPMountpoint> NTRIPMountpoint::fromSourceTableLine(const QString& line)
{
    // split keeps empty fields, so a trailing ';' is harmless (the extra empty field parses as 0 / "" for its column).
    const QStringList fields = line.split(';');
    // Casters send shorter records too, such as RTKLIB's "STR;<mountpoint>;<text>": a field the record does not
    // reach is empty, and only the name is needed to connect.
    const auto field = [&fields](StrField position) {
        return fields.value(static_cast<qsizetype>(position)).trimmed();
    };
    const QString name = field(StrField::Mountpoint);
    // A control character does not show in the list: the name would look like one that can be requested, and
    // selecting it would save a name that every connection is refused for.
    if (field(StrField::Type).toUpper() != QStringLiteral("STR") || name.isEmpty() || hasControlCharacter(name)) {
        return std::nullopt;
    }

    NTRIPMountpoint mp;
    mp.mountpoint = name;
    mp.identifier = field(StrField::Identifier);
    mp.format = field(StrField::Format);
    mp.messages = field(StrField::Messages);
    mp.carrier = carrierOf(field(StrField::Carrier));
    mp.navSystem = field(StrField::NavSystem);
    mp.country = field(StrField::Country);
    const auto parseCoord = [](const QString& s, double limit) -> double {
        bool ok = false;
        const double v = s.trimmed().toDouble(&ok);
        return (ok && qIsFinite(v) && qAbs(v) <= limit) ? v : qQNaN();
    };
    mp.latitude = parseCoord(field(StrField::Latitude), 90.0);
    mp.longitude = parseCoord(field(StrField::Longitude), 180.0);
    mp.needsPosition = field(StrField::Nmea) == QLatin1String("1");
    mp.authentication = authenticationOf(field(StrField::Authentication));
    mp.bitrate = field(StrField::Bitrate).toInt();
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
    // Many casters only repeat the mountpoint name as its identifier.
    const bool described = identifier.compare(mountpoint, Qt::CaseInsensitive) != 0;
    QString carrierText;
    switch (carrier) {
        case Carrier::None:
            //: A mountpoint whose corrections do not cover the carrier phase, which an RTK fix needs
            carrierText = QCoreApplication::translate("NTRIPMountpoint", "no carrier phase");
            break;
        case Carrier::L1:
            carrierText = QStringLiteral("L1");
            break;
        case Carrier::L1L2:
            carrierText = QStringLiteral("L1+L2");
            break;
        case Carrier::Unknown:
            break;
    }
    QStringList parts;
    for (const QString& part : {described ? identifier : QString(), format, navSystem, carrierText, country}) {
        if (!part.isEmpty()) {
            parts.append(part);
        }
    }
    if (bitrate > 0) {
        //: Mountpoint bitrate in bits per second
        parts.append(QCoreApplication::translate("NTRIPMountpoint", "%1 bps").arg(bitrate));
    }
    return joinDetails(parts);
}

QString NTRIPMountpoint::notes() const
{
    QStringList parts;
    if (needsPosition) {
        //: A mountpoint that sends corrections only to a client that reports its position
        parts.append(QCoreApplication::translate("NTRIPMountpoint", "Needs your position"));
    }
    if (authentication == Authentication::Basic) {
        //: A mountpoint that asks for the username and password of an account
        parts.append(QCoreApplication::translate("NTRIPMountpoint", "Needs a login"));
    } else if (authentication == Authentication::None) {
        //: A mountpoint that asks for no username and password
        parts.append(QCoreApplication::translate("NTRIPMountpoint", "No login"));
    }
    if (!messages.isEmpty()) {
        //: %1 is the list of RTCM messages a mountpoint sends, as its caster writes it, such as 1005(10),1077(1)
        parts.append(QCoreApplication::translate("NTRIPMountpoint", "Messages %1").arg(messages));
    }
    return joinDetails(parts);
}

QString NTRIPMountpoint::problem() const
{
    QStringList problems;
    if (namesOtherFormat(format)) {
        //: %1 is the format a caster lists for a mountpoint, such as CMR+
        problems.append(
            QCoreApplication::translate("NTRIPMountpoint", "Listed as %1: only RTCM 3 corrections can be used.")
                .arg(format));
    }
    if (authentication == Authentication::Digest) {
        problems.append(
            QCoreApplication::translate("NTRIPMountpoint", "Needs Digest authentication, which is not supported."));
    }
    return problems.join(QLatin1Char(' '));
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
    , _source(source)
{
    setSourceModel(source);
    setSortRole(NTRIPSourceTableModel::DistanceKmRole);
    sort(0);
    // The table publishes its rows in one reset; the filter is the only other change of their number.
    connect(this, &QAbstractItemModel::modelReset, this, &NTRIPSourceTableSortModel::_announceCount);
}

void NTRIPSourceTableSortModel::_announceCount()
{
    if (std::exchange(_announcedCount, rowCount()) != rowCount()) {
        emit countChanged();
    }
}

void NTRIPSourceTableSortModel::setNameFilter(const QString& text)
{
    if (text == _nameFilter) {
        return;
    }
    beginFilterChange();
    _nameFilter = text;
    endFilterChange(Direction::Rows);
    // Once, for the rows the new filter lists: the view takes rows out and puts rows in one after the other.
    _announceCount();
}

bool NTRIPSourceTableSortModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    Q_UNUSED(sourceParent);
    // The spaces around what was typed are not part of a name.
    const QString text = _nameFilter.trimmed();
    const NTRIPMountpoint& mountpoint = _source->mountpoints().at(sourceRow);
    return mountpoint.mountpoint.contains(text, Qt::CaseInsensitive) ||
           mountpoint.identifier.contains(text, Qt::CaseInsensitive);
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
