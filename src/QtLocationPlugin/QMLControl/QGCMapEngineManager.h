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

#pragma once

#include "QGCTileSet.h"
#include "QGCMapEngineData.h"

#include <QtQmlIntegration/QtQmlIntegration>
#include <QtCore/QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(QGCMapEngineManagerLog)

class QGCCachedTileSet;
class QmlObjectListModel;

class QGCMapEngineManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    // QML_SINGLETON
    Q_MOC_INCLUDE(<QmlObjectListModel.h>)

    Q_PROPERTY(quint64              tileCount       READ    tileCount                                   NOTIFY tileCountChanged)
    Q_PROPERTY(QString              tileCountStr    READ    tileCountStr                                NOTIFY tileCountChanged)
    Q_PROPERTY(quint64              tileSize        READ    tileSize                                    NOTIFY tileSizeChanged)
    Q_PROPERTY(QString              tileSizeStr     READ    tileSizeStr                                 NOTIFY tileSizeChanged)
    Q_PROPERTY(QmlObjectListModel*  tileSets        READ    tileSets                                    NOTIFY tileSetsChanged)
    Q_PROPERTY(QStringList          mapList         READ    mapList                                     CONSTANT)
    Q_PROPERTY(QStringList          mapProviderList READ    mapProviderList                             CONSTANT)
    Q_PROPERTY(QString              errorMessage    READ    errorMessage                                NOTIFY errorMessageChanged)
    Q_PROPERTY(bool                 fetchElevation  READ    fetchElevation  WRITE   setFetchElevation   NOTIFY fetchElevationChanged)
    Q_PROPERTY(quint32              freeDiskSpace   READ    freeDiskSpace                               NOTIFY freeDiskSpaceChanged)
    Q_PROPERTY(quint32              diskSpace       READ    diskSpace                                   CONSTANT)
    Q_PROPERTY(int                  selectedCount   READ    selectedCount                               NOTIFY selectedCountChanged)
    Q_PROPERTY(int                  actionProgress  READ    actionProgress                              NOTIFY actionProgressChanged)
    Q_PROPERTY(ImportAction         importAction    READ    importAction    WRITE  setImportAction      NOTIFY importActionChanged)
    Q_PROPERTY(bool                 importReplace   READ    importReplace   WRITE  setImportReplace     NOTIFY importReplaceChanged)

public:
    QGCMapEngineManager(QObject* parent = nullptr);
    ~QGCMapEngineManager();

    enum ImportAction {
        ActionNone,
        ActionImporting,
        ActionExporting,
        ActionDone,
    };
    Q_ENUM(ImportAction)

    Q_INVOKABLE void                loadTileSets            ();
    Q_INVOKABLE void                updateForCurrentView    (double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom, const QString& mapName);
    Q_INVOKABLE void                startDownload           (const QString& name, const QString& mapType);
    Q_INVOKABLE void                deleteTileSet           (QGCCachedTileSet* tileSet);
    Q_INVOKABLE void                renameTileSet           (QGCCachedTileSet* tileSet, const QString& newName);
    Q_INVOKABLE QString             getUniqueName           () const;
    Q_INVOKABLE bool                findName                (const QString &name) const;
    Q_INVOKABLE void                selectAll               ();
    Q_INVOKABLE void                selectNone              ();
    Q_INVOKABLE bool                exportSets              (const QString &path = QString());
    Q_INVOKABLE bool                importSets              (const QString &path = QString());
    Q_INVOKABLE void                resetAction             ();

    QmlObjectListModel*             tileSets                () { return _tileSets; }
    ImportAction                    importAction            () const { return _importAction; }
    QString                         tileCountStr            () const;
    QString                         tileSizeStr             () const;
    QString                         errorMessage            () const { return _errorMessage; }
    quint64                         tileCount               () const { return _imageSet.tileCount + _elevationSet.tileCount; }
    quint64                         tileSize                () const { return _imageSet.tileSize + _elevationSet.tileSize; }
    quint64                         freeDiskSpace           () const { return _freeDiskSpace; }
    quint64                         diskSpace               () const { return _diskSpace; }
    int                             selectedCount           () const;
    int                             actionProgress          () const { return _actionProgress; }
    bool                            importReplace           () const { return _importReplace; }
    bool                            fetchElevation          () const { return _fetchElevation; }

    void                            setImportReplace        (bool replace) { if(replace != _importReplace) { _importReplace = replace; emit importReplaceChanged(); } }
    void                            setImportAction         (ImportAction action) { if(action != _importAction) { _importAction = action; emit importActionChanged(); } }
    void                            setErrorMessage         (const QString& error) { if(error != _errorMessage) { _errorMessage = error; emit errorMessageChanged(); } }
    void                            setFetchElevation       (bool fetchElevation) { if(fetchElevation != _fetchElevation) { _fetchElevation = fetchElevation; emit fetchElevationChanged(); } }

    Q_INVOKABLE static void         saveSetting             (const QString& key, const QString& value);
    Q_INVOKABLE static QString      loadSetting             (const QString& key, const QString& defaultValue);
    Q_INVOKABLE static QStringList  mapTypeList             (const QString& provider);

    static QStringList              mapList                 ();
    static QStringList              mapProviderList         ();

signals:
    void tileCountChanged();
    void tileSizeChanged();
    void tileSetsChanged();
    void maxMemCacheChanged();
    void maxDiskCacheChanged();
    void errorMessageChanged();
    void fetchElevationChanged();
    void freeDiskSpaceChanged();
    void selectedCountChanged();
    void actionProgressChanged();
    void importActionChanged();
    void importReplaceChanged();

public slots:
    void taskError(QGCMapTask::TaskType type, const QString& error);

private slots:
    void _tileSetSaved(QGCCachedTileSet* set);
    void _tileSetFetched(QGCCachedTileSet* tileSets);
    void _tileSetDeleted(quint64 setID);
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _resetCompleted();
    void _actionCompleted();
    void _actionProgressHandler(int percentage);

private:
    void _updateDiskFreeSpace();

    QmlObjectListModel* _tileSets;
    QGCTileSet _imageSet;
    QGCTileSet _elevationSet;
    ImportAction _importAction = ActionNone;
    double _topleftLat = 0.;
    double _topleftLon = 0.;
    double _bottomRightLat = 0.;
    double _bottomRightLon = 0.;
    int _minZoom = 0;
    int _maxZoom = 0;
    int _actionProgress = 0;
    quint64 _setID = UINT64_MAX;
    quint32 _freeDiskSpace = 0;
    quint32 _diskSpace = 0;
    QString _errorMessage = "";
    bool _fetchElevation = true;
    bool _importReplace = false;
};
