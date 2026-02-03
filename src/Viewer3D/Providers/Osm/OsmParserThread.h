#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>

#include <vector>

Q_DECLARE_LOGGING_CATEGORY(OsmParserThreadLog)

class OsmParserThread : public QObject
{
    Q_OBJECT

    friend class OsmParserThreadTest;

public:
    struct BuildingType_t
    {
        std::vector<QGeoCoordinate> points_gps;
        std::vector<QGeoCoordinate> points_gps_inner;
        std::vector<QVector2D> points_local;
        std::vector<QVector2D> points_local_inner;
        QVector2D bb_max = QVector2D(-1e6, -1e6);
        QVector2D bb_min = QVector2D(1e6, 1e6);
        float height = 0;
        float levels = 0;

        void append(const std::vector<QGeoCoordinate> &newPoints, bool isInner);
        void append(const std::vector<QVector2D> &newPoints, bool isInner);
    };

    explicit OsmParserThread(QObject *parent = nullptr);
    ~OsmParserThread();

    void start(const QString &filePath);

    const QGeoCoordinate& gpsRefPoint() const { return _gpsRefPoint; }
    const QMap<uint64_t, QGeoCoordinate>& mapNodes() const { return _mapNodes; }
    const QMap<uint64_t, BuildingType_t>& mapBuildings() const { return _mapBuildings; }
    const QGeoCoordinate& coordinateMin() const { return _coordinateMin; }
    const QGeoCoordinate& coordinateMax() const { return _coordinateMax; }

signals:
    void fileParsed(bool isValid);
    void startThread(const QString &filePath);

private:
    void _parseOsmFile(const QString &filePath);

    QGeoCoordinate _gpsRefPoint;
    QMap<uint64_t, QGeoCoordinate> _mapNodes;
    QMap<uint64_t, BuildingType_t> _mapBuildings;
    QGeoCoordinate _coordinateMin;
    QGeoCoordinate _coordinateMax;

    QThread *_workerThread = nullptr;

    const QStringList _singleStoreyBuildings = {
        QStringLiteral("bungalow"),
        QStringLiteral("shed"),
        QStringLiteral("kiosk"),
        QStringLiteral("cabin")
    };
    const QStringList _doubleStoreyLeisure = {
        QStringLiteral("stadium"),
        QStringLiteral("sports_hall"),
        QStringLiteral("sauna")
    };

    bool _mapLoadedFlag = false;
};
