/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FenceWallGeometry.h"

#include <QtCore/QPointF>
#include <QtCore/qminmax.h>
#include <QtCore/qnumeric.h>

#include <cmath>

#include "GeoScene.h"
#include "SurfacePatchModel.h"
#include "TileMath.h"

namespace {
constexpr double kMaxSegmentMeters = 50.0;  ///< bottom-edge terrain-following resolution
constexpr int kMaxSubdivisionsPerEdge = 64;
constexpr int kFloatsPerVertex = 3;
}  // namespace

FenceWallGeometry::FenceWallGeometry(QQuick3DObject* parent) : QQuick3DGeometry(parent)
{
    _rebuild();
}

void FenceWallGeometry::setScene(GeoScene* scene)
{
    if (scene == _scene) {
        return;
    }
    if (_scene) {
        disconnect(_scene, nullptr, this, nullptr);
    }
    _scene = scene;
    if (_scene) {
        // verticalScale follows the origin latitude, so z offsets drift on re-anchor
        connect(_scene, &GeoScene::sceneOriginChanged, this, &FenceWallGeometry::_scheduleRebuild);
        connect(_scene, &QObject::destroyed, this, [this] {
            _scene = nullptr;
            emit sceneChanged();
            _scheduleRebuild();
        });
    }
    emit sceneChanged();
    _scheduleRebuild();
}

void FenceWallGeometry::setSurfaceModel(SurfacePatchModel* surfaceModel)
{
    if (surfaceModel == _surfaceModel) {
        return;
    }
    if (_surfaceModel) {
        disconnect(_surfaceModel, nullptr, this, nullptr);
    }
    _surfaceModel = surfaceModel;
    if (_surfaceModel) {
        connect(_surfaceModel, &SurfacePatchModel::terrainHeightsChanged, this, &FenceWallGeometry::_scheduleRebuild);
        connect(_surfaceModel, &QObject::destroyed, this, [this] {
            _surfaceModel = nullptr;
            emit surfaceModelChanged();
            _scheduleRebuild();
        });
    }
    emit surfaceModelChanged();
    _scheduleRebuild();
}

void FenceWallGeometry::setCoordinates(const QVariantList& coordinates)
{
    if (coordinates == _coordinates) {
        return;
    }
    _coordinates = coordinates;
    _ring.clear();
    _ring.reserve(coordinates.size());
    for (const QVariant& variant : coordinates) {
        const QGeoCoordinate coordinate = variant.value<QGeoCoordinate>();
        if (coordinate.isValid()) {
            _ring.append(coordinate);
        }
    }
    // Tolerate an already-closed ring (circleCoordinates closes back to the start)
    if ((_ring.size() > 1) && (_ring.first().distanceTo(_ring.last()) < 0.01)) {
        _ring.removeLast();
    }
    emit coordinatesChanged();
    _scheduleRebuild();
}

void FenceWallGeometry::setHeightAglMeters(qreal heightAglMeters)
{
    if (qFuzzyCompare(heightAglMeters, _heightAglMeters)) {
        return;
    }
    _heightAglMeters = heightAglMeters;
    emit heightAglMetersChanged();
    _scheduleRebuild();
}

void FenceWallGeometry::_scheduleRebuild()
{
    if (_rebuildPending) {
        return;
    }
    _rebuildPending = true;
    QMetaObject::invokeMethod(this, &FenceWallGeometry::_rebuild, Qt::QueuedConnection);
}

void FenceWallGeometry::_rebuild()
{
    _rebuildPending = false;
    if (!_scene || (_ring.size() < 3) || (_heightAglMeters <= 0.0)) {
        _setAnchorCoordinate(QGeoCoordinate());
        clear();
        setStride(kFloatsPerVertex * sizeof(float));
        setVertexData(QByteArray());
        setPrimitiveType(QQuick3DGeometry::PrimitiveType::TriangleStrip);
        setBounds(QVector3D(), QVector3D());
        update();
        return;
    }

    // Subdivide each edge so the bottom edge drapes over terrain
    QList<QGeoCoordinate> points;
    for (qsizetype i = 0; i < _ring.size(); i++) {
        const QGeoCoordinate& from = _ring[i];
        const QGeoCoordinate& to = _ring[(i + 1) % _ring.size()];
        const double distance = from.distanceTo(to);
        const int steps = qBound(1, static_cast<int>(std::ceil(distance / kMaxSegmentMeters)), kMaxSubdivisionsPerEdge);
        const double azimuth = from.azimuthTo(to);
        points.append(from);
        for (int step = 1; step < steps; step++) {
            points.append(from.atDistanceAndAzimuth(distance * step / steps, azimuth));
        }
    }
    points.append(points.first());  // close the ring

    const QGeoCoordinate& anchor = points.first();
    const double anchorTerrain = _terrainHeight(anchor);
    _setAnchorCoordinate(QGeoCoordinate(anchor.latitude(), anchor.longitude(), anchorTerrain));

    const QPointF anchorWorld = TileMath::geoToWorld(anchor);
    const double verticalScale = _scene->verticalScale();
    const float wallHeight = static_cast<float>(_heightAglMeters * verticalScale);

    QByteArray vertexData;
    vertexData.resize(points.size() * 2 * kFloatsPerVertex * static_cast<qsizetype>(sizeof(float)));
    float* vertex = reinterpret_cast<float*>(vertexData.data());
    QVector3D minBounds;
    QVector3D maxBounds;
    for (qsizetype i = 0; i < points.size(); i++) {
        const QPointF world = TileMath::geoToWorld(points[i]);
        const float x = static_cast<float>(world.x() - anchorWorld.x());
        const float y = static_cast<float>(world.y() - anchorWorld.y());
        const float zBottom = static_cast<float>((_terrainHeight(points[i]) - anchorTerrain) * verticalScale);
        *vertex++ = x;
        *vertex++ = y;
        *vertex++ = zBottom;
        *vertex++ = x;
        *vertex++ = y;
        *vertex++ = zBottom + wallHeight;
        const QVector3D bottom(x, y, zBottom);
        const QVector3D top(x, y, zBottom + wallHeight);
        if (i == 0) {
            minBounds = bottom;
            maxBounds = top;
        } else {
            minBounds = QVector3D(qMin(minBounds.x(), x), qMin(minBounds.y(), y), qMin(minBounds.z(), zBottom));
            maxBounds = QVector3D(qMax(maxBounds.x(), x), qMax(maxBounds.y(), y), qMax(maxBounds.z(), top.z()));
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

double FenceWallGeometry::_terrainHeight(const QGeoCoordinate& coordinate) const
{
    if (!_surfaceModel) {
        return 0.0;
    }
    const double height = _surfaceModel->terrainHeightAt(coordinate);
    return std::isfinite(height) ? height : 0.0;
}

void FenceWallGeometry::_setAnchorCoordinate(const QGeoCoordinate& anchorCoordinate)
{
    if (anchorCoordinate == _anchorCoordinate) {
        return;
    }
    _anchorCoordinate = anchorCoordinate;
    emit anchorCoordinateChanged();
}
