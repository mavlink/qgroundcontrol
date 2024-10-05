/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "OsmParserThread.h"
#include "Viewer3DUtils.h"

#include <QtCore/QFile>

OsmParserThread::OsmParserThread(QObject *parent)
    : QThread{parent}
{
    _mainThread = new QThread();
    _singleStoreyBuildings.append("bungalow");
    _singleStoreyBuildings.append("shed");
    _singleStoreyBuildings.append("kiosk");
    _singleStoreyBuildings.append("cabin");

    _doubleStoreyLeisure.append("stadium");
    _doubleStoreyLeisure.append("sports_hall");
    _doubleStoreyLeisure.append("sauna");

    connect(this, &OsmParserThread::startThread, this, &OsmParserThread::startThreadEvent);

    this->moveToThread(_mainThread);
    _mainThread->start();
}

void OsmParserThread::start(QString filePath)
{
    emit startThread(filePath);
}

void OsmParserThread::parseOsmFile(QString filePath)
{
    mapNodes.clear();
    mapBuildings.clear();
    _mapLoadedFlag = false;


    if(filePath == "Please select an OSM file"){
        if(_mapLoadedFlag){
            qDebug("The 3D View has been cleared!");
        }else{
            qDebug("No OSM File is selected!");
        }
        return;
    }

    //The QDomDocument class represents an XML document.
    QDomDocument xml_content;
// Load xml file as raw data
#ifdef __unix__
    filePath = QString("/") + filePath;
#endif
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly )) {
        // Error while loading file
        qDebug() << "Error while loading OSM file" << filePath;
        return;
    }
    qDebug("Loading the OSM file!!!");
    // Set data into the QDomDocument before processing
    xml_content.setContent(&f);
    f.close();

    if(decodeFile(xml_content, mapBuildings, mapNodes, coordinateMin, coordinateMax, gpsRefPoint)){
        _mapLoadedFlag = true;
        emit fileParsed(true);
        return;
    }

    emit fileParsed(false);
}

bool OsmParserThread::decodeFile(QDomDocument &xml_content, QMap<uint64_t, OsmParserThread::BuildingType_t> &buildingMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate &coordinateMin, QGeoCoordinate &coordinateMax, QGeoCoordinate &gpsRef)
{
    QDomElement root = xml_content.documentElement();
    QDomElement xmlComponent = root.firstChild().toElement();
    QGeoCoordinate tmpGpsRef;
    bool gpsRefIsSet = false;
    while(!xmlComponent.isNull()) {
        if(decodeNodeTags(xmlComponent, nodeMap, coordinateMin, coordinateMax, tmpGpsRef)){
            gpsRefIsSet = true;
            gpsRef = tmpGpsRef;
        }
        decodeBuildings(xmlComponent, buildingMap, nodeMap, coordinateMin, coordinateMax, gpsRef);
        decodeRelations(xmlComponent, buildingMap, nodeMap, gpsRef);

        xmlComponent = xmlComponent.nextSibling().toElement();
    }
    return gpsRefIsSet;
}

bool OsmParserThread::decodeNodeTags(QDomElement &xmlComponent, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate &coordMin, QGeoCoordinate &coordMax, QGeoCoordinate &gpsRef)
{
    int64_t id_tmp=0;
    QGeoCoordinate gps_tmp;
    QString attribute;
    bool gpsRefIsSet = false;

    if (xmlComponent.tagName()=="node") {

        attribute = xmlComponent.attribute("id","-1");
        id_tmp = attribute.toLongLong();

        attribute = xmlComponent.attribute("lat","0");
        gps_tmp.setLatitude(attribute.toDouble());

        attribute = xmlComponent.attribute("lon","0");
        gps_tmp.setLongitude(attribute.toDouble());

        gps_tmp.setAltitude(0);

        if(id_tmp > 0) {
            nodeMap.insert((uint64_t)id_tmp, gps_tmp);
        }
    }else if(xmlComponent.tagName() == "bounds") {
        coordMin.setLatitude(xmlComponent.attribute("minlat","0").toFloat());
        coordMin.setLongitude(xmlComponent.attribute("minlon","0").toFloat());
        coordMin.setAltitude(0);
        coordMax.setLatitude(xmlComponent.attribute("maxlat","0").toFloat());
        coordMax.setLongitude(xmlComponent.attribute("maxlon","0").toFloat());
        coordMax.setAltitude(0);

        gpsRefIsSet = true;
        gps_tmp.setLatitude(0.5 * (coordMin.latitude() + coordMax.latitude()));
        gps_tmp.setLongitude(0.5 * (coordMin.longitude() + coordMax.longitude()));
        gps_tmp.setAltitude(0);
        gpsRef = gps_tmp;
    }
    return gpsRefIsSet;
}

