#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QPointer>
#include <QtCore/QVector>
#include <QtLocation/QGeoServiceProvider>
#include <QtLocation/private/qgeotiledmappingmanagerengine_p.h>

Q_DECLARE_LOGGING_CATEGORY(QGeoTiledMappingManagerEngineQGCLog)

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
    QVector<QPointer<QGeoTiledMapQGC>> m_activeMaps;

    static constexpr int kTileVersion = 1;
    static constexpr int kTileSize = 256;
};
