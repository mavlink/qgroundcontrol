#pragma once

#include <QtCore/QHash>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QGCTileSet.h"
#include "QGCMapTasks.h"

Q_DECLARE_LOGGING_CATEGORY(QGCMapEngineManagerLog)

class QGCCachedTileSet;
class QGCCompressionJob;
class QmlObjectListModel;

class QGCMapEngineManager : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_MOC_INCLUDE("QGCCachedTileSet.h")
    Q_PROPERTY(bool                 fetchElevation  MEMBER _fetchElevation                          NOTIFY fetchElevationChanged)
    Q_PROPERTY(bool                 importReplace   MEMBER _importReplace                           NOTIFY importReplaceChanged)
    Q_PROPERTY(ImportAction         importAction    READ importAction       WRITE setImportAction   NOTIFY importActionChanged)
    Q_PROPERTY(int                  actionProgress  READ actionProgress                             NOTIFY actionProgressChanged)
    Q_PROPERTY(int                  selectedCount   READ selectedCount                              NOTIFY selectedCountChanged)
    Q_PROPERTY(QmlObjectListModel   *tileSets       READ tileSets                                   NOTIFY tileSetsChanged)
    Q_PROPERTY(QString              errorMessage    READ errorMessage                               NOTIFY errorMessageChanged)
    Q_PROPERTY(QString              tileCountStr    READ tileCountStr                               NOTIFY tileCountChanged)
    Q_PROPERTY(QString              tileSizeStr     READ tileSizeStr                                NOTIFY tileSizeChanged)
    Q_PROPERTY(QStringList          mapList         READ mapList                                    CONSTANT)
    Q_PROPERTY(QStringList          mapProviderList READ mapProviderList                            CONSTANT)
    Q_PROPERTY(QStringList          elevationProviderList   READ elevationProviderList              CONSTANT)
    Q_PROPERTY(quint64              tileCount       READ tileCount                                  NOTIFY tileCountChanged)
    Q_PROPERTY(quint64              tileSize        READ tileSize                                   NOTIFY tileSizeChanged)
    Q_PROPERTY(quint32              pendingDownloadCount READ pendingDownloadCount NOTIFY downloadMetricsChanged)
    Q_PROPERTY(quint32              activeDownloadCount  READ activeDownloadCount  NOTIFY downloadMetricsChanged)
    Q_PROPERTY(quint32              errorDownloadCount   READ errorDownloadCount   NOTIFY downloadMetricsChanged)

public:
    explicit QGCMapEngineManager(QObject *parent = nullptr);
    ~QGCMapEngineManager();
    static QGCMapEngineManager *instance();

    enum class ImportAction {
        ActionNone,
        ActionImporting,
        ActionExporting,
        ActionDone,
    };
    Q_ENUM(ImportAction)

    Q_INVOKABLE bool exportSets(const QString &path = QString());
    Q_INVOKABLE bool findName(const QString &name) const;
    Q_INVOKABLE bool importSets(const QString &path = QString());

    /// Import tile sets from an archive file (.zip, .tar.gz, etc.)
    /// If the path is an archive, it will be extracted first, then imported.
    /// @param archivePath Path to the archive file
    /// @return true if import/extraction started successfully
    Q_INVOKABLE bool importArchive(const QString &archivePath);
    Q_INVOKABLE QString getUniqueName() const;
    Q_INVOKABLE void deleteTileSet(QGCCachedTileSet *tileSet);
    Q_INVOKABLE void loadTileSets(bool forceReload = false);
    Q_INVOKABLE void renameTileSet(QGCCachedTileSet *tileSet, const QString &newName);
    Q_INVOKABLE void resetAction() { setImportAction(ImportAction::ActionNone); }
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void selectNone();
    Q_INVOKABLE void startDownload(const QString &name, const QString &mapType);
    Q_INVOKABLE void updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom, const QString &mapName);

    /// @brief Temporarily pause/resume all caching operations
    /// @param paused true to pause caching, false to resume
    /// @note Uses reference counting to support nested pause/resume calls
    /// @note Useful during database maintenance or when testing
    Q_INVOKABLE void setCachingPaused(bool paused);

    /// @brief Enable or disable caching for the default tile set
    /// @param enabled true to enable default cache, false to disable
    /// @note The default cache stores tiles from normal map browsing
    /// @note Disabling can save disk space for users who only use offline tile sets
    Q_INVOKABLE void setCachingDefaultSetEnabled(bool enabled);

    Q_INVOKABLE static QString loadSetting(const QString &key, const QString &defaultValue);
    Q_INVOKABLE static QStringList mapTypeList(const QString &provider);
    Q_INVOKABLE static void saveSetting(const QString &key, const QString &value);

    ImportAction importAction() const { return _importAction; }
    int actionProgress() const { return _actionProgress; }
    int selectedCount() const;
    QmlObjectListModel *tileSets() { return _tileSets; }
    QString errorMessage() const { return _errorMessage; }
    QString tileCountStr() const;
    QString tileSizeStr() const;
    quint64 tileCount() const { return (_imageSet.tileCount + _elevationSet.tileCount); }
    quint64 tileSize() const { return (_imageSet.tileSize + _elevationSet.tileSize); }

    /// @brief Total number of tiles waiting to be downloaded across all tile sets
    quint32 pendingDownloadCount() const { return _pendingDownloads; }

    /// @brief Total number of tiles currently being downloaded across all tile sets
    quint32 activeDownloadCount() const { return _activeDownloads; }

    /// @brief Total number of tiles that failed to download across all tile sets
    quint32 errorDownloadCount() const { return _errorDownloads; }

    void setActionProgress(int percentage) { if (percentage != _actionProgress) { _actionProgress = percentage; emit actionProgressChanged(); } }
    void setErrorMessage(const QString &error) { if (error != _errorMessage) { _errorMessage = error; emit errorMessageChanged(); } }
    void setImportAction(ImportAction action) { if (action != _importAction) { _importAction = action; emit importActionChanged(); } }

    static QStringList mapList();
    static QStringList mapProviderList();
    static QStringList elevationProviderList();

