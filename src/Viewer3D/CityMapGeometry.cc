/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CityMapGeometry.h"
#include "SettingsManager.h"
#include "Viewer3DSettings.h"
#include "OsmParser.h"


CityMapGeometry::CityMapGeometry()
{
    _osmParser = nullptr;
    _modelName = "city_map_defualt_name";
    _vertexData.clear();
    _mapLoadedFlag = 0;

    _viewer3DSettings = SettingsManager::instance()->viewer3DSettings();

    setOsmFilePath(_viewer3DSettings->osmFilePath()->rawValue());
    connect(_viewer3DSettings->osmFilePath(), &Fact::rawValueChanged, this, &CityMapGeometry::setOsmFilePath);
}

void CityMapGeometry::setModelName(QString modelName)
{
    _modelName = modelName;
    //    setName(_modelName);

    emit modelNameChanged();
}

void CityMapGeometry::setOsmFilePath(QVariant value)
{
    clearViewer();
    _mapLoadedFlag = 0;
    _osmFilePath = value.toString();
    emit osmFilePathChanged();
    loadOsmMap();
}

void CityMapGeometry::setOsmParser(OsmParser *newOsmParser)
{
    _osmParser = newOsmParser;

    if(_osmParser){
        connect(_osmParser, &OsmParser::buildingLevelHeightChanged, this, &CityMapGeometry::updateViewer);
        connect(_osmParser, &OsmParser::mapChanged, this, &CityMapGeometry::updateViewer);
    }
    emit osmParserChanged();
    loadOsmMap();
}

bool CityMapGeometry::loadOsmMap()
{
    if(!_osmParser){
        return false;
    }

    _osmParser->parseOsmFile(_osmFilePath);
    return false;
}

void CityMapGeometry::updateViewer()
{
    clear();

    if(!_osmParser){
        return;
    }

    if(_osmParser->mapLoaded()){
        _vertexData = _osmParser->buildingToMesh();

        int stride = 3 * sizeof(float);
        if(!_vertexData.isEmpty()){
            setVertexData(_vertexData);
            setStride(stride);

            setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);

            addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,
                         0,
                         QQuick3DGeometry::Attribute::F32Type);
        }
        update();
    }
}

void CityMapGeometry::clearViewer()
{
    clear();
    _vertexData.clear();
    update();
}
