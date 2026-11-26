#pragma once

#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QGCMapTaskBase.h"
#include "QGCTileSet.h"

class CompressionJob;
class QGCCachedTileSet;

namespace Compression {
struct Outcome;
}
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
    Q_PROPERTY(QStringList importArchivePatterns READ importArchivePatterns CONSTANT)
    Q_PROPERTY(quint64              tileCount       READ tileCount                                  NOTIFY tileCountChanged)
    Q_PROPERTY(quint64              tileSize        READ tileSize                                   NOTIFY tileSizeChanged)

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
    /// Import tile sets from a file that exportSets() wrote, or from an archive that holds one
    /// @return true if the import started
    Q_INVOKABLE bool importSets(const QString &path = QString());

    /// Import tile sets from an archive file (.zip, and .7z, .tar.gz, ... in a build that extracts them), which
    /// importSets() calls for one. An archive of a format that the build does not extract fails the import
    /// The tile set file in the archive is unpacked into a temporary directory and imported from there. It is
    /// told by its name: the extension exportSets() gives its files, .db or .sqlite. An archive that holds none, or
    /// more than one, fails the import. The temporary directory is removed when the import ends, also when it fails.
    /// @param archivePath Path to the archive file
    /// @return true if the extraction started
    Q_INVOKABLE bool importArchive(const QString &archivePath);
    Q_INVOKABLE QString getUniqueName() const;
    Q_INVOKABLE void deleteTileSet(QGCCachedTileSet *tileSet);
    Q_INVOKABLE void loadTileSets();
    Q_INVOKABLE void renameTileSet(QGCCachedTileSet *tileSet, const QString &newName);
    Q_INVOKABLE void resetAction() { setImportAction(ImportAction::ActionNone); }
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void selectNone();
    Q_INVOKABLE void startDownload(const QString &name, const QString &mapType);
    Q_INVOKABLE void updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom, const QString &mapName);

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

    void setActionProgress(int percentage) { if (percentage != _actionProgress) { _actionProgress = percentage; emit actionProgressChanged(); } }
    void setErrorMessage(const QString &error) { if (error != _errorMessage) { _errorMessage = error; emit errorMessageChanged(); } }
    void setImportAction(ImportAction action) { if (action != _importAction) { _importAction = action; emit importActionChanged(); } }

    static QStringList mapList();
    static QStringList mapProviderList();
    static QStringList elevationProviderList();
    /// Name patterns of the archives that importArchive() takes in this build ("*.zip", ...), for a file dialog
    static QStringList importArchivePatterns();

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

public slots:
    void taskError(QGCMapTask::TaskType type, const QString &error);

private slots:
    void _actionCompleted();
    void _actionProgressHandler(int percentage) { setActionProgress(percentage); }
    void _resetCompleted() { loadTileSets(); }
    void _tileSetDeleted(quint64 setID);
    void _tileSetFetched(QGCCachedTileSet *tileSets);
    void _tileSetSaved(QGCCachedTileSet *set);
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _handleExtractionProgress(qreal progress);
    /// Remove the directory an archive was unpacked into for its import
    void _removeExtractionOutput();

private:
    void _handleExtractionFinished(const Compression::Outcome& outcome);

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
    QPointer<CompressionJob> _extractionJob;
    QString _extractionOutputDir;

    static constexpr const char *kQmlOfflineMapKeyName = "QGCOfflineMap";
};
