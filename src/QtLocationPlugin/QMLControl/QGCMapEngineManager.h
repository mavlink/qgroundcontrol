/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Gus Grubba <gus@auterion.com>

#ifndef OfflineMapsManager_H
#define OfflineMapsManager_H

#include "QmlObjectListModel.h"
#include "QGCToolbox.h"
#include "QGCLoggingCategory.h"
#include "QGCMapEngine.h"
#include "QGCMapTileSet.h"

Q_DECLARE_LOGGING_CATEGORY(QGCMapEngineManagerLog)

class QGCMapEngineManager : public QGCTool
{
    Q_OBJECT
public:
    QGCMapEngineManager(QGCApplication* app, QGCToolbox* toolbox);
    ~QGCMapEngineManager();

    enum ImportAction {
        ActionNone,
        ActionImporting,
        ActionExporting,
        ActionDone,
    };
    Q_ENUM(ImportAction)

    Q_PROPERTY(quint64              tileCount       READ    tileCount       NOTIFY tileCountChanged)
    Q_PROPERTY(QString              tileCountStr    READ    tileCountStr    NOTIFY tileCountChanged)
    Q_PROPERTY(quint64              tileSize        READ    tileSize        NOTIFY tileSizeChanged)
    Q_PROPERTY(QString              tileSizeStr     READ    tileSizeStr     NOTIFY tileSizeChanged)
    Q_PROPERTY(QmlObjectListModel*  tileSets        READ    tileSets        NOTIFY tileSetsChanged)
    Q_PROPERTY(QStringList          mapList         READ    mapList         CONSTANT)
    Q_PROPERTY(QStringList          mapProviderList READ    mapProviderList CONSTANT)
    Q_PROPERTY(quint32              maxMemCache     READ    maxMemCache     WRITE   setMaxMemCache  NOTIFY  maxMemCacheChanged)
    Q_PROPERTY(quint32              maxDiskCache    READ    maxDiskCache    WRITE   setMaxDiskCache NOTIFY  maxDiskCacheChanged)
    Q_PROPERTY(QString              errorMessage    READ    errorMessage    NOTIFY  errorMessageChanged)
    Q_PROPERTY(bool                 fetchElevation  READ    fetchElevation  WRITE   setFetchElevation   NOTIFY  fetchElevationChanged)
    //-- Disk Space in MB
    Q_PROPERTY(quint32              freeDiskSpace   READ    freeDiskSpace   NOTIFY  freeDiskSpaceChanged)
    Q_PROPERTY(quint32              diskSpace       READ    diskSpace       CONSTANT)
    //-- Tile set export
    Q_PROPERTY(int                  selectedCount   READ    selectedCount   NOTIFY selectedCountChanged)
    Q_PROPERTY(int                  actionProgress  READ    actionProgress  NOTIFY actionProgressChanged)
    Q_PROPERTY(ImportAction         importAction    READ    importAction    WRITE  setImportAction   NOTIFY importActionChanged)

    Q_PROPERTY(bool                 importReplace   READ    importReplace   WRITE   setImportReplace   NOTIFY importReplaceChanged)

    Q_INVOKABLE void                loadTileSets            ();
    Q_INVOKABLE void                updateForCurrentView    (double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom, const QString& mapName);
    Q_INVOKABLE void                startDownload           (const QString& name, const QString& mapType);
    Q_INVOKABLE void                saveSetting             (const QString& key,  const QString& value);
    Q_INVOKABLE QString             loadSetting             (const QString& key,  const QString& defaultValue);
    Q_INVOKABLE void                deleteTileSet           (QGCCachedTileSet* tileSet);
    Q_INVOKABLE void                renameTileSet           (QGCCachedTileSet* tileSet, QString newName);
    Q_INVOKABLE QString             getUniqueName           ();
    Q_INVOKABLE bool                findName                (const QString& name);
    Q_INVOKABLE void                selectAll               ();
    Q_INVOKABLE void                selectNone              ();
    Q_INVOKABLE bool                exportSets              (QString path = QString());
    Q_INVOKABLE bool                importSets              (QString path = QString());
    Q_INVOKABLE void                resetAction             ();

    quint64                         tileCount               () const{ return _imageSet.tileCount + _elevationSet.tileCount; }
    QString                         tileCountStr            () const;
    quint64                         tileSize                () const{ return _imageSet.tileSize + _elevationSet.tileSize; }
    QString                         tileSizeStr             () const;
    QStringList                     mapList                 ();
    QStringList                     mapProviderList         ();
    Q_INVOKABLE QStringList         mapTypeList             (QString provider);
    QmlObjectListModel*             tileSets                () { return &_tileSets; }
    quint32                         maxMemCache             ();
    quint32                         maxDiskCache            ();
    QString                         errorMessage            () { return _errorMessage; }
    bool                            fetchElevation          () const{ return _fetchElevation; }
    quint64                         freeDiskSpace           () const{ return _freeDiskSpace; }
    quint64                         diskSpace               () const{ return _diskSpace; }
    int                             selectedCount           ();
    int                             actionProgress          () const{ return _actionProgress; }
    ImportAction                    importAction            () { return _importAction; }
    bool                            importReplace           () const{ return _importReplace; }

    void                            setMaxMemCache          (quint32 size);
    void                            setMaxDiskCache         (quint32 size);
    void                            setImportReplace        (bool replace) { _importReplace = replace; emit importReplaceChanged(); }
    void                            setImportAction         (ImportAction action)  {_importAction = action; emit importActionChanged(); }
    void                            setErrorMessage         (const QString& error) { _errorMessage = error; emit errorMessageChanged(); }
    void                            setFetchElevation       (bool fetchElevation) { _fetchElevation = fetchElevation; emit fetchElevationChanged(); }

    // Override from QGCTool
    void setToolbox(QGCToolbox *toolbox);

signals:
    void tileCountChanged       ();
    void tileSizeChanged        ();
    void tileSetsChanged        ();
    void maxMemCacheChanged     ();
    void maxDiskCacheChanged    ();
    void errorMessageChanged    ();
    void fetchElevationChanged  ();
    void freeDiskSpaceChanged   ();
    void selectedCountChanged   ();
    void actionProgressChanged  ();
    void importActionChanged    ();
    void importReplaceChanged   ();

public slots:
    void taskError              (QGCMapTask::TaskType type, QString error);

private slots:
    void _tileSetSaved          (QGCCachedTileSet* set);
    void _tileSetFetched        (QGCCachedTileSet* tileSets);
    void _tileSetDeleted        (quint64 setID);
    void _updateTotals          (quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _resetCompleted        ();
    void _actionCompleted       ();
    void _actionProgressHandler (int percentage);

private:
    void _updateDiskFreeSpace   ();

private:
    QGCTileSet  _imageSet;
    QGCTileSet  _elevationSet;
    double      _topleftLat;
    double      _topleftLon;
    double      _bottomRightLat;
    double      _bottomRightLon;
    int         _minZoom;
    int         _maxZoom;
    quint64     _setID;
    quint32     _freeDiskSpace;
    quint32     _diskSpace;
    QmlObjectListModel _tileSets;
    QString     _errorMessage;
    bool        _fetchElevation;
    int         _actionProgress;
    ImportAction _importAction;
    bool        _importReplace;
};

#endif
