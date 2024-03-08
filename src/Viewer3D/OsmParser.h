#ifndef OSMPARSER_H
#define OSMPARSER_H

#include <QObject>
#include <QMap>
#include <QVector3D>
#include <QVector2D>
#include <QGeoCoordinate>
#include <QVariant>
#include <QDomElement>

#include "OsmParserThread.h"
///     @author Omid Esrafilian <esrafilian.omid@gmail.com>

class Viewer3DSettings;

class OsmParser : public QObject
{
    Q_OBJECT

    // Q_PROPERTY(float buildingLevelHeight READ buildingLevelHeight WRITE setBuildingLevelHeight NOTIFY buildingLevelHeightChanged)

public:
    explicit OsmParser(QObject *parent = nullptr);

    bool mapLoaded(){return _mapLoadedFlag;}
    void setGpsRef(QGeoCoordinate gpsRef);
    void resetGpsRef();
    QGeoCoordinate getGpsRef(){ return _gpsRefPoint;}

    float buildingLevelHeight(void){return _buildingLevelHeight;}
    void parseOsmFile(QString filePath);

    QByteArray buildingToMesh();

    void trianglateWallsExtrudedPolygon(std::vector<QVector3D>& triangulatedMesh, std::vector<QVector2D> verticesCcw, float h, bool inverseOrder=0, bool duplicateStartEndPoint=0);
    void trianglateRectangle(std::vector<QVector3D>& triangulatedMesh, std::vector<QVector3D> verticesCcw, bool invertNormal);
    std::pair<QGeoCoordinate, QGeoCoordinate> getMapBoundingBoxCoordinate(){ return std::pair(_coordinateMin, _coordinateMax);}

private:
    OsmParserThread* _osmParserWorker;
    QGeoCoordinate _gpsRefPoint;
    QGeoCoordinate _coordinateMin, _coordinateMax; //Osm map bounding boxes in global coordinate


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
    void osmParserFinished(bool isValid);


};

#endif // OSMPARSER_H
