/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Map Tile Set
 *
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#ifndef QGC_MAP_TILE_SET_H
#define QGC_MAP_TILE_SET_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QDateTime>
#include <QImage>

#include "QGCLoggingCategory.h"
#include "QGCMapEngineData.h"
#include "QGCMapUrlEngine.h"

Q_DECLARE_LOGGING_CATEGORY(QGCCachedTileSetLog)

class QGCTile;
class QGCMapEngineManager;

//-----------------------------------------------------------------------------
class QGCCachedTileSet : public QObject
{
    Q_OBJECT
public:
    QGCCachedTileSet    (const QString& name, const QString& description);
    ~QGCCachedTileSet   ();

    Q_PROPERTY(QString      name            READ    name            CONSTANT)
    Q_PROPERTY(QString      description     READ    description     CONSTANT)
    Q_PROPERTY(QString      mapTypeStr      READ    mapTypeStr      CONSTANT)
    Q_PROPERTY(double       topleftLon      READ    topleftLon      CONSTANT)
    Q_PROPERTY(double       topleftLat      READ    topleftLat      CONSTANT)
    Q_PROPERTY(double       bottomRightLon  READ    bottomRightLon  CONSTANT)
    Q_PROPERTY(double       bottomRightLat  READ    bottomRightLat  CONSTANT)
    Q_PROPERTY(int          minZoom         READ    minZoom         CONSTANT)
    Q_PROPERTY(int          maxZoom         READ    maxZoom         CONSTANT)
    Q_PROPERTY(quint32      numTiles        READ    numTiles        NOTIFY numTilesChanged)
    Q_PROPERTY(QString      numTilesStr     READ    numTilesStr     NOTIFY numTilesChanged)
    Q_PROPERTY(quint64      tilesSize       READ    tilesSize       NOTIFY tilesSizeChanged)
    Q_PROPERTY(QString      tilesSizeStr    READ    tilesSizeStr    NOTIFY tilesSizeChanged)
    Q_PROPERTY(quint32      savedTiles      READ    savedTiles      NOTIFY savedTilesChanged)
    Q_PROPERTY(QString      savedTilesStr   READ    savedTilesStr   NOTIFY savedTilesChanged)
    Q_PROPERTY(quint64      savedSize       READ    savedSize       NOTIFY savedSizeChanged)
    Q_PROPERTY(QString      savedSizeStr    READ    savedSizeStr    NOTIFY savedSizeChanged)
    Q_PROPERTY(QString      downloadStatus  READ    downloadStatus  NOTIFY savedSizeChanged)
    Q_PROPERTY(QDateTime    creationDate    READ    creationDate    CONSTANT)
    Q_PROPERTY(bool         complete        READ    complete        NOTIFY completeChanged)
    Q_PROPERTY(bool         defaultSet      READ    defaultSet      CONSTANT)
    Q_PROPERTY(quint64      setID           READ    setID           CONSTANT)
    Q_PROPERTY(bool         deleting        READ    deleting        NOTIFY deletingChanged)
    Q_PROPERTY(bool         downloading     READ    downloading     NOTIFY downloadingChanged)
    Q_PROPERTY(quint32      errorCount      READ    errorCount      NOTIFY errorCountChanged)
    Q_PROPERTY(QString      errorCountStr   READ    errorCountStr   NOTIFY errorCountChanged)
    Q_PROPERTY(QImage       thumbNail       READ    thumbNail       CONSTANT)

    Q_INVOKABLE void createDownloadTask ();
    Q_INVOKABLE void resumeDownloadTask ();
    Q_INVOKABLE void cancelDownloadTask ();

    void        setManager              (QGCMapEngineManager* mgr);

