#pragma once

#include <QtCore/QLoggingCategory>
#include <QtGui/QVector2D>
#include <QtGui/QVector3D>
#include <QtPositioning/QGeoCoordinate>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick3D/QQuick3DGeometry>

Q_DECLARE_LOGGING_CATEGORY(Viewer3DTerrainGeometryLog)

class Viewer3DTerrainGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int            sectorCount   READ sectorCount   WRITE setSectorCount   NOTIFY sectorCountChanged)
    Q_PROPERTY(int            stackCount    READ stackCount    WRITE setStackCount    NOTIFY stackCountChanged)
    Q_PROPERTY(QGeoCoordinate roiMin        READ roiMin        WRITE setRoiMin        NOTIFY roiMinChanged)
    Q_PROPERTY(QGeoCoordinate roiMax        READ roiMax        WRITE setRoiMax        NOTIFY roiMaxChanged)
    Q_PROPERTY(QGeoCoordinate refCoordinate READ refCoordinate WRITE setRefCoordinate NOTIFY refCoordinateChanged)

    friend class Viewer3DTerrainGeometryTest;

public:
    explicit Viewer3DTerrainGeometry();

    Q_INVOKABLE void updateEarthData();

    int sectorCount() const { return _sectorCount; }
    void setSectorCount(int newSectorCount);

    int stackCount() const { return _stackCount; }
    void setStackCount(int newStackCount);

    QGeoCoordinate roiMin() const { return _roiMin; }
    void setRoiMin(const QGeoCoordinate &newRoiMin);

    QGeoCoordinate roiMax() const { return _roiMax; }
    void setRoiMax(const QGeoCoordinate &newRoiMax);

    QGeoCoordinate refCoordinate() const { return _refCoordinate; }
    void setRefCoordinate(const QGeoCoordinate &newRefCoordinate);

signals:
    void sectorCountChanged();
    void stackCountChanged();
    void roiMinChanged();
    void roiMaxChanged();
    void refCoordinateChanged();

private:
    bool _buildTerrain(const QGeoCoordinate &roiMinCoordinate, const QGeoCoordinate &roiMaxCoordinate, const QGeoCoordinate &refCoordinate, bool scale);
    static QVector3D _computeFaceNormal(const QVector3D &x1, const QVector3D &x2, const QVector3D &x3);
    void _clearScene();

    std::vector<QVector3D> _vertices;
    std::vector<QVector2D> _texCoords;
    std::vector<QVector3D> _normals;

    QGeoCoordinate _roiMin;
    QGeoCoordinate _roiMax;
    QGeoCoordinate _refCoordinate;

    int _sectorCount = 0;
    int _stackCount = 0;
};
