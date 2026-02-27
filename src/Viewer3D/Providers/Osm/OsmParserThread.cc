#include "OsmParserThread.h"

#include "QGCGeo.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <limits>

#include <osmium/handler.hpp>
#include <osmium/io/reader.hpp>
#include <osmium/io/xml_input.hpp>
#include <osmium/visitor.hpp>

#include <cmath>
#include <limits>

QGC_LOGGING_CATEGORY(OsmParserThreadLog, "Viewer3d.OsmParserThread")

// ============================================================================
// OsmBuildingHandler — libosmium streaming handler
// ============================================================================

class OsmBuildingHandler : public osmium::handler::Handler
{
public:
    OsmBuildingHandler(const QGeoCoordinate &gpsRef,
                       const QStringList &singleStorey,
                       const QStringList &doubleStoreyLeisure)
        : _gpsRef(gpsRef)
        , _singleStorey(singleStorey)
        , _doubleStoreyLeisure(doubleStoreyLeisure)
    {}

    void node(const osmium::Node &node)
    {
        const int64_t nodeId = node.id();
        if (nodeId <= 0) {
            return;
        }

        QGeoCoordinate coord;
        coord.setLatitude(node.location().lat());
        coord.setLongitude(node.location().lon());
        coord.setAltitude(0);

        nodes.insert(static_cast<uint64_t>(nodeId), coord);
    }

    void way(const osmium::Way &way)
    {
        const int64_t wayId = way.id();
        if (wayId == 0) {
            return;
        }

        OsmParserThread::BuildingType_t building;
        std::vector<QGeoCoordinate> gpsPoints;
        std::vector<QVector2D> localPoints;
        double lonMax = -1e10, lonMin = 1e10;
        double latMax = -1e10, latMin = 1e10;
        double xMax = -1e10, xMin = 1e10;
        double yMax = -1e10, yMin = 1e10;

        for (const auto &nr : way.nodes()) {
            const int64_t refId = nr.ref();
            if (refId <= 0) {
                continue;
            }

            auto it = nodes.constFind(static_cast<uint64_t>(refId));
            if (it == nodes.constEnd()) {
                continue;
            }

            const QGeoCoordinate &gpsCoord = it.value();
            gpsPoints.push_back(gpsCoord);
            const QVector3D localPt = QGCGeo::convertGpsToEnu(gpsCoord, _gpsRef);
            localPoints.push_back(QVector2D(localPt.x(), localPt.y()));

            xMax = std::fmax(xMax, localPt.x());
            yMax = std::fmax(yMax, localPt.y());
            xMin = std::fmin(xMin, localPt.x());
            yMin = std::fmin(yMin, localPt.y());

            lonMax = std::fmax(lonMax, gpsCoord.longitude());
            latMax = std::fmax(latMax, gpsCoord.latitude());
            lonMin = std::fmin(lonMin, gpsCoord.longitude());
            latMin = std::fmin(latMin, gpsCoord.latitude());
        }

        for (const auto &tag : way.tags()) {
            const QString key = QString::fromUtf8(tag.key());
            if (key == QStringLiteral("building:levels")) {
                building.levels = QString::fromUtf8(tag.value()).toFloat();
            } else if (key == QStringLiteral("height")) {
                building.height = QString::fromUtf8(tag.value()).toFloat();
            } else if (key == QStringLiteral("building") && building.levels == 0 && building.height == 0) {
                const QString value = QString::fromUtf8(tag.value());
                if (_singleStorey.contains(value)) {
                    building.levels = 1;
                } else {
                    building.levels = 2;
                }
            } else if (key == QStringLiteral("leisure") && building.levels == 0 && building.height == 0) {
                const QString value = QString::fromUtf8(tag.value());
                if (_doubleStoreyLeisure.contains(value)) {
                    building.levels = 2;
                }
            }
        }

        if (gpsPoints.size() > 2) {
            if (building.levels > 0 || building.height > 0) {
                coordMin.setLatitude(std::fmin(coordMin.latitude(), latMin));
                coordMin.setLongitude(std::fmin(coordMin.longitude(), lonMin));
                coordMax.setLatitude(std::fmax(coordMax.latitude(), latMax));
                coordMax.setLongitude(std::fmax(coordMax.longitude(), lonMax));
            }
            building.points_gps = gpsPoints;
            building.points_local = localPoints;
            building.bb_max = QVector2D(xMax, yMax);
            building.bb_min = QVector2D(xMin, yMin);
            buildings.insert(static_cast<uint64_t>(wayId), building);
        }
    }

