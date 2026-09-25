#pragma once

#include <QtCore/QByteArrayView>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRangeModel>
#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtPositioning/QGeoCoordinate>

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableLog)

/// True once an ENDSOURCETABLE line arrives, with CRLF, LF, or no final line ending.
[[nodiscard]] bool ntripSourceTableComplete(QByteArrayView body);

/// Parsed NTRIP source-table STR row. All fields are immutable after parse except distanceKm,
/// which is recomputed by updateDistances(). Each property is a model role of the same name.
struct NTRIPMountpoint
{
    Q_GADGET
    Q_PROPERTY(QString mountpoint MEMBER mountpoint FINAL)
    Q_PROPERTY(QString identifier MEMBER identifier FINAL)
    Q_PROPERTY(QString format MEMBER format FINAL)
    Q_PROPERTY(QString formatDetails MEMBER formatDetails FINAL)
    Q_PROPERTY(int carrier MEMBER carrier FINAL)
    Q_PROPERTY(QString navSystem MEMBER navSystem FINAL)
    Q_PROPERTY(QString network MEMBER network FINAL)
    Q_PROPERTY(QString country MEMBER country FINAL)
    Q_PROPERTY(double latitude MEMBER latitude FINAL)
    Q_PROPERTY(double longitude MEMBER longitude FINAL)
    Q_PROPERTY(bool nmea MEMBER nmea FINAL)
    Q_PROPERTY(bool solution MEMBER solution FINAL)
    Q_PROPERTY(QString generator MEMBER generator FINAL)
    Q_PROPERTY(QString compression MEMBER compression FINAL)
    Q_PROPERTY(QString authentication MEMBER authentication FINAL)
    Q_PROPERTY(bool fee MEMBER fee FINAL)
    Q_PROPERTY(int bitrate MEMBER bitrate FINAL)
    Q_PROPERTY(double distanceKm MEMBER distanceKm FINAL)

public:
    QString mountpoint;
    QString identifier;
    QString format;
    QString formatDetails;
    int carrier = 0;
    QString navSystem;
    QString network;
    QString country;
    double latitude = qQNaN();
    double longitude = qQNaN();
    bool nmea = false;
    bool solution = false;
    QString generator;
    QString compression;
    QString authentication;
    bool fee = false;
    int bitrate = 0;
    double distanceKm = -1.0;

    /// Parse one source-table line ("STR;..."). Returns true and fills out on a
    /// valid STR row; returns false (out untouched) otherwise.
    static bool fromSourceTableLine(const QString& line, NTRIPMountpoint& out);

    /// Recompute distanceKm, or mark it unknown for invalid reference/mountpoint coordinates.
    void updateDistance(const QGeoCoordinate& from);
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
        IdentifierRole,
        FormatRole,
        FormatDetailsRole,
        CarrierRole,
        NavSystemRole,
        NetworkRole,
        CountryRole,
        LatitudeRole,
        LongitudeRole,
        NmeaRole,
        SolutionRole,
        GeneratorRole,
        CompressionRole,
        AuthenticationRole,
        FeeRole,
        BitrateRole,
        DistanceKmRole,
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
