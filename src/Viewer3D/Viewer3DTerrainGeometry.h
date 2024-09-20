/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtQuick3D/QQuick3DGeometry>
#include <QtPositioning/QGeoCoordinate>
#include <QtGui/QVector3D>
#include <QtGui/QVector2D>

class Viewer3DSettings;

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DTerrainGeometry : public QQuick3DGeometry
{
    Q_OBJECT

    Q_PROPERTY(int sectorCount READ sectorCount WRITE setSectorCount NOTIFY sectorCountChanged)
    Q_PROPERTY(int stackCount READ stackCount WRITE setStackCount NOTIFY stackCountChanged)
    Q_PROPERTY(int radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(QGeoCoordinate roiMin READ roiMin WRITE setRoiMin NOTIFY roiMinChanged)
    Q_PROPERTY(QGeoCoordinate roiMax READ roiMax WRITE setRoiMax NOTIFY roiMaxChanged)
    Q_PROPERTY(QGeoCoordinate refCoordinate READ refCoordinate WRITE setRefCoordinate NOTIFY refCoordinateChanged)

public:
    explicit Viewer3DTerrainGeometry();

    Q_INVOKABLE void updateEarthData();


    int sectorCount() const;
    void setSectorCount(int newSectorCount);

    int stackCount() const;
    void setStackCount(int newStackCount);

    int radius() const;
    void setRadius(int newRadius);

    QGeoCoordinate roiMin() const;
    void setRoiMin(const QGeoCoordinate &newRoiMin);

    QGeoCoordinate roiMax() const;
    void setRoiMax(const QGeoCoordinate &newRoiMax);

    QGeoCoordinate refCoordinate() const;
    void setRefCoordinate(const QGeoCoordinate &newRefCoordinate);

private:

    int _sectorCount;
    int _stackCount;
    std::vector<QVector3D> _vertices;
    std::vector<QVector2D> _texCoords;
    std::vector<QVector3D> _normals;

    void buildTerrain(QGeoCoordinate roiMinCoordinate, QGeoCoordinate roiMaxCoordinate, QGeoCoordinate refCoordinate, bool scale);
    bool buildTerrain_2(QGeoCoordinate roiMinCoordinate, QGeoCoordinate roiMaxCoordinate, QGeoCoordinate refCoordinate, bool scale);

    QVector3D computeFaceNormal(QVector3D x1, QVector3D x2, QVector3D x3);
    void changeUpAxis(int from, int to);
    void clearScene();

    int _radius;
    QGeoCoordinate _roiMin;
    QGeoCoordinate _roiMax;
    QGeoCoordinate _refCoordinate;
    Viewer3DSettings* _viewer3DSettings = nullptr;


signals:

    void sectorCountChanged();
    void stackCountChanged();
    void radiusChanged();
    void roiMinChanged();
    void roiMaxChanged();
    void refCoordinateChanged();
};
