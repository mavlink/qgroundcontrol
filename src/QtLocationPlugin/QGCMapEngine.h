/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Cache
 *
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#ifndef QGC_MAP_ENGINE_H
#define QGC_MAP_ENGINE_H

#include <QString>

#include "QGCMapUrlEngine.h"
#include "QGCMapEngineData.h"
#include "QGCTileCacheWorker.h"

//-----------------------------------------------------------------------------
class QGCTileSet
{
public:
    QGCTileSet()
    {
        clear();
    }
    QGCTileSet& operator += (QGCTileSet& other)
    {
        tileX0      += other.tileX0;
        tileX1      += other.tileX1;
        tileY0      += other.tileY0;
        tileY1      += other.tileY1;
        tileCount   += other.tileCount;
        tileSize    += other.tileSize;
        return *this;
    }
    void clear()
    {
        tileX0      = 0;
        tileX1      = 0;
        tileY0      = 0;
        tileY1      = 0;
        tileCount   = 0;
        tileSize    = 0;
    }

    int         tileX0;
    int         tileX1;
    int         tileY0;
    int         tileY1;
    quint64     tileCount;
    quint64     tileSize;
};

//-----------------------------------------------------------------------------
class QGCMapEngine : public QObject
{
    Q_OBJECT
public:
    QGCMapEngine                ();
    ~QGCMapEngine               ();

    void                        init                ();
    void                        addTask             (QGCMapTask *task);
    void                        cacheTile           (UrlFactory::MapType type, int x, int y, int z, const QByteArray& image, const QString& format, qulonglong set = UINT64_MAX);
    void                        cacheTile           (UrlFactory::MapType type, const QString& hash, const QByteArray& image, const QString& format, qulonglong set = UINT64_MAX);
    QGCFetchTileTask*           createFetchTileTask (UrlFactory::MapType type, int x, int y, int z);
    QStringList                 getMapNameList      ();
    const QString               userAgent           () { return _userAgent; }
    void                        setUserAgent        (const QString& ua) { _userAgent = ua; }
    UrlFactory::MapType         hashToType          (const QString& hash);
    QString                     getMapBoxToken      ();
    void                        setMapBoxToken      (const QString& token);
    quint32                     getMaxDiskCache     ();
    void                        setMaxDiskCache     (quint32 size);
    quint32                     getMaxMemCache      ();
    void                        setMaxMemCache      (quint32 size);
    const QString               getCachePath        () { return _cachePath; }
    const QString               getCacheFilename    () { return _cacheFile; }
    bool                        wasCacheReset       () { return _cacheWasReset; }

    UrlFactory*                 urlFactory          () { return _urlFactory; }

    //-- Tile Math
    static QGCTileSet           getTileCount        (int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat, UrlFactory::MapType mapType);
    static int                  long2tileX          (double lon, int z);
    static int                  lat2tileY           (double lat, int z);
    static QString              getTileHash         (UrlFactory::MapType type, int x, int y, int z);
    static UrlFactory::MapType  getTypeFromName     (const QString &name);
    static QString              bigSizeToString     (quint64 size);
    static QString              numberToString      (quint64 number);
    static int                  concurrentDownloads (UrlFactory::MapType type);

private slots:
    void _updateTotals          (quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _pruned                ();

signals:
    void updateTotals           (quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private:
    void _wipeOldCaches         ();
    void _checkWipeDirectory    (const QString& dirPath);
    bool _wipeDirectory         (const QString& dirPath);

private:
    QGCCacheWorker          _worker;
    QString                 _cachePath;
    QString                 _cacheFile;
    QString                 _mapBoxToken;
    UrlFactory*             _urlFactory;
    QString                 _userAgent;
    quint32                 _maxDiskCache;
    quint32                 _maxMemCache;
    bool                    _prunning;
    bool                    _cacheWasReset;
};

extern QGCMapEngine*    getQGCMapEngine();
extern void             destroyMapEngine();

#endif // QGC_MAP_ENGINE_H
