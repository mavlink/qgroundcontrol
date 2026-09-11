#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtPositioning/QGeoCoordinate>

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableLog)

/// Parsed catalog entry. Reference-position distances belong to the model projection.
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
    QGeoCoordinate coordinate;
    bool nmea = false;
    bool solution = false;
    QString generator;
    QString compression;
    QString authentication;
    bool fee = false;
    int bitrate = 0;

    /// Parse one source-table line ("STR;..."). Returns true and fills out on a
    /// valid STR row; returns false (out untouched) otherwise.
    static bool fromSourceTableLine(const QString& line, NTRIPMountpoint& out);

    /// Unknown catalog or reference coordinates return -1.
    double distanceFrom(const QGeoCoordinate& from) const;
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
    ~NTRIPSourceTableModel() override;

    int count() const { return static_cast<int>(_current.projection.size()); }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void parseSourceTable(const QString& raw);
    void updateDistances(const QGeoCoordinate& from);
    void clear();

signals:
    void countChanged();

private:
    struct ProjectedRow
    {
        int catalogIndex;
        double distanceKm;
    };

    struct Snapshot
    {
        QList<NTRIPMountpoint> catalog;
        QList<ProjectedRow> projection;
        quint64 catalogRevision = 0;
    };

    void _publish();
    Snapshot _current;
    Snapshot _pending;
    bool _publishing = false;
    bool _publicationQueued = false;
};
