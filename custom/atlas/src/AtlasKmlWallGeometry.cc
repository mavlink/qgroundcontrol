#include "AtlasKmlWallGeometry.h"

#include <QtCore/QByteArray>
#include <QtCore/QPointF>
#include <QtCore/qminmax.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include "GeoScene.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"

namespace {
constexpr double kMaxSegmentMeters = 50.0;
constexpr int kMaxSubdivisionsPerEdge = 64;
constexpr int kFloatsPerVertex = 3;

bool finiteAltitude(double value)
{
    return std::isfinite(value);
}
}

AtlasKmlWallGeometry::AtlasKmlWallGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    _rebuild();
}

void AtlasKmlWallGeometry::setScene(GeoScene *scene)
{
    if (_scene == scene) {
        return;
    }

    if (_scene) {
        disconnect(_scene, nullptr, this, nullptr);
    }

    _scene = scene;

    if (_scene) {
        connect(_scene, &GeoScene::sceneOriginChanged, this, &AtlasKmlWallGeometry::_scheduleRebuild);
        connect(_scene, &QObject::destroyed, this, [this] {
            _scene = nullptr;
            emit sceneChanged();
            _scheduleRebuild();
        });
    }

    emit sceneChanged();
    _scheduleRebuild();
}

void AtlasKmlWallGeometry::setSurfaceModel(SurfacePatchModel *surfaceModel)
{
    if (_surfaceModel == surfaceModel) {
        return;
    }

    if (_surfaceModel) {
        disconnect(_surfaceModel, nullptr, this, nullptr);
    }

    _surfaceModel = surfaceModel;

    if (_surfaceModel) {
        connect(_surfaceModel, &SurfacePatchModel::terrainHeightsChanged, this, &AtlasKmlWallGeometry::_scheduleRebuild);
        connect(_surfaceModel, &QObject::destroyed, this, [this] {
            _surfaceModel = nullptr;
            emit surfaceModelChanged();
            _scheduleRebuild();
        });
    }

    emit surfaceModelChanged();
    _scheduleRebuild();
}

void AtlasKmlWallGeometry::setCoordinates(const QVariantList &coordinates)
{
    if (_coordinates == coordinates) {
        return;
    }

    _coordinates = coordinates;
    _path.clear();
    _path.reserve(coordinates.size());

    for (const QVariant &value : coordinates) {
        const QGeoCoordinate coordinate = value.value<QGeoCoordinate>();
        if (coordinate.isValid()) {
            _path.append(coordinate);
        }
    }

    if (_path.size() > 1 && _path.first().distanceTo(_path.last()) < 0.01) {
        _path.removeLast();
    }

    emit coordinatesChanged();
    _scheduleRebuild();
}

void AtlasKmlWallGeometry::setAltitudeMode(int mode)
{
    const int value = std::clamp(mode, 0, 2);
    if (_altitudeMode == value) {
        return;
    }

    _altitudeMode = value;
    emit altitudeModeChanged();
    _scheduleRebuild();
}

void AtlasKmlWallGeometry::setClosed(bool closed)
{
    if (_closed == closed) {
        return;
    }

    _closed = closed;
    emit closedChanged();
    _scheduleRebuild();
}

void AtlasKmlWallGeometry::_scheduleRebuild()
{
    if (_rebuildPending) {
        return;
    }

    _rebuildPending = true;
    QMetaObject::invokeMethod(this, &AtlasKmlWallGeometry::_rebuild, Qt::QueuedConnection);
}

