#pragma once

#include <QtLocation/private/qgeofiletilecache_p.h>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGeoFileTileCacheQGCLog)

class QNetworkAccessManager;
class QNetworkDiskCache;

class QGeoFileTileCacheQGC : public QGeoFileTileCache
{
	Q_OBJECT

public:
	QGeoFileTileCacheQGC(QNetworkAccessManager* networkManager, const QVariantMap &parameters, QObject *parent = nullptr);
	~QGeoFileTileCacheQGC();

private:
    static QString _getCachePath(const QVariantMap &parameters);
	static uint32_t _getMemLimit(const QVariantMap &Parameters);

    static const uint32_t _getDefaultMaxMemLimit() { return (3 * pow(1024, 2)); }
    static const uint32_t _getDefaultMaxDiskCache() { return (50 * pow(1024, 2)); }
    static const uint32_t _getDefaultExtraTexture() { return (6 * pow(1024, 2)); }
    static const uint32_t _getDefaultMinTexture() { return 0; }

    QNetworkDiskCache* m_diskCache = nullptr;
};
