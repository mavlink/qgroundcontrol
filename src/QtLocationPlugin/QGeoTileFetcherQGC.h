#pragma once

#include <QtLocation/private/qgeotilefetcher_p.h>
#include <QtCore/QLoggingCategory>
#include <QtNetwork/QNetworkRequest>

Q_DECLARE_LOGGING_CATEGORY(QGeoTileFetcherQGCLog)

class QGeoTiledMappingManagerEngineQGC;
class QGeoTiledMapReplyQGC;
class QGeoTileSpec;
class QNetworkAccessManager;

class QGeoTileFetcherQGC : public QGeoTileFetcher
{
    Q_OBJECT

public:
    explicit QGeoTileFetcherQGC(QNetworkAccessManager* networkManager, QGeoTiledMappingManagerEngineQGC *parent = nullptr);
    ~QGeoTileFetcherQGC();

    static QNetworkRequest getNetworkRequest(int mapId, int x, int y, int zoom);
    static uint32_t concurrentDownloads(const QString& type) { Q_UNUSED(type); return 6; }

private:
    QGeoTiledMapReply* getTileImage(const QGeoTileSpec &spec) final;
    bool initialized() const final;
    bool fetchingEnabled() const final;
    void timerEvent(QTimerEvent *event) final;
    void handleReply(QGeoTiledMapReply *reply, const QGeoTileSpec &spec) final;

    QNetworkAccessManager* m_networkManager = nullptr;

#if defined Q_OS_MAC
    static constexpr const char* s_userAgent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 14.5; rv:125.0) Gecko/20100101 Firefox/125.0";
#elif defined Q_OS_WIN
    static constexpr const char* s_userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:100.0) Gecko/20100101 Firefox/112.0";
#elif defined Q_OS_ANDROID
    static constexpr const char* s_userAgent = "Mozilla/5.0 (Android 13; Tablet; rv:68.0) Gecko/68.0 Firefox/112.0";
#elif defined Q_OS_LINUX
    static constexpr const char* s_userAgent = "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/112.0";
#else
    static constexpr const char* s_userAgent = "Qt Location based application";
#endif
};
