#include "MapEngine.h"

#include <QtCore/QApplicationStatic>
#include <QtNetwork/QNetworkAccessManager>

#include "Fact.h"
#include "MapProvider.h"
#include "MapProviderRegistry.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "TileServerPauses.h"

QGC_LOGGING_CATEGORY(MapEngineLog, "MapTiles.MapEngine")

Q_APPLICATION_STATIC(MapEngine, _mapEngine);

MapEngine::MapEngine(QObject* parent)
    : QObject(parent)
{
    qCDebug(MapEngineLog) << this;

    for (const SharedMapProvider& provider : MapProviderRegistry::providers()) {
        const int mapId = provider->mapId();
        for (const Fact* const setting : provider->cacheVariantSettings()) {
            (void) connect(setting, &Fact::rawValueChanged, this, [this, mapId] { emit tileSourceChanged(mapId); });
        }
    }
}

MapEngine::~MapEngine()
{
    qCDebug(MapEngineLog) << this;
}

MapEngine* MapEngine::instance()
{
    return _mapEngine();
}

QNetworkAccessManager* MapEngine::networkManager()
{
    if (_testNetworkManager) {
        return _testNetworkManager;
    }

    if (!_networkManager) {
        _networkManager = QGCNetworkHelper::createNetworkManager(this);
    }
    return _networkManager;
}

void MapEngine::setNetworkManagerForTest(QNetworkAccessManager* networkManager)
{
    _testNetworkManager = networkManager;
}

TileServerPauses* MapEngine::serverPauses()
{
    if (_testServerPauses) {
        return _testServerPauses;
    }

    if (!_serverPauses) {
        _serverPauses = new TileServerPauses(nullptr, this);
    }
    return _serverPauses;
}

void MapEngine::setServerPausesForTest(TileServerPauses* serverPauses)
{
    _testServerPauses = serverPauses;
}
