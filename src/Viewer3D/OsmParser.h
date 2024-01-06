#ifndef OSMPARSER_H
#define OSMPARSER_H

#include "qqml.h"
#include <QObject>
#include <QtXml>
#include <QFile>
#include <QMap>
#include <QVector3D>
#include <QVector2D>
#include "qgeocoordinate.h"

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class OsmParser : public QObject
{
    struct BuildingType
    {
        std::vector<QGeoCoordinate> points_gps;
        std::vector<QVector2D> points_local;
        std::vector<QVector3D> triangulated_mesh;
        QVector2D bb_max, bb_min; //bounding boxes
        float height;
        int levels;
    };

    Q_OBJECT

    Q_PROPERTY(float buildingLevelHeight READ buildingLevelHeight WRITE setBuildingLevelHeight NOTIFY buildingLevelHeightChanged)

public:
    explicit OsmParser(QObject *parent = nullptr);

    void setGpsRef(QGeoCoordinate gpsRef);
    QGeoCoordinate getGpsRef(){ return _gpsRefPoint;}
    void setBuildingLevelHeight(float levelHeight){_buildingLevelHeight = levelHeight; emit buildingLevelHeightChanged();}
    float buildingLevelHeight(void){return _buildingLevelHeight;}
    void parseOsmFile(QString filePath);
    void decodeNodeTags(QDomElement& xmlComponent, QMap<uint64_t, QGeoCoordinate> &nodeMap);
    void decodeBuildings(QDomElement& xmlComponent, QMap<uint64_t, BuildingType > &buildingMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate gpsRef);

    QByteArray buildingToMesh();

    void trianglateWallsExtrudedPolygon(std::vector<QVector3D>& triangulatedMesh, std::vector<QVector2D> verticesCcw, float h, bool inverseOrder=0, bool duplicateStartEndPoint=0);
    void trianglateRectangle(std::vector<QVector3D>& triangulatedMesh, std::vector<QVector3D> verticesCcw, bool invertNormal);

private:
    QThread* _mainThread;
    QGeoCoordinate _gpsRefPoint;
    QMap<uint64_t, QGeoCoordinate> _mapNodes;
    QMap<uint64_t, BuildingType> _mapBuildings;

    bool _gpsRefSet;
    float _buildingLevelHeight;
    bool _mapLoadedFlag;


signals:
    void gpsRefChanged(QGeoCoordinate newGpsRef);
    void newMapLoaded();
    void buildingLevelHeightChanged(void);

};

#endif // OSMPARSER_H
