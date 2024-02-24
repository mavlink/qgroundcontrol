#include "CityMapGeometry.h"

#include<QThread>

#include "QGCApplication.h"
#include "SettingsManager.h"


CityMapGeometry::CityMapGeometry()
{
    _osmParser = nullptr;
    _modelName = "city_map_defualt_name";
    _vertexData.clear();
    _mapLoadedFlag = 0;

    _viewer3DSettings = qgcApp()->toolbox()->settingsManager()->viewer3DSettings();

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
    if(_osmFilePath.compare(value.toString()) == 0){
        return;
    }

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

void CityMapGeometry::clearViewer()
{
    clear();
    _vertexData.clear();
    update();
}
