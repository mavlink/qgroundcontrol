#include "OsmParser.h"
#include "earcut.hpp"
#include "Viewer3DUtils.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

typedef union {
    uint array[3];

    struct {
        uint x;
        uint y;
        uint z;
    } axis;
} vec3i;

OsmParser::OsmParser(QObject *parent)
    : QObject{parent}
{
    _singleStoreyBuildings.append("bungalow");
    _singleStoreyBuildings.append("shed");
    _singleStoreyBuildings.append("kiosk");
    _singleStoreyBuildings.append("cabin");

    _doubleStoreyLeisure.append("stadium");
    _doubleStoreyLeisure.append("sports_hall");
    _doubleStoreyLeisure.append("sauna");

    _viewer3DSettings = qgcApp()->toolbox()->settingsManager()->viewer3DSettings();

    _gpsRefSet = false;
    _mapLoadedFlag = false;

    setBuildingLevelHeight(_viewer3DSettings->buildingLevelHeight()->rawValue()); // meters
    connect(_viewer3DSettings->buildingLevelHeight(), &Fact::rawValueChanged, this, &OsmParser::setBuildingLevelHeight);
}

void OsmParser::setGpsRef(QGeoCoordinate gpsRef)
{
    _gpsRefPoint = gpsRef;
    _gpsRefSet = true;
    emit gpsRefChanged(_gpsRefPoint, _gpsRefSet);
}

void OsmParser::resetGpsRef()
{
    _gpsRefPoint = QGeoCoordinate(0, 0, 0);
    _gpsRefSet = false;
    emit gpsRefChanged(_gpsRefPoint, _gpsRefSet);
}

void OsmParser::setBuildingLevelHeight(QVariant value)
{
    _buildingLevelHeight = value.toFloat();
    emit buildingLevelHeightChanged();
}

void OsmParser::parseOsmFile(QString filePath)
{
    _mapNodes.clear();
    _mapBuildings.clear();
    _gpsRefSet = false;
    _mapLoadedFlag = false;
    resetGpsRef();

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

    // Extract the root markup
    QDomElement root = xml_content.documentElement();

    QDomElement component = root.firstChild().toElement();

    while(!component.isNull()) {
        decodeNodeTags(component, _mapNodes);
        decodeBuildings(component, _mapBuildings, _mapNodes, _gpsRefPoint);
        decodeRelations(component, _mapBuildings, _mapNodes, _gpsRefPoint);

        component = component.nextSibling().toElement();
    }
    _mapLoadedFlag = true;
    emit mapChanged();
    qDebug() << _mapBuildings.size() << " Buildings loaded!!!";
}

void OsmParser::decodeNodeTags(QDomElement &xmlComponent, QMap<uint64_t, QGeoCoordinate> &nodeMap)
{
    int64_t id_tmp=0;
    QGeoCoordinate gps_tmp;
    QString attribute;

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
        _coordinate_min.setLatitude(xmlComponent.attribute("minlat","0").toFloat());
        _coordinate_min.setLongitude(xmlComponent.attribute("minlon","0").toFloat());
        _coordinate_min.setAltitude(0);
        _coordinate_max.setLatitude(xmlComponent.attribute("maxlat","0").toFloat());
        _coordinate_max.setLongitude(xmlComponent.attribute("maxlon","0").toFloat());
        _coordinate_max.setAltitude(0);

        if(!_gpsRefSet) {
            gps_tmp.setLatitude(0.5 * (_coordinate_min.latitude() + _coordinate_max.latitude()));
            gps_tmp.setLongitude(0.5 * (_coordinate_min.longitude() + _coordinate_max.longitude()));
            gps_tmp.setAltitude(0);
            setGpsRef(gps_tmp);
        }
    }
}

void OsmParser::decodeBuildings(QDomElement &xmlComponent, QMap<uint64_t, BuildingType> &buildingMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate gpsRef)
{
    if (xmlComponent.tagName()!="way"){
        return;
    }

    int64_t id_tmp = xmlComponent.attribute("id","0").toLongLong();
    if(id_tmp == 0) {
        return;
    }
    BuildingType bld_tmp;
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
            _coordinate_min.setLatitude(fmin(_coordinate_min.latitude(), bld_lat_min));
            _coordinate_min.setLongitude(fmin(_coordinate_min.longitude(), bld_lon_min));
            _coordinate_max.setLatitude(fmax(_coordinate_max.latitude(), bld_lat_max));
            _coordinate_max.setLongitude(fmax(_coordinate_max.longitude(), bld_lon_max));
        }
        bld_tmp.points_gps = bld_points;
        bld_tmp.points_local = bld_points_local;
        bld_tmp.bb_max = QVector2D(bld_x_max, bld_y_max);
        bld_tmp.bb_min = QVector2D(bld_x_min, bld_y_min);
        buildingMap.insert(id_tmp, bld_tmp);
    }
}

