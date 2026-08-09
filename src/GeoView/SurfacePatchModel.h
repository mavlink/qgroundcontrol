/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtQmlIntegration/QtQmlIntegration>

#include "TileMath.h"

class GeoViewCamera;
class HeightSource;
class SurfaceModel;

Q_MOC_INCLUDE("GeoViewCamera.h")

/// QML bridge between SurfaceModel and the Repeater3D that renders the surface:
/// one row per active patch, updated incrementally (no model resets on pan/zoom).
///
/// Patch positions are exposed in scene coordinates: world mercator meters
/// relative to sceneOrigin. The origin re-anchors to the camera center whenever
/// the camera strays more than kReanchorDistance from it, keeping scene
/// coordinates small enough for the renderer's float precision.
class SurfacePatchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(GeoViewCamera* camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(bool debugHills READ debugHills WRITE setDebugHills NOTIFY debugHillsChanged)
    Q_PROPERTY(QPointF sceneOrigin READ sceneOrigin NOTIFY sceneOriginChanged)
    Q_PROPERTY(qreal verticalScale READ verticalScale NOTIFY sceneOriginChanged)
    Q_PROPERTY(int gridSize READ gridSize CONSTANT)
    Q_PROPERTY(double maxRangeMultiplier READ maxRangeMultiplier CONSTANT)
    Q_PROPERTY(int patchCount READ patchCount NOTIFY statsChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY statsChanged)
    Q_PROPERTY(int maxZoomLevel READ maxZoomLevel NOTIFY statsChanged)

public:
    explicit SurfacePatchModel(QObject* parent = nullptr);
    ~SurfacePatchModel() override;

    static constexpr double kReanchorDistance = 50000.0;  ///< scene-origin re-anchor threshold (m)

    enum Roles
    {
        CenterXRole = Qt::UserRole + 1,  ///< patch center x in scene meters
        CenterYRole,                     ///< patch center y in scene meters
        SpanRole,                        ///< patch edge length (m)
        ZoomRole,                        ///< quadtree zoom level
        HeightsRole,                     ///< vertex height grid (empty = flat)
        ReadyRole,
    };

    GeoViewCamera* camera() const { return _camera; }

    void setCamera(GeoViewCamera* camera);

    bool debugHills() const { return _debugHills; }

    void setDebugHills(bool debugHills);

    QPointF sceneOrigin() const { return _sceneOrigin; }

    /// Mercator scale factor at the scene origin: heights are delivered in true
    /// meters, but scene x/y are mercator meters (stretched by 1/cos(lat)).
    /// Rendering must scale z by this factor or terrain and altitudes appear
    /// vertically squashed away from the equator.
    qreal verticalScale() const;

    int gridSize() const;

    /// SurfaceModel::kMaxRangeMultiplier, exposed so the scene camera's far
    /// clip plane can cover the full retained patch range
    double maxRangeMultiplier() const;

    int patchCount() const { return _keys.count(); }

    int pendingCount() const;
    int maxZoomLevel() const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void cameraChanged();
    void debugHillsChanged();
    void sceneOriginChanged();
    void statsChanged();

private slots:
    void _patchAdded(const TileMath::TileKey& key);
    void _patchReady(const TileMath::TileKey& key);
    void _patchRemoved(const TileMath::TileKey& key);
    void _maybeReanchor();

private:
    void _rebuildSurfaceModel();

    GeoViewCamera* _camera = nullptr;
    HeightSource* _heightSource = nullptr;
    SurfaceModel* _surfaceModel = nullptr;
    bool _debugHills = false;
    QPointF _sceneOrigin;
    bool _originSet = false;
    QList<TileMath::TileKey> _keys;  ///< row order; data fetched from SurfaceModel by key
};