void OsmParserThread::decodeBuildings(QDomElement &xmlComponent, QMap<uint64_t, OsmParserThread::BuildingType_t> &bldMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate &coordMin, QGeoCoordinate &coordMax, QGeoCoordinate gpsRef)
{
    if (xmlComponent.tagName()!="way"){
        return;
    }

    int64_t id_tmp = xmlComponent.attribute("id","0").toLongLong();
    if(id_tmp == 0) {
        return;
    }
    OsmParserThread::BuildingType_t bld_tmp;
    QGeoCoordinate gps_pt_tmp;
    QVector3D local_pt_tmp;
    std::vector<QGeoCoordinate> bld_points;
    std::vector<QVector2D> bld_points_local;
    double bld_lon_max, bld_lon_min, bld_lat_max, bld_lat_min;
    double bld_x_max, bld_x_min, bld_y_max, bld_y_min;
    bld_x_max = bld_y_max = -1e10;
    bld_x_min = bld_y_min = 1e10;
    bld_lon_max = bld_lat_max = -1e10;
    bld_lon_min = bld_lat_min = 1e10;

    int64_t ref_id;
    QDomElement Child = xmlComponent.firstChild().toElement();
    QString attribute;

    bld_tmp.height = 0;
    bld_tmp.levels = 0;

    while (!Child.isNull()) {
        if (Child.tagName()=="nd") {
            ref_id = Child.attribute("ref","0").toLongLong();

            if(ref_id > 0) {
                gps_pt_tmp = nodeMap[ref_id];
                bld_points.push_back(gps_pt_tmp);
                local_pt_tmp = mapGpsToLocalPoint(gps_pt_tmp, gpsRef);
                bld_points_local.push_back(QVector2D(local_pt_tmp.x(), local_pt_tmp.y()));

                bld_x_max = (bld_x_max < local_pt_tmp.x())?(local_pt_tmp.x()):(bld_x_max);
                bld_y_max = (bld_y_max < local_pt_tmp.y())?(local_pt_tmp.y()):(bld_y_max);
                bld_x_min = (bld_x_min > local_pt_tmp.x())?(local_pt_tmp.x()):(bld_x_min);
                bld_y_min = (bld_y_min > local_pt_tmp.y())?(local_pt_tmp.y()):(bld_y_min);

                bld_lon_max = fmax(bld_lon_max, gps_pt_tmp.longitude());
                bld_lat_max = fmax(bld_lat_max, gps_pt_tmp.latitude());
                bld_lon_min = fmin(bld_lon_min, gps_pt_tmp.longitude());
                bld_lat_min = fmin(bld_lat_min, gps_pt_tmp.latitude());
            }
        }else if (Child.tagName()=="tag") {
            attribute = Child.attribute("k","0");
            if(attribute == "building:levels") {
                bld_tmp.levels = Child.attribute("v","0").toFloat();
            }else if(attribute == "height") {
                bld_tmp.height = Child.attribute("v","0").toFloat();
            }else if(attribute == "building" && bld_tmp.levels == 0 && bld_tmp.height == 0){
                QString attribute_2 = Child.attribute("v","0");
                if(_singleStoreyBuildings.contains(attribute_2)){
                    bld_tmp.levels = 1;
                }else{
                    bld_tmp.levels = 2;
                }
            }else if(attribute == "leisure" && bld_tmp.levels == 0 && bld_tmp.height == 0){
                QString attribute_2 = Child.attribute("v","0");
                if(_doubleStoreyLeisure.contains(attribute_2)){
                    bld_tmp.levels = 2;
                }
            }
        }

        Child = Child.nextSibling().toElement();
    }

    if(bld_points.size() > 2) {
        //        float bld_height = (bld_tmp.height >= bld_tmp.levels * _buildingLevelHeight)?(bld_tmp.height):(bld_tmp.levels * _buildingLevelHeight);
        //        bld_tmp.height = bld_height;
        if(bld_tmp.levels > 0 || bld_tmp.height > 0){
            coordMin.setLatitude(fmin(coordMin.latitude(), bld_lat_min));
            coordMin.setLongitude(fmin(coordMin.longitude(), bld_lon_min));
            coordMax.setLatitude(fmax(coordMax.latitude(), bld_lat_max));
            coordMax.setLongitude(fmax(coordMax.longitude(), bld_lon_max));
        }
        bld_tmp.points_gps = bld_points;
        bld_tmp.points_local = bld_points_local;
        bld_tmp.bb_max = QVector2D(bld_x_max, bld_y_max);
        bld_tmp.bb_min = QVector2D(bld_x_min, bld_y_min);
        bldMap.insert(id_tmp, bld_tmp);
    }
}