void OsmParser::decodeRelations(QDomElement &xmlComponent, QMap<uint64_t, BuildingType> &buildingMap, QMap<uint64_t, QGeoCoordinate> &nodeMap, QGeoCoordinate gpsRef)
{
    if (xmlComponent.tagName()!="relation"){
        return;
    }

    int64_t id_tmp = xmlComponent.attribute("id","0").toLongLong();
    if(id_tmp == 0) {
        return;
    }

    BuildingType bld_tmp;
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
            auto bldItem = buildingMap.find(ref_id);
            if(bldItem != buildingMap.end()) {
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
            buildingMap.remove(bldToBeRemoved[i_id]);
        }
        buildingMap.insert(bldToBeRemoved[0], bld_tmp);
    }
}

QByteArray OsmParser::buildingToMesh()
{
    QByteArray vertexData;
    QMapIterator<uint64_t, BuildingType> ii(_mapBuildings);

    for (auto ii = _mapBuildings.begin(), end = _mapBuildings.end(); ii != end; ++ii) {
        float bld_height = 0;
        // std::vector<std::array<float, 2> > bld_points;
        // std::vector<std::vector<std::array<float, 2> > > polygon;

        std::vector<std::array<float, 2> > all_bld_points;
        std::vector<std::array<float, 2> > bld_points;
        std::vector<std::vector<std::array<float, 2> > > polygon;
        std::vector<QVector3D> triangulated_mesh;

        //        bld_height = (ii.value().height >= ii.value().levels * _buildingLevelHeight)?(ii.value().height):(ii.value().levels * _buildingLevelHeight);

        if(ii.value().height > 0){
            bld_height = ii.value().height;
        }else if(ii.value().levels > 0){
            bld_height = (float)(ii.value().levels) * _buildingLevelHeight;
        }else{
            continue;
        }

        for(unsigned int jj=0; jj<ii.value().points_local.size(); jj++) {
            bld_points.push_back({ii.value().points_local[jj].x(), ii.value().points_local[jj].y()});
            all_bld_points.push_back({ii.value().points_local[jj].x(), ii.value().points_local[jj].y()});
        }
        polygon.push_back(bld_points);

        bld_points.clear();
        for(unsigned int jj=0; jj<ii.value().points_local_inner.size(); jj++) {
            bld_points.push_back({ii.value().points_local_inner[jj].x(), ii.value().points_local_inner[jj].y()});
            all_bld_points.push_back({ii.value().points_local_inner[jj].x(), ii.value().points_local_inner[jj].y()});
        }
        if(bld_points.size() > 0){
            polygon.push_back(bld_points);
        }

        std::vector<uint32_t> indices = mapbox::earcut<uint32_t>(polygon);

        for(uint i_i=0; i_i<indices.size(); i_i+=3) {
            // mesh for roof
            uint n_idx = indices[i_i];
            triangulated_mesh.push_back(QVector3D(all_bld_points[n_idx][0], all_bld_points[n_idx][1], bld_height));
            n_idx = indices[i_i+1];
            triangulated_mesh.push_back(QVector3D(all_bld_points[n_idx][0], all_bld_points[n_idx][1], bld_height));
            n_idx = indices[i_i+2];
            triangulated_mesh.push_back(QVector3D(all_bld_points[n_idx][0], all_bld_points[n_idx][1], bld_height));

            // mesh for floor
            n_idx = indices[i_i+2];
            triangulated_mesh.push_back(QVector3D(all_bld_points[n_idx][0], all_bld_points[n_idx][1], 0));
            n_idx = indices[i_i+1];
            triangulated_mesh.push_back(QVector3D(all_bld_points[n_idx][0], all_bld_points[n_idx][1], 0));
            n_idx = indices[i_i];
            triangulated_mesh.push_back(QVector3D(all_bld_points[n_idx][0], all_bld_points[n_idx][1], 0));
        }

        if(bld_height > 0) {
            trianglateWallsExtrudedPolygon(triangulated_mesh, ii.value().points_local, bld_height, 0, 0); // mesh for wall outside
            trianglateWallsExtrudedPolygon(triangulated_mesh, ii.value().points_local, bld_height, 1, 0);// mesh for wall inside

            trianglateWallsExtrudedPolygon(triangulated_mesh, ii.value().points_local_inner, bld_height, 0, 0); // mesh for wall outside
            trianglateWallsExtrudedPolygon(triangulated_mesh, ii.value().points_local_inner, bld_height, 1, 0);// mesh for wall inside
        }

        QByteArray vertexData_tmp(triangulated_mesh.size() * 3 * sizeof(float), Qt::Initialization::Uninitialized);
        float *p = reinterpret_cast<float *>(vertexData_tmp.data());

        for(uint i_m=0; i_m<triangulated_mesh.size(); i_m++) {
            *p++ =  (float)triangulated_mesh[i_m].x(); *p++ =  (float)triangulated_mesh[i_m].y(); *p++ =  (float)triangulated_mesh[i_m].z();
        }

        ii.value().triangulated_mesh.insert(ii.value().triangulated_mesh.begin(), triangulated_mesh.begin(), triangulated_mesh.end());
        vertexData.append(vertexData_tmp);
    }
    return vertexData;
}

