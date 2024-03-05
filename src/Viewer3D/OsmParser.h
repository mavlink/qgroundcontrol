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
#include <QVariant>

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DSettings;

class OsmParser : public QObject
{
    struct BuildingType
    {
        std::vector<QGeoCoordinate> points_gps;
        std::vector<QGeoCoordinate> points_gps_inner;
        std::vector<QVector2D> points_local;
        std::vector<QVector2D> points_local_inner;
        std::vector<QVector3D> triangulated_mesh;
        QVector2D bb_max = QVector2D(-1e6, -1e6); //bounding boxes
        QVector2D bb_min = QVector2D(1e6, 1e6); //bounding boxes
        float height;
        float levels;

        void append(std::vector<QGeoCoordinate> newPoints, bool isInner){
            for(uint i=0; i<newPoints.size(); i++){
                if(isInner){
                points_gps_inner.push_back(newPoints[i]);
                }else{
                    points_gps.push_back(newPoints[i]);
                }
            }
        }

        void append(std::vector<QVector2D> newPoints, bool isInner){
            for(uint i=0; i<newPoints.size(); i++){
                if(isInner){
                    points_local_inner.push_back(newPoints[i]);
                }else{
                    points_local.push_back(newPoints[i]);
                }
            }
        }
    };

    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    // Q_PROPERTY(float buildingLevelHeight READ buildingLevelHeight WRITE setBuildingLevelHeight NOTIFY buildingLevelHeightChanged)

public:
    explicit OsmParser(QObject *parent = nullptr);

    void setGpsRef(QGeoCoordinate gpsRef);
    void resetGpsRef();
    QGeoCoordinate getGpsRef(){ return _gpsRefPoint;}

    float buildingLevelHeight(void){return _buildingLevelHeight;}
    void parseOsmFile(QString filePath);
    void decodeNodeTags(QDomElement& xmlComponent, QMap<uint64_t, QGeoCoordinate> &nodeMap);
    void decodeBuildings(QDomElement& xmlComponent, QMap<uint64_t, BuildingType > &buildingMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate gpsRef);
    void decodeRelations(QDomElement& xmlComponent, QMap<uint64_t, BuildingType > &buildingMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate gpsRef);

    QByteArray buildingToMesh();

    void trianglateWallsExtrudedPolygon(std::vector<QVector3D>& triangulatedMesh, std::vector<QVector2D> verticesCcw, float h, bool inverseOrder=0, bool duplicateStartEndPoint=0);
    void trianglateRectangle(std::vector<QVector3D>& triangulatedMesh, std::vector<QVector3D> verticesCcw, bool invertNormal);

private:
    QGeoCoordinate _gpsRefPoint;
    QMap<uint64_t, QGeoCoordinate> _mapNodes;
    QMap<uint64_t, BuildingType> _mapBuildings;
    QGeoCoordinate _coordinate_min, _coordinate_max; //Osm map bounding boxes in global coordinate

    bool _gpsRefSet;
    float _buildingLevelHeight;
    bool _mapLoadedFlag;
    Viewer3DSettings* _viewer3DSettings = nullptr;
    QList<QString> _singleStoreyBuildings;
    QList<QString> _doubleStoreyLeisure;


signals:
    void gpsRefChanged(QGeoCoordinate newGpsRef, bool isRefSet);
    void mapChanged();
    void buildingLevelHeightChanged(void);

private slots:
    void setBuildingLevelHeight(QVariant value);

};

#endif // OSMPARSER_H
