/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Cache
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#pragma once

#include "QGCMapEngineData.h"
#include "QGCTileCacheWorker.h"
#include "QGCTileSet.h"

#include <QtCore/QString>

class UrlFactory;

//-----------------------------------------------------------------------------
class QGCMapEngine : public QObject
{
    Q_OBJECT
public:
    QGCMapEngine                ();
    ~QGCMapEngine               ();

    static QGCMapEngine* instance();

    void                        init                ();
    void                        addTask             (QGCMapTask *task);
    void                        cacheTile           (const QString& type, int x, int y, int z, const QByteArray& image, const QString& format, qulonglong set = UINT64_MAX);
    void                        cacheTile           (const QString& type, const QString& hash, const QByteArray& image, const QString& format, qulonglong set = UINT64_MAX);
    QGCFetchTileTask*           createFetchTileTask (const QString& type, int x, int y, int z);
    QStringList                 getMapNameList      ();
    const QString               userAgent           () { return _userAgent; }
    void                        setUserAgent        (const QString& ua) { _userAgent = ua; }
    QString                     tileHashToType      (const QString& tileHash);
    QString                     getTileHash         (const QString& type, int x, int y, int z);
    quint32                     getMaxDiskCache     ();
    quint32                     getMaxMemCache      ();
    void                        setMaxMemCache      (quint32 size);
    const QString               getCachePath        () { return _cachePath; }
    const QString               getCacheFilename    () { return _cacheFile; }
    void                        testInternet        ();
    bool                        wasCacheReset       () const{ return _cacheWasReset; }

    UrlFactory*                 urlFactory          () { return _urlFactory; }

    //-- Tile Math
    static QGCTileSet           getTileCount        (int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat, const QString& mapType);
    static QString              getTypeFromName     (const QString& name);
    static QString              bigSizeToString     (quint64 size);
    static QString              storageFreeSizeToString(quint64 size_MB);
    static QString              numberToString      (quint64 number);
    static int                  concurrentDownloads (const QString& type);

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
    UrlFactory*             _urlFactory;
    QString                 _userAgent;
    bool                    _prunning;
    bool                    _cacheWasReset;
};

extern QGCMapEngine*    getQGCMapEngine();
