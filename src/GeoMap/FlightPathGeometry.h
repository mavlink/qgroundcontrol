/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick3D/QQuick3DGeometry>

class GeoScene;

Q_MOC_INCLUDE("GeoScene.h")

/// Flight-path trail ribbon for the GeoMap 3D scene: a triangle strip along
/// the vehicle's trajectory, depth-tested against terrain (hills hide it).
/// Each trail point yields two centerline vertices carrying the path's unit
/// 3D tangent (normal slot) and a side flag (texcoord0.x, -1 left / +1
/// right); the ribbon has zero width until a vertex shader expands each
/// vertex perpendicular to both the tangent and the view direction
/// (billboarding), keeping the width constant in screen pixels for any
/// segment orientation — including straight vertical climbs.
///
/// Positions are scene-unit offsets relative to anchorCoordinate (the first
/// path point), so the buffer is immune to scene-origin re-anchors and to any
/// constant altitude bias applied at the anchor. z offsets are pre-scaled by
/// the scene's verticalScale but NOT terrainScale: parent the model under a
/// node whose z-scale tracks GeoScene::terrainScale.
///
/// appendPoint/updateLastPoint update the vertex buffer incrementally (only
/// the last two vertex pairs change); setPath and scene re-anchors rebuild it.
class FlightPathGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(GeoScene* scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(QGeoCoordinate anchorCoordinate READ anchorCoordinate NOTIFY anchorCoordinateChanged)
    Q_PROPERTY(int pointCount READ pointCount NOTIFY pointCountChanged)

public:
    explicit FlightPathGeometry(QQuick3DObject* parent = nullptr);

    static constexpr int kFloatsPerVertex = 8;  ///< position 3, tangent 3, side flag 2

    GeoScene* scene() const { return _scene; }

    void setScene(GeoScene* scene);

    /// First path point (invalid while the path is empty). Place the ribbon's
    /// node at this coordinate; altitude offsets in the buffer are relative
    /// to its altitude.
    QGeoCoordinate anchorCoordinate() const { return _points.isEmpty() ? QGeoCoordinate() : _points.first(); }

    int pointCount() const { return static_cast<int>(_points.size()); }

    /// Replace the whole path (QGeoCoordinate variants, TrajectoryPoints::list() format)
    Q_INVOKABLE void setPath(const QVariantList& coordinates);

    Q_INVOKABLE void appendPoint(const QGeoCoordinate& coordinate);

    /// Replace the last point (TrajectoryPoints emits this for colinear motion)
    Q_INVOKABLE void updateLastPoint(const QGeoCoordinate& coordinate);

    Q_INVOKABLE void clearPath();

signals:
    void sceneChanged();
    void anchorCoordinateChanged();
    void pointCountChanged();

private:
    void _rebuild();
    QVector3D _positionFor(const QGeoCoordinate& coordinate) const;
    QVector3D _segmentDirection(qsizetype index) const;
    QVector3D _tangentAt(qsizetype index) const;
    void _writeVertexPair(qsizetype index, const QVector3D& tangent);
    void _growBounds(const QVector3D& position);

    GeoScene* _scene = nullptr;
    QList<QGeoCoordinate> _points;

    // Incremental mirrors of the GPU buffer so mutations avoid full rebuilds
    QList<QVector3D> _positions;
    QList<QVector3D> _segmentDirections;
    QByteArray _vertexData;
    QVector3D _minBounds;
    QVector3D _maxBounds;
};
