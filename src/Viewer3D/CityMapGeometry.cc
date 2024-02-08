#include "CityMapGeometry.h"

#include<QThread>


CityMapGeometry::CityMapGeometry()
{
    _osmParser = nullptr;
    _modelName = "city_map_defualt_name";
    _vertexData.clear();
    _mapLoadedFlag = 0;

    connect(this, &CityMapGeometry::osmFilePathChanged, this, &CityMapGeometry::updateData);
    connect(this, &CityMapGeometry::osmParserChanged, this, &CityMapGeometry::updateData);
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

    updateData();
}

void CityMapGeometry::setOsmParser(OsmParser *newOsmParser)
{
    _osmParser = newOsmParser;

    if(_osmParser){
        connect(_osmParser, &OsmParser::buildingLevelHeightChanged, this, &CityMapGeometry::updateData);
    }
    emit osmParserChanged();
}

//! [update data]
void CityMapGeometry::updateData()
{
    clear();

    if(!_osmParser){
        return;
    }

    if(_mapLoadedFlag == 0){
        _osmParser->parseOsmFile(_osmFilePath);
        _mapLoadedFlag = 1;
    }

    if(_mapLoadedFlag){
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