    void relation(const osmium::Relation &relation)
    {
        const int64_t relationId = relation.id();
        if (relationId == 0) {
            return;
        }

        OsmParserThread::BuildingType_t building;
        std::vector<int64_t> idsToRemove;
        bool isBuilding = false;
        bool isMultipolygon = false;

        for (const auto &member : relation.members()) {
            if (member.type() == osmium::item_type::way) {
                const int64_t refId = member.ref();
                const QString role = QString::fromUtf8(member.role());
                auto bldItem = buildings.find(static_cast<uint64_t>(refId));
                if (bldItem != buildings.end()) {
                    building.append(bldItem.value().points_local, role == QStringLiteral("inner"));
                    building.append(bldItem.value().points_gps, role == QStringLiteral("inner"));
                    building.levels = std::fmax(building.levels, bldItem.value().levels);
                    building.height = std::fmax(building.height, bldItem.value().height);

                    building.bb_max[0] = std::fmax(building.bb_max[0], bldItem.value().bb_max[0]);
                    building.bb_max[1] = std::fmax(building.bb_max[1], bldItem.value().bb_max[1]);
                    building.bb_min[0] = std::fmin(building.bb_min[0], bldItem.value().bb_min[0]);
                    building.bb_min[1] = std::fmin(building.bb_min[1], bldItem.value().bb_min[1]);
                    idsToRemove.push_back(refId);
                }
            }
        }

        for (const auto &tag : relation.tags()) {
            const QString key = QString::fromUtf8(tag.key());
            if (key == QStringLiteral("type")) {
                if (QString::fromUtf8(tag.value()) == QStringLiteral("multipolygon")) {
                    isMultipolygon = true;
                }
            } else if (key == QStringLiteral("building")) {
                isBuilding = true;
            }
        }

        if (isBuilding) {
            if (building.height == 0) {
                building.levels = (building.levels == 0) ? 2 : building.levels;
            }
        }

        if (isMultipolygon && !idsToRemove.empty()) {
            for (int64_t id : idsToRemove) {
                buildings.remove(static_cast<uint64_t>(id));
            }
            buildings.insert(static_cast<uint64_t>(idsToRemove[0]), building);
        }
    }

    QMap<uint64_t, QGeoCoordinate> nodes;
    QMap<uint64_t, OsmParserThread::BuildingType_t> buildings;
    QGeoCoordinate coordMin;
    QGeoCoordinate coordMax;

private:
    const QGeoCoordinate &_gpsRef;
    const QStringList &_singleStorey;
    const QStringList &_doubleStoreyLeisure;
};

// ============================================================================
// BuildingType_t helpers
// ============================================================================

void OsmParserThread::BuildingType_t::append(const std::vector<QGeoCoordinate> &newPoints, bool isInner)
{
    auto &target = isInner ? points_gps_inner : points_gps;
    target.insert(target.end(), newPoints.begin(), newPoints.end());
}

void OsmParserThread::BuildingType_t::append(const std::vector<QVector2D> &newPoints, bool isInner)
{
    auto &target = isInner ? points_local_inner : points_local;
    target.insert(target.end(), newPoints.begin(), newPoints.end());
}

// ============================================================================
// OsmParserThread
// ============================================================================

OsmParserThread::OsmParserThread(QObject *parent)
    : QObject{parent}
    , _workerThread(new QThread())
{
    connect(this, &OsmParserThread::startThread, this, &OsmParserThread::_parseOsmFile);

    this->moveToThread(_workerThread);
    _workerThread->start();
}

OsmParserThread::~OsmParserThread()
{
    _workerThread->quit();
    _workerThread->wait();
    delete _workerThread;
}

void OsmParserThread::start(const QString &filePath)
{
    emit startThread(filePath);
}