void AtlasKmlWallGeometry::_rebuild()
{
    _rebuildPending = false;

    const int minimumPoints = _closed ? 3 : 2;

    if (!_scene || _altitudeMode == 0 || _path.size() < minimumPoints) {
        _setAnchorCoordinate(QGeoCoordinate());
        clear();
        setStride(kFloatsPerVertex * sizeof(float));
        setVertexData(QByteArray());
        setPrimitiveType(QQuick3DGeometry::PrimitiveType::TriangleStrip);
        setBounds(QVector3D(), QVector3D());
        update();
        return;
    }

    QList<QGeoCoordinate> points;

    const qsizetype edgeCount = _closed ? _path.size() : (_path.size() - 1);
    for (qsizetype i = 0; i < edgeCount; ++i) {
        const QGeoCoordinate &from = _path[i];
        const QGeoCoordinate &to = _closed ? _path[(i + 1) % _path.size()] : _path[i + 1];

        const double distance = from.distanceTo(to);
        const int steps = qBound(1,
                                 static_cast<int>(std::ceil(distance / kMaxSegmentMeters)),
                                 kMaxSubdivisionsPerEdge);
        const double azimuth = from.azimuthTo(to);

        points.append(from);

        for (int step = 1; step < steps; ++step) {
            const double ratio = static_cast<double>(step) / static_cast<double>(steps);
            QGeoCoordinate point = from.atDistanceAndAzimuth(distance * ratio, azimuth);

            const double fromAltitude = from.altitude();
            const double toAltitude = to.altitude();
            if (finiteAltitude(fromAltitude) && finiteAltitude(toAltitude)) {
                point.setAltitude(fromAltitude + (toAltitude - fromAltitude) * ratio);
            } else if (finiteAltitude(fromAltitude)) {
                point.setAltitude(fromAltitude);
            } else if (finiteAltitude(toAltitude)) {
                point.setAltitude(toAltitude);
            }

            points.append(point);
        }
    }

    if (_closed) {
        if (!points.isEmpty()) {
            points.append(points.first());
        }
    } else if (!_path.isEmpty()) {
        points.append(_path.last());
    }

    if (points.size() < 2) {
        return;
    }

    const QGeoCoordinate &anchor = points.first();
    const double anchorTerrain = _terrainHeight(anchor);
    _setAnchorCoordinate(QGeoCoordinate(anchor.latitude(), anchor.longitude(), anchorTerrain));

    const QPointF anchorWorld = TileMath::geoToWorld(anchor);
    const double verticalScale = _scene->verticalScale();

    QByteArray vertexData;
    vertexData.resize(points.size() * 2 * kFloatsPerVertex * static_cast<qsizetype>(sizeof(float)));
    float *vertex = reinterpret_cast<float *>(vertexData.data());

    QVector3D minBounds;
    QVector3D maxBounds;
    bool haveBounds = false;

    for (const QGeoCoordinate &point : std::as_const(points)) {
        const QPointF world = TileMath::geoToWorld(point);
        const float x = static_cast<float>(world.x() - anchorWorld.x());
        const float y = static_cast<float>(world.y() - anchorWorld.y());

        const double terrain = _terrainHeight(point);
        const double topAltitude = _topAltitude(point, terrain);

        const float zBottom = static_cast<float>((terrain - anchorTerrain) * verticalScale);
        const float zTop = static_cast<float>((topAltitude - anchorTerrain) * verticalScale);

        *vertex++ = x;
        *vertex++ = y;
        *vertex++ = zBottom;

        *vertex++ = x;
        *vertex++ = y;
        *vertex++ = zTop;

        const QVector3D bottom(x, y, zBottom);
        const QVector3D top(x, y, zTop);

        if (!haveBounds) {
            minBounds = QVector3D(qMin(bottom.x(), top.x()), qMin(bottom.y(), top.y()), qMin(bottom.z(), top.z()));
            maxBounds = QVector3D(qMax(bottom.x(), top.x()), qMax(bottom.y(), top.y()), qMax(bottom.z(), top.z()));
            haveBounds = true;
        } else {
            minBounds = QVector3D(qMin(minBounds.x(), qMin(bottom.x(), top.x())),
                                  qMin(minBounds.y(), qMin(bottom.y(), top.y())),
                                  qMin(minBounds.z(), qMin(bottom.z(), top.z())));
            maxBounds = QVector3D(qMax(maxBounds.x(), qMax(bottom.x(), top.x())),
                                  qMax(maxBounds.y(), qMax(bottom.y(), top.y())),
                                  qMax(maxBounds.z(), qMax(bottom.z(), top.z())));
        }
    }

    clear();
    setStride(kFloatsPerVertex * sizeof(float));
    setVertexData(vertexData);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::TriangleStrip);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);
    setBounds(minBounds, maxBounds);
    update();
}

double AtlasKmlWallGeometry::_terrainHeight(const QGeoCoordinate &coordinate) const
{
    if (!_surfaceModel) {
        return 0.0;
    }

    const double value = _surfaceModel->terrainHeightAt(coordinate);
    return finiteAltitude(value) ? value : 0.0;
}

double AtlasKmlWallGeometry::_topAltitude(const QGeoCoordinate &coordinate, double terrainHeight) const
{
    const double altitude = coordinate.altitude();

    if (!finiteAltitude(altitude)) {
        return terrainHeight;
    }

    if (_altitudeMode == 2) {
        // KML absolute = AMSL/metres above sea level.
        return altitude;
    }

    if (_altitudeMode == 1) {
        // KML relativeToGround = metres above local terrain.
        return terrainHeight + altitude;
    }

    return terrainHeight;
}

void AtlasKmlWallGeometry::_setAnchorCoordinate(const QGeoCoordinate &coordinate)
{
    if (_anchorCoordinate == coordinate) {
        return;
    }

    _anchorCoordinate = coordinate;
    emit anchorCoordinateChanged();
}
