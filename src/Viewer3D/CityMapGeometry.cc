#include "CityMapGeometry.h"

#include<QThread>


CityMapGeometry::CityMapGeometry()
{
    _osmParser = nullptr;
    _modelName = "city_map_defualt_name";
    _vertexData.clear();
    _mapLoadedFlag = 0;
}

void CityMapGeometry::setModelName(QString modelName)
{
    _modelName = modelName;
    //    setName(_modelName);

    emit modelNameChanged();
}

void CityMapGeometry::setOsmFilePath(QString filePath)
{
    if(_osmFilePath.compare(filePath) == 0){
        return;
    }

    _mapLoadedFlag = 0;
    _osmFilePath = filePath;
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
    if(_mapLoadedFlag){
        return true;
    }

    if(!_osmParser){
        return false;
    }
    _mapLoadedFlag = 1;
    _osmParser->parseOsmFile(_osmFilePath);
    return true;
}

void CityMapGeometry::updateViewer()
{
    clear();

    if(!_osmParser){
        return;
    }

    if(loadOsmMap()){
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
