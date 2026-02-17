#include "OsmParser.h"

#include "OsmParserThread.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Viewer3DSettings.h"

#include <mapbox/earcut.hpp>

#include <array>

QGC_LOGGING_CATEGORY(OsmParserLog, "Viewer3d.OsmParser")

OsmParser::OsmParser(QObject *parent)
    : Viewer3DMapProvider{parent}
    , _osmParserWorker(new OsmParserThread(this))
{
    Viewer3DSettings* viewer3DSettings = SettingsManager::instance()->viewer3DSettings();
    _setBuildingLevelHeight(viewer3DSettings->buildingLevelHeight()->rawValue());
    connect(viewer3DSettings->buildingLevelHeight(), &Fact::rawValueChanged, this, &OsmParser::_setBuildingLevelHeight);
    connect(_osmParserWorker, &OsmParserThread::fileParsed, this, &OsmParser::_onOsmParserFinished);
}

void OsmParser::setGpsRef(const QGeoCoordinate &gpsRef)
{
    _gpsRefPoint = gpsRef;
    _gpsRefSet = true;
    emit gpsRefChanged(_gpsRefPoint, _gpsRefSet);
}

void OsmParser::resetGpsRef()
{
    _gpsRefPoint = QGeoCoordinate(0, 0, 0);
    _gpsRefSet = false;
    emit gpsRefChanged(_gpsRefPoint, _gpsRefSet);
}

void OsmParser::_setBuildingLevelHeight(const QVariant &value)
{
    _buildingLevelHeight = value.toFloat();
    emit buildingLevelHeightChanged();
}

void OsmParser::_onOsmParserFinished(bool isValid)
{
    if (isValid) {
        if (!_gpsRefSet) {
            setGpsRef(_osmParserWorker->gpsRefPoint());

            _coordinateMin = _osmParserWorker->coordinateMin();
            _coordinateMax = _osmParserWorker->coordinateMax();
        }
        _mapLoadedFlag = true;
        emit mapChanged();
        qCDebug(OsmParserLog) << _osmParserWorker->mapBuildings().size() << "buildings loaded";
    }
}

void OsmParser::parseOsmFile(const QString &filePath)
{
    _gpsRefSet = false;
    _mapLoadedFlag = false;
    resetGpsRef();

    _osmParserWorker->start(filePath);
}

QByteArray OsmParser::buildingToMesh()
{
    QByteArray vertexData;
    const auto &buildings = _osmParserWorker->mapBuildings();
    vertexData.reserve(static_cast<qsizetype>(buildings.size()) * 1000);

    for (auto it = buildings.begin(), end = buildings.end(); it != end; ++it) {
        const auto &building = it.value();
        float buildingHeight = 0;

        std::vector<std::array<float, 2>> allPoints;
        std::vector<std::array<float, 2>> ringPoints;
        std::vector<std::vector<std::array<float, 2>>> polygon;
        std::vector<QVector3D> mesh;

        if (building.height > 0) {
            buildingHeight = building.height;
        } else if (building.levels > 0) {
            buildingHeight = static_cast<float>(building.levels) * _buildingLevelHeight;
        } else {
            continue;
        }

        for (const auto &pt : building.points_local) {
            ringPoints.push_back({pt.x(), pt.y()});
            allPoints.push_back({pt.x(), pt.y()});
        }
        polygon.push_back(ringPoints);

        ringPoints.clear();
        for (const auto &pt : building.points_local_inner) {
            ringPoints.push_back({pt.x(), pt.y()});
            allPoints.push_back({pt.x(), pt.y()});
        }
        if (!ringPoints.empty()) {
            polygon.push_back(ringPoints);
        }

        const std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(polygon);

        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t idx = indices[i];
            mesh.push_back(QVector3D(allPoints[idx][0], allPoints[idx][1], buildingHeight));
            idx = indices[i + 1];
            mesh.push_back(QVector3D(allPoints[idx][0], allPoints[idx][1], buildingHeight));
            idx = indices[i + 2];
            mesh.push_back(QVector3D(allPoints[idx][0], allPoints[idx][1], buildingHeight));

            idx = indices[i + 2];
            mesh.push_back(QVector3D(allPoints[idx][0], allPoints[idx][1], 0));
            idx = indices[i + 1];
            mesh.push_back(QVector3D(allPoints[idx][0], allPoints[idx][1], 0));
            idx = indices[i];
            mesh.push_back(QVector3D(allPoints[idx][0], allPoints[idx][1], 0));
        }

        if (buildingHeight > 0) {
            _triangulateWallsExtrudedPolygon(mesh, building.points_local, buildingHeight, false);
            _triangulateWallsExtrudedPolygon(mesh, building.points_local, buildingHeight, true);

            _triangulateWallsExtrudedPolygon(mesh, building.points_local_inner, buildingHeight, false);
            _triangulateWallsExtrudedPolygon(mesh, building.points_local_inner, buildingHeight, true);
        }

        QByteArray buildingData(mesh.size() * 3 * sizeof(float), Qt::Initialization::Uninitialized);
        float *p = reinterpret_cast<float *>(buildingData.data());

        for (const auto &vertex : mesh) {
            *p++ = vertex.x();
            *p++ = vertex.y();
            *p++ = vertex.z();
        }

        vertexData.append(buildingData);
    }
    return vertexData;
}

