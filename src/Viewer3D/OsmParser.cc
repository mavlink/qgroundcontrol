/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "OsmParser.h"
#include "SettingsManager.h"
#include "Viewer3DSettings.h"
#include "OsmParserThread.h"
#include "earcut.hpp"

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
    _osmParserWorker = new OsmParserThread();
    _singleStoreyBuildings.append("bungalow");
    _singleStoreyBuildings.append("shed");
    _singleStoreyBuildings.append("kiosk");
    _singleStoreyBuildings.append("cabin");

    _doubleStoreyLeisure.append("stadium");
    _doubleStoreyLeisure.append("sports_hall");
    _doubleStoreyLeisure.append("sauna");

    _viewer3DSettings = SettingsManager::instance()->viewer3DSettings();

    _gpsRefSet = false;
    _mapLoadedFlag = false;

    setBuildingLevelHeight(_viewer3DSettings->buildingLevelHeight()->rawValue()); // meters
    connect(_viewer3DSettings->buildingLevelHeight(), &Fact::rawValueChanged, this, &OsmParser::setBuildingLevelHeight);
    connect(_osmParserWorker, &OsmParserThread::fileParsed, this, &OsmParser::osmParserFinished);
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

void OsmParser::osmParserFinished(bool isValid)
{
    if(isValid){
        if(!_gpsRefSet) {
            setGpsRef(_osmParserWorker->gpsRefPoint);

            _coordinateMin = _osmParserWorker->coordinateMin;
            _coordinateMax = _osmParserWorker->coordinateMax;
        }
        _mapLoadedFlag = true;
        emit mapChanged();
        qDebug() << _osmParserWorker->mapBuildings.size() << " Buildings loaded!!!";
    }
}

void OsmParser::parseOsmFile(QString filePath)
{
    _osmParserWorker->mapNodes.clear();
    _osmParserWorker->mapBuildings.clear();
    _gpsRefSet = false;
    _mapLoadedFlag = false;
    resetGpsRef();

    _osmParserWorker->start(filePath);
}

QByteArray OsmParser::buildingToMesh()
{
    QByteArray vertexData;
    QMapIterator<uint64_t, OsmParserThread::BuildingType_t> ii(_osmParserWorker->mapBuildings);

    for (auto ii = _osmParserWorker->mapBuildings.begin(), end = _osmParserWorker->mapBuildings.end(); ii != end; ++ii) {
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
