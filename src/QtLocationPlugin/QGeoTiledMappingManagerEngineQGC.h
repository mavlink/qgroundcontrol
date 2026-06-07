#pragma once

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeotiledmap_p.h>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

class QNetworkAccessManager;
class QGeoTiledMapQGC;

class QGeoTiledMappingManagerEngineQGC : public QGeoTiledMappingManagerEngine
{
    Q_OBJECT

public:
    QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString, QNetworkAccessManager *networkManager = nullptr, QObject *parent = nullptr);
    ~QGeoTiledMappingManagerEngineQGC();

    QGeoMap* createMap() final;
    QNetworkAccessManager *networkManager() const { return m_networkManager; }

private:
    void _updatePrefetchStyles();

    QNetworkAccessManager *m_networkManager = nullptr;
    QGeoTiledMap::PrefetchStyle m_prefetchStyle = QGeoTiledMap::PrefetchTwoNeighbourLayers;
    QList<QPointer<QGeoTiledMapQGC>> m_activeMaps;

    static constexpr int kTileVersion = 1;
};