void OsmParserThread::decodeRelations(QDomElement &xmlComponent, QMap<uint64_t, OsmParserThread::BuildingType_t> &bldMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate gpsRef)
{
    if (xmlComponent.tagName()!="relation"){
        return;
    }

    int64_t id_tmp = xmlComponent.attribute("id","0").toLongLong();
    if(id_tmp == 0) {
        return;
    }

    OsmParserThread::BuildingType_t bld_tmp;
    int64_t ref_id;
    QDomElement Child = xmlComponent.firstChild().toElement();
    QString attribute;
    QString role;

    bld_tmp.height = 0;
    bld_tmp.levels = 0;
    std::vector<int64_t> bldToBeRemoved;
    bool isBuilding = false;
    bool isMultipolygon = false;

    while (!Child.isNull()) {
        if (Child.tagName()=="member") {
            ref_id = Child.attribute("ref","0").toLongLong();
            role = Child.attribute("role","0");
            auto bldItem = bldMap.find(ref_id);
            if(bldItem != bldMap.end()) {
                bld_tmp.append(bldItem.value().points_local, role == "inner");
                bld_tmp.append(bldItem.value().points_gps, role == "inner");
                bld_tmp.levels = fmax(bld_tmp.levels, bldItem.value().levels);
                bld_tmp.height = fmax(bld_tmp.height, bldItem.value().height);

                bld_tmp.bb_max[0] = fmax(bld_tmp.bb_max[0], bldItem.value().bb_max[0]);
                bld_tmp.bb_max[1] = fmax(bld_tmp.bb_max[1], bldItem.value().bb_max[1]);
                bld_tmp.bb_min[0] = fmin(bld_tmp.bb_min[0], bldItem.value().bb_min[0]);
                bld_tmp.bb_min[1] = fmin(bld_tmp.bb_min[1], bldItem.value().bb_min[1]);
                bldToBeRemoved.push_back(ref_id);
            }
        }else if (Child.tagName()=="tag") {
            attribute = Child.attribute("k","0");
            if(attribute == "type") {
                if(Child.attribute("v","0") == "multipolygon"){
                    isMultipolygon = true;
                }
            }else if(attribute == "building"){
                isBuilding = true;
            }
        }
        Child = Child.nextSibling().toElement();
    }

    if(isBuilding){
        if(bld_tmp.height == 0){
            bld_tmp.levels = (bld_tmp.levels == 0)?(2):(bld_tmp.levels);
        }
    }
    if(isMultipolygon){
        for(uint i_id=0; i_id<bldToBeRemoved.size(); i_id++){
            bldMap.remove(bldToBeRemoved[i_id]);
        }
        bldMap.insert(bldToBeRemoved[0], bld_tmp);
    }
}

void OsmParserThread::startThreadEvent(QString filePath)
{
    parseOsmFile(filePath);
}
