#pragma once

#include <optional>

#include <QtCore/QByteArrayView>
#include <QtCore/QList>
#include <QtCore/QRangeModel>
#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtPositioning/QGeoCoordinate>

/// True once an ENDSOURCETABLE line has arrived. The first byte of its line ending, CRLF or LF, shows that the line is
/// whole, so the answer does not depend on how the caster's bytes are split; a body that has @a ended may also end
/// with the line itself. When the first @a checkedSize bytes were already checked, only a terminator that may have
/// completed since is searched for.
[[nodiscard]] bool ntripSourceTableComplete(QByteArrayView body, qsizetype checkedSize = 0, bool ended = false);

/// The parsed fields of an NTRIP source-table STR row that the mountpoint list shows, sorts or filters by. All fields
/// are immutable after parse except distanceKm, which is recomputed by updateDistances(). Each property is a model role
/// of the same name. Only the mountpoint name is required: a field the row does not have is empty, zero or unknown.
struct NTRIPMountpoint
{
    Q_GADGET
    Q_PROPERTY(QString mountpoint MEMBER mountpoint FINAL)
    Q_PROPERTY(QString format MEMBER format FINAL)
    Q_PROPERTY(QString navSystem MEMBER navSystem FINAL)
    Q_PROPERTY(QString country MEMBER country FINAL)
    Q_PROPERTY(int bitrate MEMBER bitrate FINAL)
    Q_PROPERTY(double distanceKm MEMBER distanceKm FINAL)
    /// The identifier, format, navigation systems, carrier, country and bitrate that are known, as one line.
    Q_PROPERTY(QString details READ details FINAL)
    /// What the mountpoint asks of a client, a position or a login, and the messages it lists, as one line.
    Q_PROPERTY(QString notes READ notes FINAL)
    /// Why this client cannot use the mountpoint as the caster lists it; empty when nothing in the row rules it out.
    Q_PROPERTY(QString problem READ problem FINAL)

public:
    /// The carrier phases the corrections cover. Without them a rover gets a differential fix at best, not an RTK one.
    enum class Carrier
    {
        Unknown,
        None,
        L1,
        L1L2,
    };

    /// How the caster lets a client have the stream.
    enum class Authentication
    {
        Unknown,
        None,
        Basic,
        /// HTTP Digest, which this client does not offer.
        Digest,
    };

    QString mountpoint;
    /// The caster's description of the source, usually the nearest town.
    QString identifier;
    QString format;
    /// The stream's messages as the caster lists them, such as "1005(10),1077(1)": each with its interval in seconds.
    QString messages;
    Carrier carrier = Carrier::Unknown;
    QString navSystem;
    QString country;
    /// The caster serves the stream only to a client that reports its position (NMEA GGA), as a network RTK (VRS)
    /// mountpoint does.
    bool needsPosition = false;
    Authentication authentication = Authentication::Unknown;
    int bitrate = 0;
    double distanceKm = -1.0;
    double latitude = qQNaN();
    double longitude = qQNaN();

    /// Parses one source-table line ("STR;..."); none unless it is an STR row that names a mountpoint.
    static std::optional<NTRIPMountpoint> fromSourceTableLine(const QString& line);

    /// Recompute distanceKm, or mark it unknown for invalid reference/mountpoint coordinates.
    void updateDistance(const QGeoCoordinate& from);

    QString details() const;
    QString notes() const;
    QString problem() const;
};

template <>
struct QRangeModel::RowOptions<NTRIPMountpoint>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

/// Read-only list model over the parsed source table in caster order; NTRIPSourceTableSortModel orders it by
/// distance and filters it by name. QML binds to the NTRIPMountpoint property names.
class NTRIPSourceTableModel : public QRangeModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    /// Roles follow the NTRIPMountpoint property order, as QRangeModel assigns them.
    enum Roles
    {
        MountpointRole = Qt::UserRole,
        FormatRole,
        NavSystemRole,
        CountryRole,
        BitrateRole,
        DistanceKmRole,
        DetailsRole,
        NotesRole,
        ProblemRole,
    };

    explicit NTRIPSourceTableModel(QObject* parent = nullptr);

    int count() const { return static_cast<int>(_mountpoints.size()); }

    /// The rows in caster order.
    const QList<NTRIPMountpoint>& mountpoints() const { return _mountpoints; }

    /// Publish the parsed rows and their distances in a single reset.
    void parseSourceTable(const QString& raw, const QGeoCoordinate& from = {});
    /// Recompute distances; one dataChanged for DistanceKmRole spans the rows whose distance changed.
    void updateDistances(const QGeoCoordinate& from);
    void clear();

signals:
    void countChanged();

private:
    QList<NTRIPMountpoint> _mountpoints;
};

/// Distance-ordered view of a source table: known distances ascending, unknown distances last, and caster order
/// among equal distances. Distance updates reorder rows without resetting the view. A name filter narrows the view
/// to the mountpoints that match it.
class NTRIPSourceTableSortModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    explicit NTRIPSourceTableSortModel(NTRIPSourceTableModel* source, QObject* parent = nullptr);

    int count() const { return rowCount(); }

    const QString& nameFilter() const { return _nameFilter; }

    /// Lists only the mountpoints whose name or identifier contains @a text, whatever the case of either; text
    /// without anything but spaces lists them all.
    void setNameFilter(const QString& text);

signals:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    void _announceCount();

    NTRIPSourceTableModel* const _source;
    QString _nameFilter;
    /// The row count the signal last announced.
    int _announcedCount = 0;
};
