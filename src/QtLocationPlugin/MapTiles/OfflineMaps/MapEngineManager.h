#pragma once

#include <optional>

#include <QtQmlIntegration/QtQmlIntegration>

#include "TileSetPlan.h"

class CachedTileSet;
class CacheTask;
class QmlObjectListModel;

/// The offline tile sets of the tile cache, for the UI: their list, the download of a new set for a view, and
/// their import and export. The sets that it lists are its own.
class MapEngineManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_MOC_INCLUDE("CachedTileSet.h")
    Q_PROPERTY(bool fetchElevation MEMBER _fetchElevation NOTIFY fetchElevationChanged FINAL)
    Q_PROPERTY(bool importReplace MEMBER _importReplace NOTIFY importReplaceChanged FINAL)
    Q_PROPERTY(ImportAction importAction READ importAction WRITE setImportAction NOTIFY importActionChanged FINAL)
    Q_PROPERTY(int actionProgress READ actionProgress NOTIFY actionProgressChanged FINAL)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged FINAL)
    Q_PROPERTY(QmlObjectListModel* tileSets READ tileSets NOTIFY tileSetsChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(QString tileCountStr READ tileCountStr NOTIFY tileCountChanged FINAL)
    Q_PROPERTY(QString tileSizeStr READ tileSizeStr NOTIFY tileSizeChanged FINAL)
    Q_PROPERTY(quint64 tileCount READ tileCount NOTIFY tileCountChanged FINAL)
    Q_PROPERTY(quint64 tileSize READ tileSize NOTIFY tileSizeChanged FINAL)

public:
    explicit MapEngineManager(QObject* parent = nullptr);
    ~MapEngineManager();
    static MapEngineManager* instance();

    enum class ImportAction
    {
        ActionNone,
        ActionImporting,
        ActionExporting,
        ActionDone,
    };
    Q_ENUM(ImportAction)

    Q_INVOKABLE bool exportSets(const QString& path = QString());
    Q_INVOKABLE bool findName(const QString& name) const;
    Q_INVOKABLE bool importSets(const QString& path = QString());

    Q_INVOKABLE QString getUniqueName() const;
    /// Deletes \a tileSet with the tiles that no other set has. For the default set: everything that the cache has,
    /// the offline sets as well.
    Q_INVOKABLE void deleteTileSet(CachedTileSet* tileSet);
    /// Deletes the tiles that the maps cached as they were looked at. The offline sets keep theirs.
    Q_INVOKABLE void clearCachedTiles();
    /// Lists the sets that the cache has now. A set that is listed already stays the object it is, with its download.
    Q_INVOKABLE void loadTileSets();
    Q_INVOKABLE void renameTileSet(CachedTileSet* tileSet, const QString& newName);
    Q_INVOKABLE void selectAll();
    /// Creates the sets that updateForCurrentView() counted the tiles of, and starts their downloads
    Q_INVOKABLE void startDownload(const QString& name, const QString& mapType);
    /// Counts the tiles of the sets that startDownload() creates for a view: those that TileSetPlan::forView()
    /// plans for it
    Q_INVOKABLE void updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom,
                                          const QString& mapName);

    ImportAction importAction() const { return _importAction; }

    int actionProgress() const { return _actionProgress; }

    int selectedCount() const;

    QmlObjectListModel* tileSets() const { return _tileSets; }

    QString errorMessage() const { return _errorMessage; }

    QString tileCountStr() const;
    QString tileSizeStr() const;

    quint64 tileCount() const { return _plan.tileCount(); }

    quint64 tileSize() const { return _plan.tileSize(); }

    void setActionProgress(int percentage)
    {
        if (percentage != _actionProgress) {
            _actionProgress = percentage;
            emit actionProgressChanged();
        }
    }

    void setErrorMessage(const QString& error)
    {
        if (error != _errorMessage) {
            _errorMessage = error;
            emit errorMessageChanged();
        }
    }

    void setImportAction(ImportAction action)
    {
        if (action != _importAction) {
            _importAction = action;
            emit importActionChanged();
        }
    }

signals:
    void actionProgressChanged();
    void errorMessageChanged();
    void fetchElevationChanged();
    void importActionChanged();
    void importReplaceChanged();
    void selectedCountChanged();
    void tileCountChanged();
    void tileSetsChanged();
    void tileSizeChanged();

private slots:
    void _tileSetDeleted(quint64 setID);
    void _tileSetsFetched(const QList<CachedTileSet*>& fetched);
    void _tileSetSaved(CachedTileSet* set);
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);

private:
    /// Shows the user the error of \a task, if it fails
    void _reportErrorOf(CacheTask* task);
    void _taskFailed(const QString& taskName, const QString& error);
    /// Takes \a tileSet as its own, and listens to it
    void _adoptTileSet(CachedTileSet* tileSet);
    /// Deletes the listed sets, whose downloads end with them
    void _removeTileSets();
    /// Queues the import or the export that importAction() tells of
    bool _startAction(CacheTask* task);
    /// The end of an import or of an export, whose \a error is empty when it succeeded
    void _actionFinished(const QString& taskName, const QString& error);
    /// Marks the set \a setID, or every set without one, as being deleted or not
    void _setDeleting(std::optional<quint64> setID, bool deleting);
    /// Stores a set of the tiles \a planned for the region of the current view and starts its download
    void _createTileSet(const QString& name, const QString& mapType, const TileSetPlan::Set& planned);

    QmlObjectListModel* _tileSets = nullptr;
    /// The sets of the view that updateForCurrentView() last had
    TileSetPlan _plan;
    ImportAction _importAction = ImportAction::ActionNone;
    int _actionProgress = 0;
    QString _errorMessage;
    bool _fetchElevation = true;
    bool _importReplace = false;
};
