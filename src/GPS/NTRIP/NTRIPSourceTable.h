#pragma once

#include <deque>
#include <functional>

#include <QtCore/QByteArrayView>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRangeModel>
#include <QtCore/QString>
#include <QtCore/qnumeric.h>
#include <QtPositioning/QGeoCoordinate>

Q_DECLARE_LOGGING_CATEGORY(NTRIPSourceTableLog)

/// True once an ENDSOURCETABLE line arrives, with CRLF, LF, or no final line ending.
bool ntripSourceTableComplete(QByteArrayView body);

/// Parsed NTRIP source-table STR row. All fields are immutable after parse except distanceKm,
/// which is recomputed by updateDistances(). Each property is a model role of the same name.
struct NTRIPMountpoint
{
    Q_GADGET
    Q_PROPERTY(QString mountpoint MEMBER mountpoint)
    Q_PROPERTY(QString identifier MEMBER identifier)
    Q_PROPERTY(QString format MEMBER format)
    Q_PROPERTY(QString formatDetails MEMBER formatDetails)
    Q_PROPERTY(int carrier MEMBER carrier)
    Q_PROPERTY(QString navSystem MEMBER navSystem)
    Q_PROPERTY(QString network MEMBER network)
    Q_PROPERTY(QString country MEMBER country)
    Q_PROPERTY(double latitude MEMBER latitude)
    Q_PROPERTY(double longitude MEMBER longitude)
    Q_PROPERTY(bool nmea MEMBER nmea)
    Q_PROPERTY(bool solution MEMBER solution)
    Q_PROPERTY(QString generator MEMBER generator)
    Q_PROPERTY(QString compression MEMBER compression)
    Q_PROPERTY(QString authentication MEMBER authentication)
    Q_PROPERTY(bool fee MEMBER fee)
    Q_PROPERTY(int bitrate MEMBER bitrate)
    Q_PROPERTY(double distanceKm MEMBER distanceKm)

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

/// Read-only list model over the parsed source table; QML binds to the NTRIPMountpoint property names.
class NTRIPSourceTableModel : public QRangeModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

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

    // Rows change only through the owner's update functions.
    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        return QRangeModel::flags(index) & ~Qt::ItemIsEditable;
    }

    bool setData(const QModelIndex&, const QVariant&, int) override { return false; }

    bool setItemData(const QModelIndex&, const QMap<int, QVariant>&) override { return false; }

    bool clearItemData(const QModelIndex&) override { return false; }

    /// Publish parsed rows with distances and stable ordering in a single reset.
    void parseSourceTable(const QString& raw, const QGeoCoordinate& from = {});
    void updateDistances(const QGeoCoordinate& from);
    void clear();

signals:
    void countChanged();

private:
    friend class NTRIPSourceTableController;

    /// Reset observers may request another mutation; finish the current notification first.
    void _mutate(std::function<void()> mutation);
    static void _sortByDistance(QList<NTRIPMountpoint>& mountpoints);

    QList<NTRIPMountpoint> _mountpoints;
    std::deque<std::function<void()>> _pendingMutations;
    bool _mutating = false;
};