void OsmParser::trianglateWallsExtrudedPolygon(std::vector<QVector3D>& triangulatedMesh, std::vector<QVector2D> verticesCcw, float h, bool inverseOrder, bool duplicateStartEndPoint)
{
    std::vector<QVector3D> tmp_rec_ccw(4);
    uint vertices_size = verticesCcw.size() - (uint)(duplicateStartEndPoint);

    if(inverseOrder) {
        for(uint i_p=0; i_p<vertices_size; i_p++) {
            int i_p_p = (i_p < vertices_size-1)?(i_p+1):(0);
            tmp_rec_ccw[0] = QVector3D(verticesCcw[i_p_p].x(), verticesCcw[i_p_p].y(), 0);
            tmp_rec_ccw[1] = QVector3D(verticesCcw[i_p].x(), verticesCcw[i_p].y(), 0);
            tmp_rec_ccw[2] = QVector3D(verticesCcw[i_p].x(), verticesCcw[i_p].y(), h);
            tmp_rec_ccw[3] = QVector3D(verticesCcw[i_p_p].x(), verticesCcw[i_p_p].y(), h);
            trianglateRectangle(triangulatedMesh, tmp_rec_ccw, 0);
        }
        trianglateRectangle(triangulatedMesh, tmp_rec_ccw, 1);
    } else {
        for(uint i_p=0; i_p<vertices_size; i_p++) {
            int i_p_p = (i_p < vertices_size-1)?(i_p+1):(0);
            tmp_rec_ccw[0] = QVector3D(verticesCcw[i_p].x(), verticesCcw[i_p].y(), 0);
            tmp_rec_ccw[1] = QVector3D(verticesCcw[i_p_p].x(), verticesCcw[i_p_p].y(), 0);
            tmp_rec_ccw[2] = QVector3D(verticesCcw[i_p_p].x(), verticesCcw[i_p_p].y(), h);
            tmp_rec_ccw[3] = QVector3D(verticesCcw[i_p].x(), verticesCcw[i_p].y(), h);
            trianglateRectangle(triangulatedMesh, tmp_rec_ccw, 0);
        }
        trianglateRectangle(triangulatedMesh, tmp_rec_ccw, 1);
    }
}

void OsmParser::trianglateRectangle(std::vector<QVector3D>& triangulatedMesh, std::vector<QVector3D> verticesCcw, bool invertNormal)
{
    std::vector<vec3i> mesh_set_idx;
    mesh_set_idx.resize(2);

    if(invertNormal) {
        mesh_set_idx[0] = {{3, 1, 0}};
        mesh_set_idx[1] = {{3, 2, 1}};
    } else {
        mesh_set_idx[0] = {{0, 1, 3}};
        mesh_set_idx[1] = {{1, 2, 3}};
    }

    uint idx_tmp;

    for(uint i_m=0; i_m<mesh_set_idx.size(); i_m++) {
        for(uint i_v=0; i_v<3; i_v++) {
            idx_tmp = mesh_set_idx[i_m].array[i_v];
            triangulatedMesh.push_back(QVector3D(verticesCcw[idx_tmp].x(), verticesCcw[idx_tmp].y(), verticesCcw[idx_tmp].z()));
        }
    }
}
