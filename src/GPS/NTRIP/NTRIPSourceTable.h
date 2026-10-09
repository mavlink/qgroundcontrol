#pragma once

#include <optional>

#include <QtCore/QByteArrayView>
#include <QtCore/QList>
#include <QtCore/QRangeModel>
#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtPositioning/QGeoCoordinate>

/// True once an ENDSOURCETABLE line arrives, with CRLF, LF, or no final line ending. When the first @a checkedSize
/// bytes were already checked, only a terminator that may have completed since is searched for.
[[nodiscard]] bool ntripSourceTableComplete(QByteArrayView body, qsizetype checkedSize = 0);

/// The parsed fields of an NTRIP source-table STR row that the mountpoint list shows or sorts by. All fields are
/// immutable after parse except distanceKm, which is recomputed by updateDistances(). Each property is a model role of
/// the same name.
struct NTRIPMountpoint
{
    Q_GADGET
    Q_PROPERTY(QString mountpoint MEMBER mountpoint FINAL)
    Q_PROPERTY(QString format MEMBER format FINAL)
    Q_PROPERTY(QString navSystem MEMBER navSystem FINAL)
    Q_PROPERTY(QString country MEMBER country FINAL)
    Q_PROPERTY(int bitrate MEMBER bitrate FINAL)
    Q_PROPERTY(double distanceKm MEMBER distanceKm FINAL)
    /// The format, navigation systems, country, bitrate and distance that are known, as one line.
    Q_PROPERTY(QString details READ details FINAL)

public:
    QString mountpoint;
    QString format;
    QString navSystem;
    QString country;
    int bitrate = 0;
    double distanceKm = -1.0;
    double latitude = qQNaN();
    double longitude = qQNaN();

    /// Parses one source-table line ("STR;..."); none unless it is a valid STR row.
    static std::optional<NTRIPMountpoint> fromSourceTableLine(const QString& line);

    /// Recompute distanceKm, or mark it unknown for invalid reference/mountpoint coordinates.
    void updateDistance(const QGeoCoordinate& from);

    QString details() const;
};

template <>
struct QRangeModel::RowOptions<NTRIPMountpoint>
{
    static constexpr auto rowCategory = QRangeModel::RowCategory::MultiRoleItem;
};

/// Read-only list model over the parsed source table in caster order; NTRIPSourceTableSortModel orders it by
/// distance. QML binds to the NTRIPMountpoint property names.
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
    };

    explicit NTRIPSourceTableModel(QObject* parent = nullptr);

    int count() const { return static_cast<int>(_mountpoints.size()); }

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
/// among equal distances. Distance updates reorder rows without resetting the view.
class NTRIPSourceTableSortModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    explicit NTRIPSourceTableSortModel(NTRIPSourceTableModel* source, QObject* parent = nullptr);

    int count() const { return rowCount(); }

signals:
    void countChanged();

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};
