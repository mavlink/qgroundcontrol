#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

class QNetworkAccessManager;
class TileServerPauses;

/// What the maps share besides the tile cache (TileCache): the network manager of their requests, the pauses of the
/// tile servers that asked to be asked less often, and the news that the source of a provider's tiles changed.
class MapEngine : public QObject
{
    Q_OBJECT

public:
    explicit MapEngine(QObject* parent = nullptr);
    ~MapEngine();

    /// The network manager for the requests of the maps: tiles, and lists such as the credits of a provider. The
    /// requests share it, and so its connections to a server. Created on first use, never null, and alive as long
    /// as the engine. For the main thread, where the engine lives.
    QNetworkAccessManager* networkManager();

    /// For tests: networkManager() returns \a networkManager, which is not owned, until that is destroyed or null
    /// is set.
    void setNetworkManagerForTest(QNetworkAccessManager* networkManager);

    /// True while networkManager() returns the network manager of a test
    bool hasNetworkManagerForTest() const { return !_testNetworkManager.isNull(); }

    /// The pauses of the tile servers that answered that they were asked too often, or that they are unavailable for a
    /// time they named, which every request for a tile (TileReply) waits for: those of the maps, of the offline tile
    /// sets, of the terrain and of the 3D view ask the same servers. Created on first use, never null, and alive as
    /// long as the engine. For the main thread, where the engine lives.
    TileServerPauses* serverPauses();

    /// For tests: serverPauses() returns \a serverPauses, which is not owned, until that is destroyed or null is set.
    void setServerPausesForTest(TileServerPauses* serverPauses);

    /// Emits tileSourceChanged(): the tile cache got tiles of the provider \a mapId from a file that the user
    /// imported, which the maps asked for in vain before or have older ones of
    void tilesImported(int mapId) { emit tileSourceChanged(mapId); }

    static MapEngine* instance();

signals:
    /// The user configured another source for the provider \a mapId, or another language for the labels of its
    /// tiles (MapProvider::cacheVariantSettings()), or imported tiles of it. The tiles of that provider which a map
    /// holds, and those it gets for a request it made before, are of the previous source.
    void tileSourceChanged(int mapId);

private:
    QNetworkAccessManager* _networkManager = nullptr;
    QPointer<QNetworkAccessManager> _testNetworkManager;
    TileServerPauses* _serverPauses = nullptr;
    QPointer<TileServerPauses> _testServerPauses;
};
