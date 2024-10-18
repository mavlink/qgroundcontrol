/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Map Tile Set
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtNetwork/QNetworkReply>

Q_DECLARE_LOGGING_CATEGORY(QGCCachedTileSetLog)

class QGCTile;
class QGCMapEngineManager;
class QNetworkAccessManager;

class QGCCachedTileSet : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QGCTile.h")

    Q_PROPERTY(QString      name                READ    name                NOTIFY nameChanged)
    Q_PROPERTY(QString      mapTypeStr          READ    mapTypeStr          CONSTANT)
    Q_PROPERTY(double       topleftLon          READ    topleftLon          CONSTANT)
    Q_PROPERTY(double       topleftLat          READ    topleftLat          CONSTANT)
    Q_PROPERTY(double       bottomRightLon      READ    bottomRightLon      CONSTANT)
    Q_PROPERTY(double       bottomRightLat      READ    bottomRightLat      CONSTANT)
    Q_PROPERTY(int          minZoom             READ    minZoom             CONSTANT)
    Q_PROPERTY(int          maxZoom             READ    maxZoom             CONSTANT)
    Q_PROPERTY(quint32      totalTileCount      READ    totalTileCount      NOTIFY totalTileCountChanged)
    Q_PROPERTY(QString      totalTileCountStr   READ    totalTileCountStr   NOTIFY totalTileCountChanged)
    Q_PROPERTY(quint64      totalTilesSize      READ    totalTilesSize      NOTIFY totalTilesSizeChanged)
    Q_PROPERTY(QString      totalTilesSizeStr   READ    totalTilesSizeStr   NOTIFY totalTilesSizeChanged)
    Q_PROPERTY(quint32      uniqueTileCount     READ    uniqueTileCount     NOTIFY uniqueTileCountChanged)
    Q_PROPERTY(QString      uniqueTileCountStr  READ    uniqueTileCountStr  NOTIFY uniqueTileCountChanged)
    Q_PROPERTY(quint64      uniqueTileSize      READ    uniqueTileSize      NOTIFY uniqueTileSizeChanged)
    Q_PROPERTY(QString      uniqueTileSizeStr   READ    uniqueTileSizeStr   NOTIFY uniqueTileSizeChanged)
    Q_PROPERTY(quint32      savedTileCount      READ    savedTileCount      NOTIFY savedTileCountChanged)
    Q_PROPERTY(QString      savedTileCountStr   READ    savedTileCountStr   NOTIFY savedTileCountChanged)
    Q_PROPERTY(quint64      savedTileSize       READ    savedTileSize       NOTIFY savedTileSizeChanged)
    Q_PROPERTY(QString      savedTileSizeStr    READ    savedTileSizeStr    NOTIFY savedTileSizeChanged)
    Q_PROPERTY(QString      downloadStatus      READ    downloadStatus      NOTIFY savedTileSizeChanged)
    Q_PROPERTY(QDateTime    creationDate        READ    creationDate        CONSTANT)
    Q_PROPERTY(bool         complete            READ    complete            NOTIFY completeChanged)
    Q_PROPERTY(bool         defaultSet          READ    defaultSet          CONSTANT)
    Q_PROPERTY(quint64      id                  READ    id                  CONSTANT)
    Q_PROPERTY(bool         deleting            READ    deleting            NOTIFY deletingChanged)
    Q_PROPERTY(bool         downloading         READ    downloading         NOTIFY downloadingChanged)
    Q_PROPERTY(quint32      errorCount          READ    errorCount          NOTIFY errorCountChanged)
    Q_PROPERTY(QString      errorCountStr       READ    errorCountStr       NOTIFY errorCountChanged)
    Q_PROPERTY(bool         selected            READ    selected            WRITE  setSelected  NOTIFY selectedChanged)

