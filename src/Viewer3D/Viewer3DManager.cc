#include "Viewer3DManager.h"


Viewer3DManager::Viewer3DManager()
{
    _qmlBackend  = new Viewer3DQmlBackend(this);
    _osmParser = new OsmParser();

    // _osmParser->setBuildingLevelHeight(qgcApp()->toolbox()->settingsManager()->viewer3DSettings()->buildingLevelHeight()->rawValue().toFloat()); // meters
    _qmlBackend->init(_osmParser);
}
