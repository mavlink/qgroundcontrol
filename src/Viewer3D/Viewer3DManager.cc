#include "Viewer3DManager.h"
#include "OsmParser.h"
#include "Viewer3DQmlBackend.h"

Viewer3DManager::Viewer3DManager()
{
    _qmlBackend  = new Viewer3DQmlBackend(this);
    _osmParser = new OsmParser();

    _qmlBackend->init(_osmParser);
}

Viewer3DManager::~Viewer3DManager()
{
    delete _osmParser;
    delete _qmlBackend;
}