void OsmParserThread::_parseOsmFile(const QString &filePath)
{
    _mapNodes.clear();
    _mapBuildings.clear();

    if (filePath.isEmpty()) {
        if (_mapLoadedFlag) {
            qCDebug(OsmParserThreadLog, "The 3D View has been cleared!");
        } else {
            qCDebug(OsmParserThreadLog, "No OSM File is selected!");
        }
        _mapLoadedFlag = false;
        return;
    }

    _mapLoadedFlag = false;

    QString resolvedPath = filePath;
#ifdef Q_OS_UNIX
    if (!QDir::isAbsolutePath(resolvedPath)) {
        resolvedPath = QStringLiteral("/") + filePath;
    }
#endif

    QFileInfo fileInfo(resolvedPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        qCDebug(OsmParserThreadLog) << "OSM file does not exist:" << resolvedPath;
        emit fileParsed(false);
        return;
    }
    const QString suffix = fileInfo.suffix().toLower();
    if (suffix != QStringLiteral("osm") && suffix != QStringLiteral("xml")) {
        qCDebug(OsmParserThreadLog) << "Invalid file extension:" << suffix;
        emit fileParsed(false);
        return;
    }

    try {
        osmium::io::File inputFile{resolvedPath.toStdString()};
        osmium::io::Reader reader{inputFile, osmium::osm_entity_bits::all};

        const auto &header = reader.header();
        bool hasHeaderBounds = !header.boxes().empty();
        if (hasHeaderBounds) {
            const auto &box = header.boxes().front();
            _coordinateMin = QGeoCoordinate(box.bottom_left().lat(), box.bottom_left().lon(), 0);
            _coordinateMax = QGeoCoordinate(box.top_right().lat(), box.top_right().lon(), 0);
            _gpsRefPoint = QGeoCoordinate(
                0.5 * (_coordinateMin.latitude() + _coordinateMax.latitude()),
                0.5 * (_coordinateMin.longitude() + _coordinateMax.longitude()), 0);
        }

        OsmBuildingHandler handler(_gpsRefPoint, _singleStoreyBuildings, _doubleStoreyLeisure);
        handler.coordMin = _coordinateMin;
        handler.coordMax = _coordinateMax;
        osmium::apply(reader, handler);
        reader.close();

        _mapNodes = std::move(handler.nodes);
        _mapBuildings = std::move(handler.buildings);
        if (!hasHeaderBounds) {
            // Some libosmium builds do not expose bounds in header for valid .osm files.
            if (_mapNodes.isEmpty()) {
                emit fileParsed(false);
                return;
            }

            double minLat = std::numeric_limits<double>::max();
            double minLon = std::numeric_limits<double>::max();
            double maxLat = std::numeric_limits<double>::lowest();
            double maxLon = std::numeric_limits<double>::lowest();

            for (auto it = _mapNodes.cbegin(); it != _mapNodes.cend(); ++it) {
                minLat = std::fmin(minLat, it.value().latitude());
                minLon = std::fmin(minLon, it.value().longitude());
                maxLat = std::fmax(maxLat, it.value().latitude());
                maxLon = std::fmax(maxLon, it.value().longitude());
            }

            _coordinateMin = QGeoCoordinate(minLat, minLon, 0);
            _coordinateMax = QGeoCoordinate(maxLat, maxLon, 0);
            _gpsRefPoint = QGeoCoordinate(0.5 * (minLat + maxLat), 0.5 * (minLon + maxLon), 0);

            // Building local coordinates were computed before fallback bounds were known.
            // Recompute with the finalized reference point for consistent geometry.
            for (auto it = _mapBuildings.begin(); it != _mapBuildings.end(); ++it) {
                BuildingType_t &building = it.value();
                building.points_local.clear();
                building.points_local_inner.clear();

                building.bb_max = QVector2D(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
                building.bb_min = QVector2D(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

                for (const QGeoCoordinate &gpsCoord : building.points_gps) {
                    const QVector3D localPt = QGCGeo::convertGpsToEnu(gpsCoord, _gpsRefPoint);
                    const QVector2D local2D(localPt.x(), localPt.y());
                    building.points_local.push_back(local2D);

                    building.bb_max[0] = std::fmax(building.bb_max[0], local2D.x());
                    building.bb_max[1] = std::fmax(building.bb_max[1], local2D.y());
                    building.bb_min[0] = std::fmin(building.bb_min[0], local2D.x());
                    building.bb_min[1] = std::fmin(building.bb_min[1], local2D.y());
                }

                for (const QGeoCoordinate &gpsCoord : building.points_gps_inner) {
                    const QVector3D localPt = QGCGeo::convertGpsToEnu(gpsCoord, _gpsRefPoint);
                    const QVector2D local2D(localPt.x(), localPt.y());
                    building.points_local_inner.push_back(local2D);

                    building.bb_max[0] = std::fmax(building.bb_max[0], local2D.x());
                    building.bb_max[1] = std::fmax(building.bb_max[1], local2D.y());
                    building.bb_min[0] = std::fmin(building.bb_min[0], local2D.x());
                    building.bb_min[1] = std::fmin(building.bb_min[1], local2D.y());
                }
            }
        } else {
            _coordinateMin = handler.coordMin;
            _coordinateMax = handler.coordMax;
        }

        _mapLoadedFlag = true;
        emit fileParsed(true);
    } catch (const std::exception &e) {
        qCDebug(OsmParserThreadLog) << "OSM parse error:" << e.what();
        emit fileParsed(false);
    }
}
