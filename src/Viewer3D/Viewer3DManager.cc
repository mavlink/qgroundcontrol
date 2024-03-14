#include "Viewer3DManager.h"


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
