#pragma once

#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

class QNetworkAccessManager;

class QGeoTiledMappingManagerEngineQGC : public QGeoTiledMappingManagerEngine
{
    Q_OBJECT

public:
    /// \a networkManager fetches the tiles instead of the network manager of the map engine, which the maps share
    /// (test injection seam): it is not owned and must outlive this object
    QGeoTiledMappingManagerEngineQGC(QGeoServiceProvider::Error* error, QString* errorString,
                                     QNetworkAccessManager* networkManager = nullptr, QObject* parent = nullptr);
    ~QGeoTiledMappingManagerEngineQGC();

    QGeoMap* createMap() final;

private:
    static constexpr int kTileVersion = 1;
};