signals:
    void actionProgressChanged();
    void errorMessageChanged();
    void fetchElevationChanged();
    void freeDiskSpaceChanged();
    void importActionChanged();
    void importReplaceChanged();
    void selectedCountChanged();
    void tileCountChanged();
    void tileSetsChanged();
    void tileSizeChanged();
    void downloadMetricsChanged();

public slots:
    void taskError(QGCMapTask::TaskType type, const QString &error);

private slots:
    void _actionCompleted();
    void _actionProgressHandler(int percentage) { setActionProgress(percentage); }
    void _resetCompleted();
    void _tileSetDeleted(quint64 setID);
    void _tileSetFetched(QGCCachedTileSet *tileSets);
    void _tileSetSaved(QGCCachedTileSet *set);
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _handleExtractionProgress(qreal progress);
    void _handleExtractionFinished(bool success);
    void _downloadStatusUpdated(quint64 setID, quint32 pending, quint32 downloading, quint32 errors);

private:
    QmlObjectListModel *_tileSets = nullptr;
    QGCTileSet _imageSet;
    QGCTileSet _elevationSet;
    ImportAction _importAction = ImportAction::ActionNone;
    double _topleftLat = 0.;
    double _topleftLon = 0.;
    double _bottomRightLat = 0.;
    double _bottomRightLon = 0.;
    int _minZoom = 0;
    int _maxZoom = 0;
    int _actionProgress = 0;
    quint64 _setID = UINT64_MAX;
    QString _errorMessage;
    bool _fetchElevation = true;
    bool _importReplace = false;
    QGCCompressionJob *_extractionJob = nullptr;
    QString _extractionOutputDir;
    quint32 _pendingDownloads = 0;
    quint32 _activeDownloads = 0;
    quint32 _errorDownloads = 0;
    struct DownloadStatsCache {
        quint32 pending = 0;
        quint32 downloading = 0;
        quint32 errors = 0;
    };
    QHash<quint64, DownloadStatsCache> _pendingDownloadStats;
    bool _cachePausedForReset = false;
    int _cacheDisableRefCount = 0;
    bool _cacheEnabledState = true;
    bool _defaultCacheEnabled = true;
    bool _defaultCacheEnabledState = true;

    void _applyPendingStats(QGCCachedTileSet *set);
    void _recomputeDownloadTotals();

    template<typename Func>
    void _forEachTileSet(Func&& func);

    template<typename Func>
    void _forEachTileSet(Func&& func) const;

    void _updateCacheEnabledState();

    static constexpr const char *kQmlOfflineMapKeyName = "QGCOfflineMap";
};
