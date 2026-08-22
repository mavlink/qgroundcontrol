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
#include <QtCore/QVariantList>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick3D/QQuick3DGeometry>

class GeoScene;
class SurfacePatchModel;

Q_MOC_INCLUDE("GeoScene.h")
Q_MOC_INCLUDE("SurfacePatchModel.h")

/// Vertical fence curtain for the GeoMap 3D scene: a triangle strip extruded
/// upward from the terrain to heightAglMeters along a closed coordinate ring
/// (geofence walls). Edges are subdivided so the bottom edge drapes over
/// terrain and the top edge stays a constant AGL height above it, matching
/// how firmwares evaluate AGL fences.
///
/// Positions are scene-unit offsets relative to anchorCoordinate (the first
/// ring vertex, at its terrain altitude). z offsets are pre-scaled by the
/// scene's verticalScale but NOT terrainScale: parent the model under a node
/// whose z-scale tracks GeoScene::terrainScale so the wall flattens away
/// during the 3D->2D transition.
class FenceWallGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(GeoScene* scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(SurfacePatchModel* surfaceModel READ surfaceModel WRITE setSurfaceModel NOTIFY surfaceModelChanged)
    Q_PROPERTY(QVariantList coordinates READ coordinates WRITE setCoordinates NOTIFY coordinatesChanged)
    Q_PROPERTY(qreal heightAglMeters READ heightAglMeters WRITE setHeightAglMeters NOTIFY heightAglMetersChanged)
    Q_PROPERTY(QGeoCoordinate anchorCoordinate READ anchorCoordinate NOTIFY anchorCoordinateChanged)

public:
    explicit FenceWallGeometry(QQuick3DObject* parent = nullptr);

    GeoScene* scene() const { return _scene; }

    void setScene(GeoScene* scene);

    SurfacePatchModel* surfaceModel() const { return _surfaceModel; }

    void setSurfaceModel(SurfacePatchModel* surfaceModel);

    /// Ring vertices (QGeoCoordinate variants). A trailing duplicate of the
    /// first vertex is tolerated; the ring is closed internally either way.
    QVariantList coordinates() const { return _coordinates; }

    void setCoordinates(const QVariantList& coordinates);

    qreal heightAglMeters() const { return _heightAglMeters; }

    void setHeightAglMeters(qreal heightAglMeters);

    /// First ring vertex at its terrain altitude (invalid while the ring is
    /// empty). Place the wall's node at this coordinate; z offsets in the
    /// buffer are relative to its altitude.
    QGeoCoordinate anchorCoordinate() const { return _anchorCoordinate; }

signals:
    void sceneChanged();
    void surfaceModelChanged();
    void coordinatesChanged();
    void heightAglMetersChanged();
    void anchorCoordinateChanged();

private slots:
    void _rebuild();

private:
    /// Coalesces rebuild triggers (terrain tiles arrive in bursts) into one
    /// rebuild per event-loop turn
    void _scheduleRebuild();
    double _terrainHeight(const QGeoCoordinate& coordinate) const;
    void _setAnchorCoordinate(const QGeoCoordinate& anchorCoordinate);

    GeoScene* _scene = nullptr;
    SurfacePatchModel* _surfaceModel = nullptr;
    QVariantList _coordinates;
    QList<QGeoCoordinate> _ring;
    qreal _heightAglMeters = 0.0;
    QGeoCoordinate _anchorCoordinate;
    bool _rebuildPending = false;
};
