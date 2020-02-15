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
 *   @brief Map Tile Cache Data
 *
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#ifndef QGC_MAP_ENGINE_DATA_H
#define QGC_MAP_ENGINE_DATA_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QDateTime>

#include "QGCMapUrlEngine.h"

class QGCCachedTileSet;

//-----------------------------------------------------------------------------
class QGCTile
{
public:
    QGCTile()
        : _x(0)
        , _y(0)
        , _z(0)
        , _set(UINT64_MAX)
        , _type("Invalid")
    {
    }

    enum TyleState {
        StatePending = 0,
        StateDownloading,
        StateError,
        StateComplete
    };

    int                 x           () const { return _x; }
    int                 y           () const { return _y; }
    int                 z           () const { return _z; }
    qulonglong          set         () const { return _set;  }
    const QString       hash        () const { return _hash; }
    QString type        () const { return _type; }

    void                setX        (int x) { _x = x; }
    void                setY        (int y) { _y = y; }
    void                setZ        (int z) { _z = z; }
    void                setTileSet  (qulonglong set) { _set = set;  }
    void                setHash     (const QString& hash) { _hash = hash; }
    void                setType     (QString type) { _type = type; }

private:
    int         _x;
    int         _y;
    int         _z;
    qulonglong  _set;
    QString     _hash;
    QString _type;
};

//-----------------------------------------------------------------------------
class QGCCacheTile : public QObject
{
    Q_OBJECT
public:
    QGCCacheTile    (const QString hash, const QByteArray img, const QString format, QString type, qulonglong set = UINT64_MAX)
        : _set(set)
        , _hash(hash)
        , _img(img)
        , _format(format)
        , _type(type)
    {
    }
    QGCCacheTile    (const QString hash, qulonglong set)
        : _set(set)
        , _hash(hash)
    {
    }
    qulonglong          set     () { return _set;   }
    QString             hash    () { return _hash;  }
    QByteArray          img     () { return _img;   }
    QString             format  () { return _format;}
    QString type    () { return _type; }
private:
    qulonglong  _set;
    QString     _hash;
    QByteArray  _img;
    QString     _format;
    QString _type;
};

//-----------------------------------------------------------------------------
class QGCMapTask : public QObject
{
    Q_OBJECT
public:

    enum TaskType {
        taskInit,
        taskTestInternet,
        taskCacheTile,
        taskFetchTile,
        taskFetchTileSets,
        taskCreateTileSet,
        taskGetTileDownloadList,
        taskUpdateTileDownloadState,
        taskDeleteTileSet,
        taskRenameTileSet,
        taskPruneCache,
        taskReset,
        taskExport,
        taskImport
    };

    QGCMapTask(TaskType type)
        : _type(type)
    {}
    virtual ~QGCMapTask()
    {}

    virtual TaskType    type            () { return _type; }

    void setError(QString errorString = QString())
    {
        emit error(_type, errorString);
    }

signals:
    void error          (QGCMapTask::TaskType type, QString errorString);

private:
    TaskType    _type;
};

//-----------------------------------------------------------------------------
class QGCTestInternetTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCTestInternetTask()
        : QGCMapTask(QGCMapTask::taskTestInternet)
    {}
};

//-----------------------------------------------------------------------------
class QGCFetchTileSetTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCFetchTileSetTask()
        : QGCMapTask(QGCMapTask::taskFetchTileSets)
    {}

    void setTileSetFetched(QGCCachedTileSet* tileSet)
    {
        emit tileSetFetched(tileSet);
    }

signals:
    void            tileSetFetched  (QGCCachedTileSet* tileSet);
};

//-----------------------------------------------------------------------------
class QGCCreateTileSetTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCCreateTileSetTask(QGCCachedTileSet* tileSet)
        : QGCMapTask(QGCMapTask::taskCreateTileSet)
        , _tileSet(tileSet)
        , _saved(false)
    {}

    ~QGCCreateTileSetTask();

    QGCCachedTileSet*   tileSet () { return _tileSet; }

    void setTileSetSaved()
    {
        //-- Flag as saved. Signalee wll maintain it.
        _saved = true;
        emit tileSetSaved(_tileSet);
    }

signals:
    void tileSetSaved   (QGCCachedTileSet* tileSet);

private:
    QGCCachedTileSet* _tileSet;
    bool              _saved;
};

//-----------------------------------------------------------------------------
class QGCFetchTileTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCFetchTileTask(const QString hash)
        : QGCMapTask(QGCMapTask::taskFetchTile)
        , _hash(hash)
    {}

    ~QGCFetchTileTask()
    {
    }

    void setTileFetched(QGCCacheTile* tile)
    {
        emit tileFetched(tile);
    }

    QString         hash() { return _hash; }

signals:
    void            tileFetched     (QGCCacheTile* tile);