    QString     name                    () { return _name; }
    QString     description             () { return _description; }
    QString     mapTypeStr              () { return _mapTypeStr; }
    double      topleftLat              () { return _topleftLat; }
    double      topleftLon              () { return _topleftLon; }
    double      bottomRightLat          () { return _bottomRightLat; }
    double      bottomRightLon          () { return _bottomRightLon; }
    quint32     numTiles                () { return (quint32)_numTiles; }
    QString     numTilesStr             ();
    quint64     tilesSize               () { return (quint64)_tilesSize; }
    QString     tilesSizeStr            ();
    quint32     savedTiles              () { return (quint32)_savedTiles; }
    QString     savedTilesStr           ();
    quint64     savedSize               () { return (quint64)_savedSize; }
    QString     savedSizeStr            ();
    QString     downloadStatus          ();
    int         minZoom                 () { return _minZoom; }
    int         maxZoom                 () { return _maxZoom; }
    QDateTime   creationDate            () { return _creationDate; }
    quint64     id                      () { return _id; }
    UrlFactory::MapType type            () { return _type; }
    bool        complete                () { return _defaultSet || (_numTiles == _savedTiles); }
    bool        defaultSet              () { return _defaultSet; }
    quint64     setID                   () { return _id; }
    bool        deleting                () { return _deleting; }
    bool        downloading             () { return _downloading; }
    quint32     errorCount              () { return _errorCount; }
    QString     errorCountStr           ();
    QImage      thumbNail               () { return _thumbNail; }

    void        setName                 (QString name)              { _name = name; }
    void        setDescription          (QString desc)              { _description = desc; }
    void        setMapTypeStr           (QString typeStr)           { _mapTypeStr = typeStr; }
    void        setTopleftLat           (double lat)                { _topleftLat = lat; }
    void        setTopleftLon           (double lon)                { _topleftLon = lon; }
    void        setBottomRightLat       (double lat)                { _bottomRightLat = lat; }
    void        setBottomRightLon       (double lon)                { _bottomRightLon = lon; }
    void        setNumTiles             (quint32 num)               { _numTiles = num; }
    void        setTilesSize            (quint64 size)              { _tilesSize = size; }
    void        setSavedTiles           (quint32 num)               { _savedTiles = num; emit savedTilesChanged(); }
    void        setSavedSize            (quint64 size)              { _savedSize = size; emit savedSizeChanged();  }
    void        setMinZoom              (int zoom)                  { _minZoom = zoom; }
    void        setMaxZoom              (int zoom)                  { _maxZoom = zoom; }
    void        setCreationDate         (QDateTime date)            { _creationDate = date; }
    void        setId                   (quint64 id)                { _id = id; }
    void        setType                 (UrlFactory::MapType type)  { _type = type; }
    void        setDefaultSet           (bool def)                  { _defaultSet = def; }
    void        setDeleting             (bool del)                  { _deleting = del; emit deletingChanged(); }
    void        setDownloading          (bool down)                 { _downloading = down; }
    void        setThumbNail            (const QImage& thumb)       { _thumbNail = thumb; }

signals:
    void        deletingChanged         ();
    void        downloadingChanged      ();
    void        numTilesChanged         ();
    void        tilesSizeChanged        ();
    void        savedTilesChanged       ();
    void        savedSizeChanged        ();
    void        completeChanged         ();
    void        errorCountChanged       ();

private slots:
    void _tileListFetched               (QList<QGCTile*> tiles);
    void _networkReplyFinished          ();
    void _networkReplyError             (QNetworkReply::NetworkError error);

private:
    void        _prepareDownload        ();

private:
    QString     _name;
    QString     _description;
    QString     _mapTypeStr;
    double      _topleftLat;
    double      _topleftLon;
    double      _bottomRightLat;
    double      _bottomRightLon;
    quint32     _numTiles;
    quint64     _tilesSize;
    quint32     _savedTiles;
    quint64     _savedSize;
    int         _minZoom;
    int         _maxZoom;
    bool        _defaultSet;
    bool        _deleting;
    bool        _downloading;
    QDateTime   _creationDate;
    quint64     _id;
    UrlFactory::MapType _type;
    QNetworkAccessManager*  _networkManager;
    QHash<QString, QNetworkReply*> _replies;
    quint32     _errorCount;
    //-- Tile download
    QList<QGCTile *> _tilesToDownload;
    bool        _noMoreTiles;
    bool        _batchRequested;
    QGCMapEngineManager* _manager;
    QImage      _thumbNail;
};

#endif // QGC_MAP_TILE_SET_H

