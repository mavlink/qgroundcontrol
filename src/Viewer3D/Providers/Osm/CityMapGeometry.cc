#include "CityMapGeometry.h"

#include "OsmParser.h"
#include "QGCLoggingCategory.h"
#include "SettingsManager.h"
#include "Viewer3DMapProvider.h"
#include "Viewer3DSettings.h"

QGC_LOGGING_CATEGORY(CityMapGeometryLog, "Viewer3d.CityMapGeometry")

CityMapGeometry::CityMapGeometry()
    : _modelName(QStringLiteral("city_map_default_name"))
{
    Viewer3DSettings* viewer3DSettings = SettingsManager::instance()->viewer3DSettings();

    _setOsmFilePath(viewer3DSettings->osmFilePath()->rawValue());
    connect(viewer3DSettings->osmFilePath(), &Fact::rawValueChanged, this, &CityMapGeometry::_setOsmFilePath);
}

void CityMapGeometry::setModelName(const QString &modelName)
{
    if (_modelName == modelName) {
        return;
    }
    _modelName = modelName;
    emit modelNameChanged();
}

void CityMapGeometry::_setOsmFilePath(const QVariant &value)
{
    _clearViewer();
    _osmFilePath = value.toString();
    qCDebug(CityMapGeometryLog) << "OSM file path set to:" << _osmFilePath;
    emit osmFilePathChanged();
    _loadOsmMap();
}

void CityMapGeometry::setMapProvider(Viewer3DMapProvider *newMapProvider)
{
    if (_mapProvider == newMapProvider) {
        return;
    }

    if (_osmParser) {
        disconnect(_osmParser, &OsmParser::buildingLevelHeightChanged, this, &CityMapGeometry::_updateViewer);
    }
    if (_mapProvider) {
        disconnect(_mapProvider, &Viewer3DMapProvider::mapChanged, this, &CityMapGeometry::_updateViewer);
    }

    _mapProvider = newMapProvider;
    _osmParser = qobject_cast<OsmParser*>(_mapProvider);

    if (_osmParser) {
        connect(_osmParser, &OsmParser::buildingLevelHeightChanged, this, &CityMapGeometry::_updateViewer);
    }
    if (_mapProvider) {
        connect(_mapProvider, &Viewer3DMapProvider::mapChanged, this, &CityMapGeometry::_updateViewer);
    }

    emit mapProviderChanged();
    _loadOsmMap();
}

bool CityMapGeometry::_loadOsmMap()
{
    if (!_osmParser) {
        return false;
    }

    _osmParser->parseOsmFile(_osmFilePath);
    return true;
}

void CityMapGeometry::_updateViewer()
{
    clear();

    if (!_osmParser) {
        qCDebug(CityMapGeometryLog) << "updateViewer: no OSM parser set";
        return;
    }

    if (_osmParser->mapLoaded()) {
        _vertexData = _osmParser->buildingToMesh();
        qCDebug(CityMapGeometryLog) << "Building mesh generated:" << _vertexData.size() << "bytes";

        constexpr int stride = 3 * sizeof(float);
        if (!_vertexData.isEmpty()) {
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

void CityMapGeometry::_clearViewer()
{
    clear();
    _vertexData.clear();
    update();
}
