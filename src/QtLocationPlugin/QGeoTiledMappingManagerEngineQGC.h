#pragma once

#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGeoTiledMappingManagerEngineQGCLog)

class QNetworkAccessManager;

class QGeoTiledMappingManagerEngineQGC : public QGeoTiledMappingManagerEngine
{
    Q_OBJECT

public:
    QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString);
    ~QGeoTiledMappingManagerEngineQGC();

    QGeoMap *createMap() final;
    QNetworkAccessManager* networkManager() const { return m_networkManager; }

private:
    void _setCache(const QVariantMap &parameters);
    void _createSupportedMapTypesList();

    QNetworkAccessManager* m_networkManager = nullptr;
};
