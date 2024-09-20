/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtXml/QDomDocument>
#include <QtCore/QMap>
#include <QtGui/QVector3D>
#include <QtGui/QVector2D>
#include <QtPositioning/QGeoCoordinate>

///     @author Omid Esrafilian <esrafilian.omid@gmail.com>


class OsmParserThread : public QThread
{

public:
    typedef struct BuildingType_s
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
    }BuildingType_t;

    Q_OBJECT
public:
    explicit OsmParserThread(QObject *parent = nullptr);

    QGeoCoordinate gpsRefPoint;
    QMap<uint64_t, QGeoCoordinate> mapNodes;
    QMap<uint64_t, BuildingType_t> mapBuildings;
    QGeoCoordinate coordinateMin, coordinateMax;

    void start(QString filePath);

private:
    QThread* _mainThread;
    bool _mapLoadedFlag;
    QList<QString> _singleStoreyBuildings;
    QList<QString> _doubleStoreyLeisure;

    void parseOsmFile(QString filePath);
    bool decodeFile(QDomDocument& xml_content, QMap<uint64_t, BuildingType_t > &buildingMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate& coordinateMin, QGeoCoordinate& coordinateMax, QGeoCoordinate& gpsRef);
    bool decodeNodeTags(QDomElement& xmlComponent, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate& coordMin, QGeoCoordinate& coordMax, QGeoCoordinate& gpsRef);
    void decodeBuildings(QDomElement& xmlComponent, QMap<uint64_t, BuildingType_t > &bldMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate& coordMin, QGeoCoordinate& coordMax, QGeoCoordinate gpsRef);
    void decodeRelations(QDomElement& xmlComponent, QMap<uint64_t, BuildingType_t > &bldMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate gpsRef);


signals:
    void fileParsed(bool isValid);
    void startThread(QString filePath);

private slots:
    void startThreadEvent(QString filePath);
};
