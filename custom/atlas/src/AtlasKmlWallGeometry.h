#pragma once

#include <QtCore/QList>
#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick3D/QQuick3DGeometry>

class GeoScene;
class SurfacePatchModel;

// Qt 6.11 requires pointer types exposed through Q_PROPERTY to be complete
// when moc generates their meta-type information. Keep the header light and
// teach moc which concrete headers to include, matching QGroundControl's
// native GeoMapItem/FenceWallGeometry pattern.
Q_MOC_INCLUDE("GeoScene.h")
Q_MOC_INCLUDE("SurfacePatchModel.h")

class AtlasKmlWallGeometry : public QQuick3DGeometry
{
    Q_OBJECT

    Q_PROPERTY(GeoScene* scene READ scene WRITE setScene NOTIFY sceneChanged)
    Q_PROPERTY(SurfacePatchModel* surfaceModel READ surfaceModel WRITE setSurfaceModel NOTIFY surfaceModelChanged)
    Q_PROPERTY(QVariantList coordinates READ coordinates WRITE setCoordinates NOTIFY coordinatesChanged)
    Q_PROPERTY(int altitudeMode READ altitudeMode WRITE setAltitudeMode NOTIFY altitudeModeChanged)
    Q_PROPERTY(bool closed READ closed WRITE setClosed NOTIFY closedChanged)
    Q_PROPERTY(QGeoCoordinate anchorCoordinate READ anchorCoordinate NOTIFY anchorCoordinateChanged)

public:
    explicit AtlasKmlWallGeometry(QQuick3DObject *parent = nullptr);

    GeoScene *scene() const { return _scene; }
    void setScene(GeoScene *scene);

    SurfacePatchModel *surfaceModel() const { return _surfaceModel; }
    void setSurfaceModel(SurfacePatchModel *surfaceModel);

    QVariantList coordinates() const { return _coordinates; }
    void setCoordinates(const QVariantList &coordinates);

    // 0 = clampToGround, 1 = relativeToGround, 2 = absolute.
    int altitudeMode() const { return _altitudeMode; }
    void setAltitudeMode(int mode);

    bool closed() const { return _closed; }
    void setClosed(bool closed);

    QGeoCoordinate anchorCoordinate() const { return _anchorCoordinate; }

signals:
    void sceneChanged();
    void surfaceModelChanged();
    void coordinatesChanged();
    void altitudeModeChanged();
    void closedChanged();
    void anchorCoordinateChanged();

private slots:
    void _rebuild();

private:
    void _scheduleRebuild();
    double _terrainHeight(const QGeoCoordinate &coordinate) const;
    double _topAltitude(const QGeoCoordinate &coordinate, double terrainHeight) const;
    void _setAnchorCoordinate(const QGeoCoordinate &coordinate);

    GeoScene *_scene = nullptr;
    SurfacePatchModel *_surfaceModel = nullptr;
    QVariantList _coordinates;
    QList<QGeoCoordinate> _path;
    int _altitudeMode = 0;
    bool _closed = true;
    QGeoCoordinate _anchorCoordinate;
    bool _rebuildPending = false;
};
