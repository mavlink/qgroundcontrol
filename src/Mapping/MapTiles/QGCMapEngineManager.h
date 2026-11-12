#pragma once

#include <QtCore/QPointer>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QGCMapTask.h"
#include "QGCTileSet.h"

class EsriAttribution;
class QGCCachedTileSet;
class QGCCompressionJob;
class QmlObjectListModel;
class QNetworkAccessManager;

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
    Q_PROPERTY(QStringList mapList READ mapList CONSTANT)
    /// Name of the map the user chose, as mapList has it. The settings select the map by a provider and a type that
    /// are set one after the other and name no map in between: the map chosen before stays until they name one.
    /// Saved settings that name no map are repaired when the manager is created: they get the first type of their
    /// provider, or the default map when no map has that provider.
    Q_PROPERTY(QString chosenMap READ chosenMap NOTIFY chosenMapChanged FINAL)
    Q_PROPERTY(QStringList mapProviderList READ mapProviderList CONSTANT)
    Q_PROPERTY(QStringList elevationProviderList READ elevationProviderList CONSTANT)
    Q_PROPERTY(QString terrainAttribution READ terrainAttribution CONSTANT FINAL)
    /// Counts the changes of what areaAttribution() answers, as when the list of a map's credits arrives: a binding
    /// that reads it beside its call of areaAttribution() is evaluated again
    Q_PROPERTY(int areaAttributionRevision READ areaAttributionRevision NOTIFY areaAttributionChanged FINAL)
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
    /// Counts the tiles of the sets that startDownload() creates for a view. Imagery \a mapName gets a set of the
    /// zooms \a minZoom to \a maxZoom and, when elevation is fetched, the sets of elevation tiles beside it. An
    /// elevation provider \a mapName gets one set, of the zooms that the provider has for those of the view.
    Q_INVOKABLE void updateForCurrentView(double lon0, double lat0, double lon1, double lat1, int minZoom, int maxZoom,
                                          const QString& mapName);

    /// The credit of the provider \a mapType, named as mapList() names it. Empty for a name that is not in that
    /// list, and for a provider without a credit.
    Q_INVOKABLE static QString mapAttribution(const QString& mapType);
    /// The credits that the provider \a mapType, named as mapList() names it, asks for beside mapAttribution() where
    /// a view shows its tiles: the sources of what is on screen, which depend on the place and the zoom level. The
    /// best source there is first, and they are joined with ", ". The view has its centre at \a latitude and
    /// \a longitude, in degrees, and is \a width by \a height pixels of a map at the zoom level \a zoomLevel, whose
    /// tiles are 256 pixels wide. The credits are those of the tile zoom level at or below \a zoomLevel, and of the
    /// area that such a view has from straight above, north up.
    /// Empty for a provider that publishes no list of such credits, and until its list has arrived:
    /// areaAttributionRevision changes when it has.
    Q_INVOKABLE QString areaAttribution(const QString& mapType, double latitude, double longitude, double zoomLevel,
                                        double width, double height);
    /// True for a map \a mapType, named as mapList() names it, that a map engine can show: one that is not an
    /// elevation provider. False for a name that is not in that list.
    Q_INVOKABLE static bool isImageryMap(const QString& mapType);
    /// True for a map \a mapType, named as mapList() names it, of satellite or aerial pictures, also one with
    /// labels on them (a hybrid map): views draw what they put on such a map in light colors. The map style of
    /// the provider tells, as the name of a map need not say it. False for a name that is not in that list.
    Q_INVOKABLE static bool isSatelliteMap(const QString& mapType);
    Q_INVOKABLE static QStringList mapTypeList(const QString& provider);

    ImportAction importAction() const { return _importAction; }

    int actionProgress() const { return _actionProgress; }

    int selectedCount() const;

    QmlObjectListModel* tileSets() { return _tileSets; }

    QString errorMessage() const { return _errorMessage; }

    QString chosenMap() const { return _chosenMap; }

    int areaAttributionRevision() const { return _areaAttributionRevision; }

    /// Test seam: the lists that areaAttribution() answers from are fetched through \a networkManager, and those
    /// fetched before are forgotten. Null restores the network manager of the map engine. Unit tests reach no
    /// server: under them, a manager that has no network manager from this function fetches no list.
    void setAreaAttributionNetworkManager(QNetworkAccessManager* networkManager);

    QString tileCountStr() const;
    QString tileSizeStr() const;

    quint64 tileCount() const { return (_imageSet.tileCount + _elevationSet.tileCount + _terrainSet.tileCount); }

    quint64 tileSize() const { return (_imageSet.tileSize + _elevationSet.tileSize + _terrainSet.tileSize); }

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

    static QStringList mapList();
    /// The name, as mapList() has it, of the map of \a provider that has \a type: the two parts by which the
    /// settings select a map. A provider whose only map has no type names that map itself, and \a type is ignored.
    static QString mapName(const QString& provider, const QString& type);
    static QStringList mapProviderList();
    static QStringList elevationProviderList();
    /// The credit of the terrain that the GeoMap engine shapes its surface with
    static QString terrainAttribution();