public:
    explicit QGCCachedTileSet(const QString &name, QObject *parent = nullptr);
    ~QGCCachedTileSet();

    Q_INVOKABLE void createDownloadTask();
    Q_INVOKABLE void resumeDownloadTask();
    Q_INVOKABLE void cancelDownloadTask();

    const QString &name() const { return _name; }
    const QString &mapTypeStr() const { return _mapTypeStr; }

    double topleftLat() const { return _topleftLat; }
    double topleftLon() const { return _topleftLon; }
    double bottomRightLat() const { return _bottomRightLat; }
    double bottomRightLon() const { return _bottomRightLon; }

    quint32 totalTileCount() const { return _totalTileCount; }
    QString totalTileCountStr() const;
    quint64 totalTilesSize() const { return _totalTileSize; }
    QString totalTilesSizeStr() const;
    quint32 uniqueTileCount() const { return _uniqueTileCount; }
    QString uniqueTileCountStr() const;
    quint64 uniqueTileSize() const { return _uniqueTileSize; }
    QString uniqueTileSizeStr() const;
    quint32 savedTileCount() const { return _savedTileCount; }
    QString savedTileCountStr() const;
    quint64 savedTileSize() const { return _savedTileSize; }
    QString savedTileSizeStr() const;

    QString downloadStatus() const;
    int minZoom() const { return _minZoom; }
    int maxZoom() const { return _maxZoom; }
    const QDateTime &creationDate() const { return _creationDate; }
    quint64 id() const { return _id; }
    const QString &type() const { return _type; }
    bool complete() const { return (_defaultSet || (_totalTileCount <= _savedTileCount)); }
    bool defaultSet() const { return _defaultSet; }
    bool deleting() const { return _deleting; }
    bool downloading() const { return _downloading; }
    quint32 errorCount() const { return _errorCount; }
    QString errorCountStr() const;
    bool selected() const { return _selected; }

    void setManager(QGCMapEngineManager *mgr) { _manager = mgr; }
    void setSelected(bool sel);
    void setName(const QString &name) { if (name != _name) { _name = name; emit nameChanged(); } }

    void setMapTypeStr(const QString &typeStr) { _mapTypeStr = typeStr; }
    void setTopleftLat(double lat) { _topleftLat = lat; }
    void setTopleftLon(double lon) { _topleftLon = lon; }
    void setBottomRightLat(double lat) { _bottomRightLat = lat; }
    void setBottomRightLon(double lon) { _bottomRightLon = lon; }

    void setUniqueTileCount(quint32 num) { if (num != _uniqueTileCount) { _uniqueTileCount = num; emit uniqueTileCountChanged(); } }
    void setTotalTileCount(quint32 num) { if (num != _totalTileCount) { _totalTileCount = num; emit totalTileCountChanged(); } }
    void setSavedTileCount(quint32 num) { if (num != _savedTileCount) { _savedTileCount = num; emit savedTileCountChanged(); } }
    void setUniqueTileSize(quint64 size) { if (size != _uniqueTileSize) { _uniqueTileSize = size; emit uniqueTileSizeChanged(); } }
    void setTotalTileSize(quint64 size) { if (size != _totalTileSize) { _totalTileSize = size; emit totalTilesSizeChanged(); } }
    void setSavedTileSize(quint64 size) { if (size != _savedTileSize) { _savedTileSize = size; emit savedTileSizeChanged(); }  }

    void setMinZoom(int zoom) { _minZoom = zoom; }
    void setMaxZoom(int zoom) { _maxZoom = zoom; }
    void setCreationDate(const QDateTime &date) { _creationDate = date; }
    void setId(quint64 id) { _id = id; }
    void setType(const QString &type) { _type = type; }
    void setDefaultSet(bool def) { _defaultSet = def; }
    void setDeleting(bool del) { if (del != _deleting) { _deleting = del; emit deletingChanged(); } }
    void setDownloading(bool down) { if (down != _downloading) { _downloading = down; emit downloadingChanged(); } }
    void setErrorCount(quint32 count) { if (count != _errorCount) { _errorCount = count; emit errorCountChanged(); } }

signals:
    void deletingChanged();
    void downloadingChanged();
    void totalTileCountChanged();
    void uniqueTileCountChanged();
    void uniqueTileSizeChanged();
    void totalTilesSizeChanged();
    void savedTileCountChanged();
    void savedTileSizeChanged();
    void completeChanged();
    void errorCountChanged();
    void selectedChanged();
    void nameChanged();

private slots:
    void _tileListFetched(const QQueue<QGCTile*> &tiles);
    void _networkReplyFinished();
    void _networkReplyError(QNetworkReply::NetworkError error);

private:
    void _prepareDownload();
    void _doneWithDownload();

    QString _name;
    QString _mapTypeStr;
    QString _type = QStringLiteral("Invalid");
    quint64 _id = 0;
    double _topleftLat = 0.;
    double _topleftLon = 0.;
    double _bottomRightLat = 0.;
    double _bottomRightLon = 0.;
    quint32 _totalTileCount = 0;
    quint64 _totalTileSize = 0;
    quint32 _uniqueTileCount = 0;
    quint64 _uniqueTileSize = 0;
    quint32 _savedTileCount = 0;
    quint64 _savedTileSize = 0;
    quint32 _errorCount = 0;
    int _minZoom = 3;
    int _maxZoom = 3;
    bool _defaultSet = false;
    bool _deleting = false;
    bool _downloading = false;
    bool _noMoreTiles = false;
    bool _batchRequested = false;
    bool _selected = false;
    bool _cancelPending = false;
    QDateTime _creationDate;

    QHash<QString, QNetworkReply*> _replies;
    QQueue<QGCTile*> _tilesToDownload;
    QGCMapEngineManager *_manager = nullptr;
    QNetworkAccessManager *_networkManager = nullptr;

    static constexpr uint32_t kTileBatchSize = 256;
};
