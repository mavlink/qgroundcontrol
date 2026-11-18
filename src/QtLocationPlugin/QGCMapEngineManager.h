#pragma once

#include <optional>

#include <QtQmlIntegration/QtQmlIntegration>

#include "QGCTileSetPlan.h"

class QGCCachedTileSet;
class QGCCompressionJob;
class QGCMapTask;
class QmlObjectListModel;

/// The offline tile sets of the tile cache, for the UI: their list, the download of a new set for a view, and
/// their import and export
class QGCMapEngineManager : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_MOC_INCLUDE("QGCCachedTileSet.h")
    Q_PROPERTY(bool fetchElevation MEMBER _fetchElevation NOTIFY fetchElevationChanged)
    Q_PROPERTY(bool importReplace MEMBER _importReplace NOTIFY importReplaceChanged)
    Q_PROPERTY(ImportAction importAction READ importAction WRITE setImportAction NOTIFY importActionChanged)
    Q_PROPERTY(int actionProgress READ actionProgress NOTIFY actionProgressChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
    Q_PROPERTY(QmlObjectListModel* tileSets READ tileSets NOTIFY tileSetsChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString tileCountStr READ tileCountStr NOTIFY tileCountChanged)
    Q_PROPERTY(QString tileSizeStr READ tileSizeStr NOTIFY tileSizeChanged)
    Q_PROPERTY(quint64 tileCount READ tileCount NOTIFY tileCountChanged)
    Q_PROPERTY(quint64 tileSize READ tileSize NOTIFY tileSizeChanged)

public:
    explicit QGCMapEngineManager(QObject* parent = nullptr);
    ~QGCMapEngineManager();
    static QGCMapEngineManager* instance();

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

    /// Import tile sets from an archive file (.zip, .tar.gz, etc.)
    /// If the path is an archive, it will be extracted first, then imported.
    /// @param archivePath Path to the archive file
    /// @return true if import/extraction started successfully
    Q_INVOKABLE bool importArchive(const QString& archivePath);
    Q_INVOKABLE QString getUniqueName() const;
    Q_INVOKABLE void deleteTileSet(QGCCachedTileSet* tileSet);
    Q_INVOKABLE void loadTileSets();
    Q_INVOKABLE void renameTileSet(QGCCachedTileSet* tileSet, const QString& newName);
    Q_INVOKABLE void selectAll();
    /// Creates the sets that updateForCurrentView() counted the tiles of, and starts their downloads
    Q_INVOKABLE void startDownload(const QString& name, const QString& mapType);
    /// Counts the tiles of the sets that startDownload() creates for a view: those that QGCTileSetPlan::forView()
    /// plans for it
    Q_INVOKABLE void updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom,
                                          const QString& mapName);

    ImportAction importAction() const { return _importAction; }

    int actionProgress() const { return _actionProgress; }

    int selectedCount() const;

    QmlObjectListModel* tileSets() { return _tileSets; }

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

    /// Shows the user the error of \a task, if it fails
    void reportErrorOf(QGCMapTask* task);

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
    void _tileSetFetched(QGCCachedTileSet* tileSets);
    void _tileSetSaved(QGCCachedTileSet* set);
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _handleExtractionProgress(qreal progress);
    void _handleExtractionFinished(bool success);

private:
    void _taskFailed(const QString& taskName, const QString& error);
    /// Queues the import or the export that importAction() tells of
    bool _startAction(QGCMapTask* task);
    /// The end of an import or of an export, whose \a error is empty when it succeeded
    void _actionFinished(const QString& taskName, const QString& error);
    /// Marks the set \a setID, or every set without one, as being deleted or not
    void _setDeleting(std::optional<quint64> setID, bool deleting);
    /// Stores a set of the tiles \a planned for the region of the current view and starts its download
    void _createTileSet(const QString& name, const QString& mapType, const QGCTileSetPlan::Set& planned);

    QmlObjectListModel* _tileSets = nullptr;
    /// The sets of the view that updateForCurrentView() last had
    QGCTileSetPlan _plan;
    ImportAction _importAction = ImportAction::ActionNone;
    int _actionProgress = 0;
    QString _errorMessage;
    bool _fetchElevation = true;
    bool _importReplace = false;
    QGCCompressionJob* _extractionJob = nullptr;
    QString _extractionOutputDir;
};
