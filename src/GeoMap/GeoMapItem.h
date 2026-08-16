/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QPointF>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick/QQuickItem>

class GeoMapCamera;
class GeoScene;
class QQmlComponent;
class SurfacePatchModel;

Q_MOC_INCLUDE("GeoScene.h")
Q_MOC_INCLUDE("SurfacePatchModel.h")
Q_MOC_INCLUDE("QtQml/QQmlComponent")

/// GeoMap counterpart of QtLocation's MapQuickItem: anchors its QML content
/// at a geographic coordinate, projected through the scene's camera into the
/// 2D overlay above the View3D. Meant to be used as the base of marker
/// controls (e.g. GeoMapPin) that declare their visuals as ordinary children.
/// The item tracks camera pose, scene re-anchoring, terrain-scale transitions,
/// and (in ClampToGround mode) terrain height refinement. While the coordinate
/// is behind the camera or beyond the horizon the item parks off-screen and
/// projected is false.
///
/// Consumers parent the item into the GeoMap control and wire scene (required)
/// and surfaceModel (required for ClampToGround) from the map's properties.
class GeoMapItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QGeoCoordinate coordinate READ coordinate WRITE setCoordinate NOTIFY coordinateChanged)
    Q_PROPERTY(QPointF anchorPoint READ anchorPoint WRITE setAnchorPoint NOTIFY anchorPointChanged)
    Q_PROPERTY(GeoScene* scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(SurfacePatchModel* surfaceModel READ surfaceModel WRITE setSurfaceModel NOTIFY surfaceModelChanged)
    Q_PROPERTY(AltitudeMode altitudeMode READ altitudeMode WRITE setAltitudeMode NOTIFY altitudeModeChanged)
    Q_PROPERTY(bool projected READ projected NOTIFY projectedChanged)
    Q_PROPERTY(QVector3D scenePosition READ scenePosition NOTIFY scenePositionChanged)
    Q_PROPERTY(QQmlComponent* delegate3D READ delegate3D WRITE setDelegate3D NOTIFY delegate3DChanged)
    Q_PROPERTY(QObject* node3D READ node3D NOTIFY node3DChanged)
    Q_PROPERTY(bool crossfade3D READ crossfade3D WRITE setCrossfade3D NOTIFY crossfade3DChanged)
    Q_PROPERTY(qreal contentOpacity2D READ contentOpacity2D NOTIFY contentOpacity2DChanged)

public:
    explicit GeoMapItem(QQuickItem* parent = nullptr);
    ~GeoMapItem() override;

    /// How the coordinate's vertical position is resolved
    enum class AltitudeMode
    {
        ClampToGround,  ///< sit on the rendered terrain surface (default)
        Absolute        ///< use coordinate.altitude (NaN treated as 0)
    };
    Q_ENUM(AltitudeMode)

    QGeoCoordinate coordinate() const { return _coordinate; }

    void setCoordinate(const QGeoCoordinate& coordinate);

    /// Pixel offset from this item's top-left placed at the projected
    /// coordinate (e.g. the tip of a pin, or the center for a centered marker)
    QPointF anchorPoint() const { return _anchorPoint; }

    void setAnchorPoint(const QPointF& anchorPoint);

    GeoScene* scene() const { return _scene; }

    void setScene(GeoScene* scene);

    SurfacePatchModel* surfaceModel() const { return _surfaceModel; }

    void setSurfaceModel(SurfacePatchModel* surfaceModel);

    AltitudeMode altitudeMode() const { return _altitudeMode; }

    void setAltitudeMode(AltitudeMode mode);

    /// False while the coordinate cannot be projected on screen (invalid, no
    /// camera, behind the camera or beyond the horizon)
    bool projected() const { return _projected; }

    /// Origin-relative scene position of the coordinate (z pre-scaled by
    /// verticalScale and terrainScale), matching the rendered surface. Valid
    /// even while not projected, so 3D content can bind to it directly.
    QVector3D scenePosition() const { return _scenePosition; }

    /// Optional 3D counterpart of this item's 2D visuals: a QQuick3DNode
    /// component instantiated into the scene's content3D container. The item
    /// keeps the node's position on scenePosition, crossfades node opacity
    /// (terrainScale) against contentOpacity2D (1 - terrainScale), and syncs
    /// node visibility to this item's visible and coordinate validity.
    QQmlComponent* delegate3D() const { return _delegate3D; }

    void setDelegate3D(QQmlComponent* delegate3D);

    /// The instantiated delegate3D node (null while there is no delegate,
    /// scene or content3D container)
    QObject* node3D() const { return _node3D; }

    /// Whether the delegate3D node crossfades with 2D content during the
    /// 2D<->3D transition (default). When false the node stays fully opaque
    /// in both modes (for items with no 2D counterpart, e.g. the flight path).
    bool crossfade3D() const { return _crossfade3D; }

    void setCrossfade3D(bool crossfade3D);

    /// Opacity for this item's 2D visuals: fades out during the 2D->3D
    /// transition when a delegate3D is set, 1.0 otherwise. Bind children's
    /// opacity to it (multiplying any dimming of their own).
    qreal contentOpacity2D() const { return _contentOpacity2D; }

signals:
    void coordinateChanged();
    void anchorPointChanged();
    void sceneChanged();
    void surfaceModelChanged();
    void altitudeModeChanged();
    void projectedChanged();
    void scenePositionChanged();
    void delegate3DChanged();
    void node3DChanged();
    void crossfade3DChanged();
    void contentOpacity2DChanged();

private slots:
    void _updatePosition();
    void _terrainHeightsChanged();

private:
    void _connectCamera();
    void _setProjected(bool projected);
    void _setScenePosition(const QVector3D& scenePosition);
    void _rebuildNode3D();
    void _updateCrossfade();
    void _updateNode3DVisibility();

    QGeoCoordinate _coordinate;
    QPointF _anchorPoint;
    GeoScene* _scene = nullptr;
    SurfacePatchModel* _surfaceModel = nullptr;
    GeoMapCamera* _connectedCamera = nullptr;
    AltitudeMode _altitudeMode = AltitudeMode::ClampToGround;
    bool _projected = false;
    QVector3D _scenePosition;
    QQmlComponent* _delegate3D = nullptr;
    QObject* _node3D = nullptr;
    bool _crossfade3D = true;
    qreal _contentOpacity2D = 1.0;
};
