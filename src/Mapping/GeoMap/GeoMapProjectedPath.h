/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>

#include "GeoMapItem.h"

class GeoScene;
class GeoMapCamera;
class SurfacePatchModel;

Q_MOC_INCLUDE("GeoScene.h")
Q_MOC_INCLUDE("SurfacePatchModel.h")

/// Batch coordinate->screen projector for the GeoMap 2D overlay. Mirrors
/// GeoMapItem's own single-point camera-reactive projection
/// (GeoMapItem::_updatePosition), batched over N points: projects
/// coordinates through the camera whenever the camera pose, scene anchor, or
/// (in ClampToGround mode) terrain heights change, and republishes the
/// result as screenPoints. Meant for QtQuick.Shapes-based polygon/polyline
/// overlays (survey area, transect lines, landing approach lines, ...) that
/// need many vertices reprojected reactively without each caller hand-wiring
/// its own camera Connections.
class GeoMapProjectedPath : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList coordinates READ coordinates WRITE setCoordinates NOTIFY coordinatesChanged)
    Q_PROPERTY(GeoScene* scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(SurfacePatchModel* surfaceModel READ surfaceModel WRITE setSurfaceModel NOTIFY surfaceModelChanged)
    Q_PROPERTY(GeoMapItem::AltitudeMode altitudeMode READ altitudeMode WRITE setAltitudeMode NOTIFY altitudeModeChanged)
    Q_PROPERTY(QVariantList screenPoints READ screenPoints NOTIFY screenPointsChanged)
    Q_PROPERTY(bool projected READ projected NOTIFY projectedChanged)

public:
    explicit GeoMapProjectedPath(QObject* parent = nullptr);

    QVariantList coordinates() const { return _coordinates; }

    void setCoordinates(const QVariantList& coordinates);

    GeoScene* scene() const { return _scene; }

    void setScene(GeoScene* scene);

    SurfacePatchModel* surfaceModel() const { return _surfaceModel; }

    void setSurfaceModel(SurfacePatchModel* surfaceModel);

    GeoMapItem::AltitudeMode altitudeMode() const { return _altitudeMode; }

    void setAltitudeMode(GeoMapItem::AltitudeMode mode);

    QVariantList screenPoints() const { return _screenPoints; }

    /// False while any coordinate fails to project (invalid, no scene/camera,
    /// behind the camera plane, or beyond the horizon)
    bool projected() const { return _projected; }

    /// Points evenly spaced around a circle (great-circle distance/azimuth
    /// from center, closed back to the first point), for consumers that want
    /// a loiter-ring outline as an ordinary coordinates list
    Q_INVOKABLE static QVariantList circleCoordinates(const QGeoCoordinate& center, double radiusMeters,
                                                      int segments = 32);

signals:
    void coordinatesChanged();
    void sceneChanged();
    void surfaceModelChanged();
    void altitudeModeChanged();
    void screenPointsChanged();
    void projectedChanged();

private slots:
    void _rebuild();
    void _terrainHeightsChanged();

private:
    void _connectCamera();

    QVariantList _coordinates;
    GeoScene* _scene = nullptr;
    SurfacePatchModel* _surfaceModel = nullptr;
    GeoMapCamera* _connectedCamera = nullptr;
    GeoMapItem::AltitudeMode _altitudeMode = GeoMapItem::AltitudeMode::ClampToGround;
    QVariantList _screenPoints;
    bool _projected = false;
};
