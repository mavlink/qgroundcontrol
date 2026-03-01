#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick3D/QQuick3DGeometry>

Q_DECLARE_LOGGING_CATEGORY(TerrainMeshDataLog)

class TerrainAtCoordinateQuery;

/// Generates a 3D terrain mesh grid for overlaying on a tilted map.
/// Takes a center coordinate and visible radius, queries terrain elevation
/// data, and builds a triangle mesh with altitude-based vertex heights.
class TerrainMeshData : public QQuick3DGeometry
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QGeoCoordinate center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(double visibleMeters READ visibleMeters WRITE setVisibleMeters NOTIFY visibleMetersChanged)
    Q_PROPERTY(int gridSize READ gridSize WRITE setGridSize NOTIFY gridSizeChanged)
    Q_PROPERTY(double heightScale READ heightScale WRITE setHeightScale NOTIFY heightScaleChanged)
    Q_PROPERTY(double minHeight READ minHeight NOTIFY terrainDataChanged)
    Q_PROPERTY(double maxHeight READ maxHeight NOTIFY terrainDataChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)

public:
    explicit TerrainMeshData();
    ~TerrainMeshData() override;

    QGeoCoordinate center() const { return _center; }
    void setCenter(const QGeoCoordinate &center);

    double visibleMeters() const { return _visibleMeters; }
    void setVisibleMeters(double meters);

    int gridSize() const { return _gridSize; }
    void setGridSize(int size);

    double heightScale() const { return _heightScale; }
    void setHeightScale(double scale);

    double minHeight() const { return _minHeight; }
    double maxHeight() const { return _maxHeight; }
    bool ready() const { return _ready; }

signals:
    void centerChanged();
    void visibleMetersChanged();
    void gridSizeChanged();
    void heightScaleChanged();
    void terrainDataChanged();
    void readyChanged();

private:
    void _scheduleUpdate();
    void _requestTerrainData();
    void _onTerrainDataReceived(bool success, const QList<double> &heights);
    void _buildMesh();
    static QVector3D _computeNormal(const QVector3D &v1, const QVector3D &v2, const QVector3D &v3);

    QGeoCoordinate _center;
    double _visibleMeters = 5000.0;
    int _gridSize = 40;
    double _heightScale = 1.0;
    double _minHeight = 0.0;
    double _maxHeight = 0.0;
    bool _ready = false;

    QList<QGeoCoordinate> _gridCoordinates;
    QList<double> _heights;

    QTimer _updateTimer;
    TerrainAtCoordinateQuery *_currentQuery = nullptr;
};
