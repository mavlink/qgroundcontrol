#pragma once

#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

class QNetworkAccessManager;

class QGeoTiledMappingManagerEngineQGC : public QGeoTiledMappingManagerEngine
{
    Q_OBJECT

public:
    QGeoTiledMappingManagerEngineQGC(QGeoServiceProvider::Error* error, QString* errorString,
                                     QNetworkAccessManager* networkManager = nullptr, QObject* parent = nullptr);
    ~QGeoTiledMappingManagerEngineQGC();

    QGeoMap* createMap() final;

private:
    static constexpr int kTileVersion = 1;
};
