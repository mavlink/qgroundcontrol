#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>
#include <QtPositioning/QGeoCoordinate>

class QmlObjectListModel;
class QNetworkAccessManager;

class NTRIPMountpointModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString mountpoint   READ mountpoint   CONSTANT)
    Q_PROPERTY(QString identifier   READ identifier   CONSTANT)
    Q_PROPERTY(QString format       READ format       CONSTANT)
    Q_PROPERTY(QString formatDetails READ formatDetails CONSTANT)
    Q_PROPERTY(int     carrier      READ carrier      CONSTANT)
    Q_PROPERTY(QString navSystem    READ navSystem    CONSTANT)
    Q_PROPERTY(QString network      READ network      CONSTANT)
    Q_PROPERTY(QString country      READ country      CONSTANT)
    Q_PROPERTY(double  latitude     READ latitude     CONSTANT)
    Q_PROPERTY(double  longitude    READ longitude    CONSTANT)
    Q_PROPERTY(bool    nmea         READ nmea         CONSTANT)
    Q_PROPERTY(bool    solution     READ solution     CONSTANT)
    Q_PROPERTY(QString generator    READ generator    CONSTANT)
    Q_PROPERTY(QString compression  READ compression  CONSTANT)
    Q_PROPERTY(QString authentication READ authentication CONSTANT)
    Q_PROPERTY(bool    fee          READ fee          CONSTANT)
    Q_PROPERTY(int     bitrate      READ bitrate      CONSTANT)
    Q_PROPERTY(double  distanceKm   READ distanceKm   NOTIFY distanceKmChanged)

public:
    explicit NTRIPMountpointModel(QObject* parent = nullptr) : QObject(parent) {}

    QString mountpoint()     const { return _mountpoint; }
    QString identifier()     const { return _identifier; }
    QString format()         const { return _format; }
    QString formatDetails()  const { return _formatDetails; }
    int     carrier()        const { return _carrier; }
    QString navSystem()      const { return _navSystem; }
    QString network()        const { return _network; }
    QString country()        const { return _country; }
    double  latitude()       const { return _latitude; }
    double  longitude()      const { return _longitude; }
    bool    nmea()           const { return _nmea; }
    bool    solution()       const { return _solution; }
    QString generator()      const { return _generator; }
    QString compression()    const { return _compression; }
    QString authentication() const { return _authentication; }
    bool    fee()            const { return _fee; }
    int     bitrate()        const { return _bitrate; }
    double  distanceKm()     const { return _distanceKm; }

    static NTRIPMountpointModel* fromSourceTableLine(const QString& line, QObject* parent = nullptr);
    void updateDistance(const QGeoCoordinate& from);

signals:
    void distanceKmChanged();

private:
    QString _mountpoint;
    QString _identifier;
    QString _format;
    QString _formatDetails;
    int     _carrier = 0;
    QString _navSystem;
    QString _network;
    QString _country;
    double  _latitude = 0.0;
    double  _longitude = 0.0;
    bool    _nmea = false;
    bool    _solution = false;
    QString _generator;
    QString _compression;
    QString _authentication;
    bool    _fee = false;
    int     _bitrate = 0;
    double  _distanceKm = -1.0;
};

// ---------------------------------------------------------------------------
// NTRIPSourceTableModel
// ---------------------------------------------------------------------------

class NTRIPSourceTableModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QmlObjectListModel* mountpoints READ mountpoints CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit NTRIPSourceTableModel(QObject* parent = nullptr);

    QmlObjectListModel* mountpoints() const { return _mountpoints; }
    int count() const;

    void parseSourceTable(const QString& raw);
    void updateDistances(const QGeoCoordinate& from);
    void sortByDistance();
    void clear();

signals:
    void countChanged();

private:
    QmlObjectListModel* _mountpoints = nullptr;
};

// ---------------------------------------------------------------------------
// NTRIPSourceTableFetcher
// ---------------------------------------------------------------------------

class NTRIPSourceTableFetcher : public QObject
{
    Q_OBJECT

public:
    static constexpr int kFetchTimeoutMs = 10000;

    NTRIPSourceTableFetcher(const QString& host, int port,
                            const QString& username, const QString& password,
                            bool useTls = false,
                            QObject* parent = nullptr);
    ~NTRIPSourceTableFetcher() override = default;

    void fetch();

signals:
    void sourceTableReceived(const QString& table);
    void error(const QString& errorMsg);
    void finished();

private:
    void _onReplyFinished();

    QNetworkAccessManager* _networkManager = nullptr;
    QNetworkReply* _reply = nullptr;
    QString _host;
    int _port;
    QString _username;
    QString _password;
    bool _useTls = false;
};
