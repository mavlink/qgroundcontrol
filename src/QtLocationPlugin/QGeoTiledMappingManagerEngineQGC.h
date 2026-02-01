#pragma once

#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>


class QNetworkAccessManager;

class QGeoTiledMappingManagerEngineQGC : public QGeoTiledMappingManagerEngine
{
    Q_OBJECT

public:
    QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString, QNetworkAccessManager *networkManager = nullptr, QObject *parent = nullptr);
    ~QGeoTiledMappingManagerEngineQGC();

    QGeoMap* createMap() final;
    QNetworkAccessManager *networkManager() const { return m_networkManager; }

private:
    QNetworkAccessManager *m_networkManager = nullptr;

    static constexpr int kTileVersion = 1;
};
