#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableLog)

/// Parsed NTRIP source-table STR row. Plain value type — all fields are immutable
/// after parse except distanceKm, which is recomputed by updateDistances().
struct NTRIPMountpoint
{
    QString mountpoint;
    QString identifier;
    QString format;
    QString formatDetails;
    int carrier = 0;
    QString navSystem;
    QString network;
    QString country;
    double latitude = 0.0;
    double longitude = 0.0;
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

    /// Recompute distanceKm from a reference coordinate. No-op for invalid
    /// references or unknown (0,0) mountpoint coordinates.
    void updateDistance(const QGeoCoordinate& from);
};

/// Single QAbstractListModel over the parsed source table. Replaces the former
/// QObject-per-row + QmlObjectListModel double-wrapping. QML binds against the
/// role names below (mountpoint, format, distanceKm, ...).
class NTRIPSourceTableModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles
    {
        MountpointRole = Qt::UserRole + 1,
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

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void parseSourceTable(const QString& raw);
    void updateDistances(const QGeoCoordinate& from);
    void sortByDistance();
    void clear();

signals:
    void countChanged();

private:
    QList<NTRIPMountpoint> _mountpoints;
};