private:
    QString         _hash;
};

//-----------------------------------------------------------------------------
class QGCSaveTileTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCSaveTileTask(QGCCacheTile* tile)
        : QGCMapTask(QGCMapTask::taskCacheTile)
        , _tile(tile)
    {}

    ~QGCSaveTileTask()
    {
        delete _tile;
        _tile = nullptr;
    }

    QGCCacheTile*   tile() { return _tile; }

private:
    QGCCacheTile*   _tile;
};

//-----------------------------------------------------------------------------
class QGCGetTileDownloadListTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCGetTileDownloadListTask(qulonglong setID, int count)
        : QGCMapTask(QGCMapTask::taskGetTileDownloadList)
        , _setID(setID)
        , _count(count)
    {}

    qulonglong  setID() { return _setID; }
    int         count() { return _count; }

    void setTileListFetched(QList<QGCTile*> tiles)
    {
        emit tileListFetched(tiles);
    }

signals:
    void            tileListFetched  (QList<QGCTile*> tiles);

private:
    qulonglong  _setID;
    int         _count;
};

//-----------------------------------------------------------------------------
class QGCUpdateTileDownloadStateTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCUpdateTileDownloadStateTask(qulonglong setID, QGCTile::TyleState state, const QString& hash)
        : QGCMapTask(QGCMapTask::taskUpdateTileDownloadState)
        , _setID(setID)
        , _state(state)
        , _hash(hash)
    {}

    QString             hash    () { return _hash; }
    qulonglong          setID   () { return _setID; }
    QGCTile::TyleState  state   () { return _state; }

private:
    qulonglong          _setID;
    QGCTile::TyleState  _state;
    QString             _hash;
};

//-----------------------------------------------------------------------------
class QGCDeleteTileSetTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCDeleteTileSetTask(qulonglong setID)
        : QGCMapTask(QGCMapTask::taskDeleteTileSet)
        , _setID(setID)
    {}

    qulonglong  setID() { return _setID; }

    void setTileSetDeleted()
    {
        emit tileSetDeleted(_setID);
    }

signals:
    void tileSetDeleted(qulonglong setID);

private:
    qulonglong  _setID;
};

//-----------------------------------------------------------------------------
class QGCRenameTileSetTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCRenameTileSetTask(qulonglong setID, QString newName)
        : QGCMapTask(QGCMapTask::taskRenameTileSet)
        , _setID(setID)
        , _newName(newName)
    {}

    qulonglong  setID   () { return _setID; }
    QString     newName () { return _newName; }

private:
    qulonglong  _setID;
    QString     _newName;
};

//-----------------------------------------------------------------------------
class QGCPruneCacheTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCPruneCacheTask(quint64 amount)
        : QGCMapTask(QGCMapTask::taskPruneCache)
        , _amount(amount)
    {}

    quint64  amount() { return _amount; }

    void setPruned()
    {
        emit pruned();
    }

signals:
    void pruned();

private:
    quint64  _amount;
};

//-----------------------------------------------------------------------------
class QGCResetTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCResetTask()
        : QGCMapTask(QGCMapTask::taskReset)
    {}

    void setResetCompleted()
    {
        emit resetCompleted();
    }

signals:
    void resetCompleted();
};

//-----------------------------------------------------------------------------
class QGCExportTileTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCExportTileTask(QVector<QGCCachedTileSet*> sets, QString path)
        : QGCMapTask(QGCMapTask::taskExport)
        , _sets(sets)
        , _path(path)
    {}

    ~QGCExportTileTask()
    {
    }

    QVector<QGCCachedTileSet*> sets() { return _sets; }
    QString                    path() { return _path; }

    void setExportCompleted()
    {
        emit actionCompleted();
    }

    void setProgress(int percentage)
    {
        emit actionProgress(percentage);
    }

private:
    QVector<QGCCachedTileSet*>  _sets;
    QString                     _path;

signals:
    void actionCompleted        ();
    void actionProgress         (int percentage);

};

//-----------------------------------------------------------------------------
class QGCImportTileTask : public QGCMapTask
{
    Q_OBJECT
public:
    QGCImportTileTask(QString path, bool replace)
        : QGCMapTask(QGCMapTask::taskImport)
        , _path(path)
        , _replace(replace)
    {}

    ~QGCImportTileTask()
    {
    }

    QString                    path     () { return _path; }
    bool                       replace  () { return _replace; }

    void setImportCompleted()
    {
        emit actionCompleted();
    }

    void setProgress(int percentage)
    {
        emit actionProgress(percentage);
    }

private:
    QString                     _path;
    bool                        _replace;

signals:
    void actionCompleted        ();
    void actionProgress         (int percentage);

};

#endif // QGC_MAP_ENGINE_DATA_H
