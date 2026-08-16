/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "FlightPathGeometry.h"

#include <QtCore/QByteArray>
#include <QtCore/QPointF>
#include <QtCore/qminmax.h>
#include <QtCore/qnumeric.h>
#include <QtGui/QVector3D>

#include <cmath>

#include "GeoScene.h"
#include "TileMath.h"

FlightPathGeometry::FlightPathGeometry(QQuick3DObject* parent) : QQuick3DGeometry(parent)
{
    _rebuild();
}

void FlightPathGeometry::setScene(GeoScene* scene)
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
        connect(_scene, &GeoScene::sceneOriginChanged, this, &FlightPathGeometry::_rebuild);
        connect(_scene, &QObject::destroyed, this, [this] {
            _scene = nullptr;
            emit sceneChanged();
            _rebuild();
        });
    }
    emit sceneChanged();
    _rebuild();
}

void FlightPathGeometry::setPath(const QVariantList& coordinates)
{
    _points.clear();
    _points.reserve(coordinates.size());
    for (const QVariant& variant : coordinates) {
        const QGeoCoordinate coordinate = variant.value<QGeoCoordinate>();
        if (coordinate.isValid()) {
            _points.append(coordinate);
        }
    }
    emit anchorCoordinateChanged();
    emit pointCountChanged();
    _rebuild();
}

void FlightPathGeometry::appendPoint(const QGeoCoordinate& coordinate)
{
    if (!coordinate.isValid()) {
        return;
    }
    _points.append(coordinate);
    if (_points.size() == 1) {
        emit anchorCoordinateChanged();
    }
    emit pointCountChanged();
    if (!_scene || (_points.size() < 3)) {
        _rebuild();
        return;
    }

    // Incremental: only the new vertex pair and the previous point's blended tangent change
    const qsizetype last = _points.size() - 1;
    _positions.append(_positionFor(coordinate));
    _segmentDirections.append(_segmentDirection(last - 1));
    _vertexData.resize(_vertexData.size() + (2 * kFloatsPerVertex * static_cast<qsizetype>(sizeof(float))));
    _writeVertexPair(last - 1, _tangentAt(last - 1));
    _writeVertexPair(last, _tangentAt(last));
    _growBounds(_positions.last());
    setVertexData(_vertexData);
    setBounds(_minBounds, _maxBounds);
    update();
}

void FlightPathGeometry::updateLastPoint(const QGeoCoordinate& coordinate)
{
    if (!coordinate.isValid() || _points.isEmpty()) {
        return;
    }
    _points.last() = coordinate;
    if (_points.size() == 1) {
        emit anchorCoordinateChanged();
    }
    if (!_scene || (_points.size() < 2)) {
        _rebuild();
        return;
    }

    // Incremental: rewrite the last two vertex pairs and upload just that byte range
    const qsizetype last = _points.size() - 1;
    _positions.last() = _positionFor(coordinate);
    _segmentDirections.last() = _segmentDirection(last - 1);
    _writeVertexPair(last - 1, _tangentAt(last - 1));
    _writeVertexPair(last, _tangentAt(last));
    _growBounds(_positions.last());  // grow-only: bounds stay conservative if the point retreats
    const qsizetype offset = (last - 1) * 2 * kFloatsPerVertex * static_cast<qsizetype>(sizeof(float));
    const qsizetype length = 4 * kFloatsPerVertex * static_cast<qsizetype>(sizeof(float));
    setVertexData(static_cast<int>(offset), QByteArray(_vertexData.constData() + offset, length));
    setBounds(_minBounds, _maxBounds);
    update();
}

void FlightPathGeometry::clearPath()
{
    if (_points.isEmpty()) {
        return;
    }
    _points.clear();
    emit anchorCoordinateChanged();
    emit pointCountChanged();
    _rebuild();
}

