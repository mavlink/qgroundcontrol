#include "Viewer3DManager.h"


Viewer3DManager::Viewer3DManager()
{
    _qmlBackend  = new Viewer3DQmlBackend(this);
    _osmParser = new OsmParser();
    _viewer3DSetting = new Viewer3DSettings();

    _osmParser->setBuildingLevelHeight(_viewer3DSetting->buildingLevelHeight()->rawValue().toFloat()); // meters
    _qmlBackend->init(_viewer3DSetting, _osmParser);
}