void OsmParser::_triangulateWallsExtrudedPolygon(std::vector<QVector3D> &triangulatedMesh, const std::vector<QVector2D> &verticesCcw, float h, bool inverseOrder)
{
    std::vector<QVector3D> quad(4);
    const size_t verticesSize = verticesCcw.size();

    if (inverseOrder) {
        for (size_t i = 0; i < verticesSize; i++) {
            const size_t next = (i + 1 < verticesSize) ? (i + 1) : size_t{0};
            quad[0] = QVector3D(verticesCcw[next].x(), verticesCcw[next].y(), 0);
            quad[1] = QVector3D(verticesCcw[i].x(), verticesCcw[i].y(), 0);
            quad[2] = QVector3D(verticesCcw[i].x(), verticesCcw[i].y(), h);
            quad[3] = QVector3D(verticesCcw[next].x(), verticesCcw[next].y(), h);
            _triangulateRectangle(triangulatedMesh, quad, false);
        }
        _triangulateRectangle(triangulatedMesh, quad, true);
    } else {
        for (size_t i = 0; i < verticesSize; i++) {
            const size_t next = (i + 1 < verticesSize) ? (i + 1) : size_t{0};
            quad[0] = QVector3D(verticesCcw[i].x(), verticesCcw[i].y(), 0);
            quad[1] = QVector3D(verticesCcw[next].x(), verticesCcw[next].y(), 0);
            quad[2] = QVector3D(verticesCcw[next].x(), verticesCcw[next].y(), h);
            quad[3] = QVector3D(verticesCcw[i].x(), verticesCcw[i].y(), h);
            _triangulateRectangle(triangulatedMesh, quad, false);
        }
        _triangulateRectangle(triangulatedMesh, quad, true);
    }
}

void OsmParser::_triangulateRectangle(std::vector<QVector3D> &triangulatedMesh, const std::vector<QVector3D> &verticesCcw, bool invertNormal)
{
    using Vec3i = std::array<unsigned int, 3>;
    std::array<Vec3i, 2> meshIndices;

    if (invertNormal) {
        meshIndices[0] = {3, 1, 0};
        meshIndices[1] = {3, 2, 1};
    } else {
        meshIndices[0] = {0, 1, 3};
        meshIndices[1] = {1, 2, 3};
    }

    for (const auto &tri : meshIndices) {
        for (unsigned int vi : tri) {
            triangulatedMesh.push_back(verticesCcw[vi]);
        }
    }
}