void FlightPathGeometry::_rebuild()
{
    _positions.clear();
    _segmentDirections.clear();
    _vertexData.clear();

    if (!_scene || (_points.size() < 2)) {
        clear();
        setStride(kFloatsPerVertex * sizeof(float));
        setVertexData(QByteArray());
        setPrimitiveType(QQuick3DGeometry::PrimitiveType::TriangleStrip);
        setBounds(QVector3D(), QVector3D());
        update();
        return;
    }

    _positions.reserve(_points.size());
    for (const QGeoCoordinate& point : _points) {
        _positions.append(_positionFor(point));
    }

    _segmentDirections.reserve(_positions.size() - 1);
    for (qsizetype i = 0; i < _positions.size() - 1; i++) {
        _segmentDirections.append(_segmentDirection(i));
    }

    _vertexData.resize(_positions.size() * 2 * kFloatsPerVertex * static_cast<qsizetype>(sizeof(float)));
    _minBounds = _positions.first();
    _maxBounds = _positions.first();
    for (qsizetype i = 0; i < _positions.size(); i++) {
        _writeVertexPair(i, _tangentAt(i));
        _growBounds(_positions[i]);
    }

    clear();
    setStride(kFloatsPerVertex * sizeof(float));
    setVertexData(_vertexData);
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::TriangleStrip);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic, 3 * sizeof(float), QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic, 6 * sizeof(float),
                 QQuick3DGeometry::Attribute::F32Type);
    setBounds(_minBounds, _maxBounds);
    update();
}

// Anchor-relative offset in double: exact regardless of scene origin
QVector3D FlightPathGeometry::_positionFor(const QGeoCoordinate& coordinate) const
{
    const QGeoCoordinate& anchor = _points.first();
    const QPointF anchorWorld = TileMath::geoToWorld(anchor);
    const double anchorAltitude = std::isfinite(anchor.altitude()) ? anchor.altitude() : 0.0;
    const QPointF world = TileMath::geoToWorld(coordinate);
    const double altitude = std::isfinite(coordinate.altitude()) ? coordinate.altitude() : 0.0;
    return QVector3D(static_cast<float>(world.x() - anchorWorld.x()), static_cast<float>(world.y() - anchorWorld.y()),
                     static_cast<float>((altitude - anchorAltitude) * _scene->verticalScale()));
}

// Unit 3D tangent of the segment starting at point index (vertical climbs keep
// a valid direction); degenerate segments (repeated points) reuse the previous direction
QVector3D FlightPathGeometry::_segmentDirection(qsizetype index) const
{
    const QVector3D direction = _positions[index + 1] - _positions[index];
    if (!qFuzzyIsNull(direction.lengthSquared())) {
        return direction.normalized();
    }
    return (index > 0) ? _segmentDirections[index - 1] : QVector3D(0.0f, 1.0f, 0.0f);
}

// Interior points blend adjacent segments for a smooth join
QVector3D FlightPathGeometry::_tangentAt(qsizetype index) const
{
    QVector3D direction;
    if (index == 0) {
        direction = _segmentDirections.first();
    } else if (index == _positions.size() - 1) {
        direction = _segmentDirections.last();
    } else {
        direction = _segmentDirections[index - 1] + _segmentDirections[index];
    }
    if (qFuzzyIsNull(direction.lengthSquared())) {
        direction = _segmentDirections[index - 1];  // 180-degree reversal: either side works
    }
    direction.normalize();
    return direction;
}

void FlightPathGeometry::_writeVertexPair(qsizetype index, const QVector3D& tangent)
{
    const QVector3D& position = _positions[index];
    float* vertex = reinterpret_cast<float*>(_vertexData.data()) + (index * 2 * kFloatsPerVertex);
    for (const float side : {-1.0f, 1.0f}) {
        *vertex++ = position.x();
        *vertex++ = position.y();
        *vertex++ = position.z();
        *vertex++ = tangent.x();
        *vertex++ = tangent.y();
        *vertex++ = tangent.z();
        *vertex++ = side;
        *vertex++ = 0.0f;
    }
}

void FlightPathGeometry::_growBounds(const QVector3D& position)
{
    _minBounds = QVector3D(qMin(_minBounds.x(), position.x()), qMin(_minBounds.y(), position.y()),
                           qMin(_minBounds.z(), position.z()));
    _maxBounds = QVector3D(qMax(_maxBounds.x(), position.x()), qMax(_maxBounds.y(), position.y()),
                           qMax(_maxBounds.z(), position.z()));
}