signals:
    void actionProgressChanged();
    void areaAttributionChanged();
    void chosenMapChanged();
    void errorMessageChanged();
    void fetchElevationChanged();
    void importActionChanged();
    void importReplaceChanged();
    void selectedCountChanged();
    void tileCountChanged();
    void tileSetsChanged();
    void tileSizeChanged();

public slots:
    void taskError(QGCMapTask::TaskType type, const QString& error);

private slots:
    void _actionCompleted();

    void _actionProgressHandler(int percentage) { setActionProgress(percentage); }

    void _resetCompleted() { loadTileSets(); }

    void _tileSetDeleted(quint64 setID);
    void _tileSetFetched(QGCCachedTileSet* tileSets);
    void _tileSetSaved(QGCCachedTileSet* set);
    void _updateTotals(quint32 totaltiles, quint64 totalsize, quint32 defaulttiles, quint64 defaultsize);
    void _updateChosenMap();
    void _handleExtractionProgress(qreal progress);
    void _handleExtractionFinished(bool success);

private:
    /// The maps that a map engine can show: mapList() without the elevation providers
    static QStringList _imageryMapList();
    /// The name that the provider and the type in the settings make: no map has it while one of the two is being set
    static QString _mapOfSettings();
    /// Makes settings that name no map name one: they get the first type of their provider or, when no map has that
    /// provider, the default map. They stay as they are when the default settings name no map either. Earlier
    /// versions saved the provider "Street" with the type "Street Map" for OpenStreetMap, which names no map.
    static void _repairMapOfSettings();
    /// Stores a set of \a tiles for the region of the current view and starts its download
    void _createTileSet(const QString& name, const QString& mapType, int minZoom, int maxZoom, const QGCTileSet& tiles);
    /// The reader of the lists that areaAttribution() answers from, created when it is first needed. Null under unit
    /// tests, unless a test set a network manager for it.
    EsriAttribution* _areaAttributionLists();
    void _notifyAreaAttributionChanged();

    QmlObjectListModel* _tileSets = nullptr;
    QGCTileSet _imageSet;
    QGCTileSet _elevationSet;
    /// Terrarium tiles GeoMap renders its terrain from: empty unless elevation is fetched and GeoMap is enabled
    QGCTileSet _terrainSet;
    ImportAction _importAction = ImportAction::ActionNone;
    double _topleftLat = 0.;
    double _topleftLon = 0.;
    double _bottomRightLat = 0.;
    double _bottomRightLon = 0.;
    int _minZoom = 0;
    int _maxZoom = 0;
    int _terrainMinZoom = 0;
    int _terrainMaxZoom = 0;
    int _actionProgress = 0;
    QString _errorMessage;
    QString _chosenMap;
    bool _fetchElevation = true;
    bool _importReplace = false;
    QGCCompressionJob* _extractionJob = nullptr;
    QString _extractionOutputDir;
    EsriAttribution* _esriAttribution = nullptr;
    QPointer<QNetworkAccessManager> _areaAttributionNetworkManager;
    int _areaAttributionRevision = 0;

    /// The zoom in the keys of the tiles of an elevation provider that has no zoom levels: the terrain queries ask
    /// the tile cache for it
    static constexpr int kElevationZoom = 1;
};
