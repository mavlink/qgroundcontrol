#pragma once

#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"
#include "QmlObjectListModel.h"
#include "FactMetaData.h"

class ParameterManager;

class ParameterTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ParameterTableModel(QObject* parent = nullptr);
    ~ParameterTableModel() override;

    typedef QVector<QVariant> ColumnData;

    enum {
        FactRole = Qt::UserRole + 1
    };

    enum {
        FavColumn = 0,
        NameColumn,
        ValueColumn,
        DescriptionColumn,
    };

    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)

    void append      (Fact* fact);
    void insert      (int row, Fact* fact);
    void clear       ();
    void beginReset  (); ///< Supports nesting - only outermost call has effect
    void endReset    (); ///< Supports nesting - only outermost call has effect
    Fact*            factAt(int row) const;

    // Overrides from QAbstractTableModel
    int         rowCount    (const QModelIndex & parent = QModelIndex()) const override;
    int         columnCount (const QModelIndex &parent = QModelIndex()) const override;
    QVariant    data        (const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QVariant    headerData  (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames(void) const override;

    signals:
    void rowCountChanged(int count);

private:
    bool _isResetting() const { return _resetNestingCount > 0; }

    int                 _tableViewColCount = 4;
    QList<ColumnData>   _tableData;
    uint                _resetNestingCount = 0;
};

class ParameterEditorGroup : public QObject
{
    Q_OBJECT

public:
    ParameterEditorGroup(QObject* parent);

    Q_PROPERTY(QString              name    MEMBER name     CONSTANT)
    Q_PROPERTY(QAbstractTableModel* facts   READ getFacts   CONSTANT)

    QAbstractTableModel*  getFacts(void) { return &facts; }

    int                 componentId;
    QString             name;
    ParameterTableModel facts;
};

class ParameterEditorCategory : public QObject
{
    Q_OBJECT

public:
    ParameterEditorCategory(QObject* parent) : QObject(parent) { }

    Q_PROPERTY(QString              name    MEMBER name     CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  groups  READ getGroups  CONSTANT)

    QmlObjectListModel*  getGroups(void) { return &groups; }

    QString             name;
    QmlObjectListModel  groups;
    QMap<QString, ParameterEditorGroup*> mapGroupName2Group;
};

class ParameterEditorDiff : public QObject
{
    Q_OBJECT

public:
    ParameterEditorDiff(QObject* parent) : QObject(parent) { }

    Q_PROPERTY(int      componentId         MEMBER componentId      CONSTANT)
    Q_PROPERTY(QString  name                MEMBER name             CONSTANT)
    Q_PROPERTY(QString  fileValue           MEMBER fileValue        CONSTANT)
    Q_PROPERTY(QString  vehicleValue        MEMBER vehicleValue     CONSTANT)
    Q_PROPERTY(bool     noVehicleValue      MEMBER noVehicleValue   CONSTANT)
    Q_PROPERTY(bool     cannotSend          MEMBER cannotSend       CONSTANT)
    Q_PROPERTY(QString  units               MEMBER units            CONSTANT)
    Q_PROPERTY(bool     load                MEMBER load             NOTIFY loadChanged)

    int                         componentId;
    QString                     name;
    FactMetaData::ValueType_t   valueType;
    QString                     fileValue;
    QVariant                    fileValueVar;
    QString                     vehicleValue;
    bool                        noVehicleValue  = false;
    bool                        cannotSend      = false;    ///< Param not on vehicle and file has no type info (MP format) - shown but never sent
    QString                     units;
    bool                        load            = true;

signals:
    void loadChanged(bool load);
};

class ParameterEditorController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString              searchText              MEMBER _searchText                                          NOTIFY searchTextChanged)
    Q_PROPERTY(QmlObjectListModel*  categories              READ categories                                             CONSTANT)
    Q_PROPERTY(QObject*             currentCategory         READ currentCategory            WRITE setCurrentCategory    NOTIFY currentCategoryChanged)
    Q_PROPERTY(QObject*             currentGroup            READ currentGroup               WRITE setCurrentGroup       NOTIFY currentGroupChanged)
    Q_PROPERTY(QAbstractTableModel* parameters              MEMBER _parameters                                          NOTIFY parametersChanged)
    Q_PROPERTY(bool                 showModifiedOnly        MEMBER _showModifiedOnly                                    NOTIFY showModifiedOnlyChanged)
    Q_PROPERTY(bool                 showFavoritesOnly       MEMBER _showFavoritesOnly                                   NOTIFY showFavoritesOnlyChanged)
    Q_PROPERTY(bool                 hideReadOnly            MEMBER _hideReadOnly                                        NOTIFY hideReadOnlyChanged)
    Q_PROPERTY(QStringList          favoriteParameterNames  READ favoriteParameterNames                                 NOTIFY favoritesChanged)

    // These property are related to the diff associated with a load from file
    Q_PROPERTY(bool                 diffOtherVehicle        READ diffOtherVehicle                                       NOTIFY diffOtherVehicleChanged)
    Q_PROPERTY(bool                 diffMultipleComponents  READ diffMultipleComponents                                 NOTIFY diffMultipleComponentsChanged)
    Q_PROPERTY(QmlObjectListModel*  diffList                READ diffList                                               CONSTANT)
    Q_PROPERTY(int                  diffParsedCount         READ diffParsedCount                                        NOTIFY diffParsedCountChanged)
    Q_PROPERTY(int                  diffUnchangedCount      READ diffUnchangedCount                                     NOTIFY diffUnchangedCountChanged)
    Q_PROPERTY(int                  diffReadOnlyCount       READ diffReadOnlyCount                                      NOTIFY diffReadOnlyCountChanged)
    Q_PROPERTY(int                  diffNoVehicleCount      READ diffNoVehicleCount                                     NOTIFY diffNoVehicleCountChanged)
    Q_PROPERTY(int                  diffSendableCount       READ diffSendableCount                                      NOTIFY diffSendableCountChanged)
    Q_PROPERTY(int                  diffSelectedCount       READ diffSelectedCount                                      NOTIFY diffSelectedCountChanged)
    Q_PROPERTY(QStringList          diffMissingParams       READ diffMissingParams                                      NOTIFY diffMissingParamsChanged)

public:
    explicit ParameterEditorController(QObject *parent = nullptr);
    ~ParameterEditorController();

    Q_INVOKABLE void saveToFile                     (const QString& filename);
    Q_INVOKABLE bool buildDiffFromFile              (const QString& filename);
    Q_INVOKABLE void clearDiff                      (void);
    Q_INVOKABLE void sendDiff                       (void);
    Q_INVOKABLE void refresh                        (void);
    Q_INVOKABLE void resetAllToDefaults             (void);
    Q_INVOKABLE void resetAllToVehicleConfiguration (void);
    Q_INVOKABLE void toggleFavorite                 (const QString& paramName);
    Q_INVOKABLE bool isFavorite                     (const QString& paramName) const;
    Q_INVOKABLE void clearAllFavorites              (void);

    QObject*            currentCategory         (void) { return _currentCategory; }
    QObject*            currentGroup            (void) { return _currentGroup; }
    QmlObjectListModel* categories              (void) { return &_categories; }
    QmlObjectListModel* diffList                (void) { return &_diffList; }
    bool                diffOtherVehicle        (void) const { return _diffOtherVehicle; }
    bool                diffMultipleComponents  (void) const { return _diffMultipleComponents; }
    int                 diffParsedCount         (void) const { return _diffParsedCount; }
    int                 diffUnchangedCount      (void) const { return _diffUnchangedCount; }
    int                 diffReadOnlyCount       (void) const { return _diffReadOnlyCount; }
    int                 diffNoVehicleCount      (void) const { return _diffNoVehicleCount; }
    int                 diffSendableCount       (void) const { return _diffSendableCount; }
    int                 diffSelectedCount       (void) const { return _diffSelectedCount; }
    QStringList         diffMissingParams       (void) const { return _diffMissingParams; }
    QStringList         favoriteParameterNames  (void) const;
    void                setCurrentCategory  (QObject* currentCategory);
    void                setCurrentGroup     (QObject* currentGroup);

signals:
    void searchTextChanged              (QString searchText);
    void currentCategoryChanged         (void);
    void currentGroupChanged            (void);
    void showModifiedOnlyChanged        (void);
    void showFavoritesOnlyChanged       (void);
    void hideReadOnlyChanged            (void);
    void favoritesChanged               (void);
    void diffOtherVehicleChanged        (bool diffOtherVehicle);
    void diffMultipleComponentsChanged  (bool diffMultipleComponents);
    void diffParsedCountChanged         (int diffParsedCount);
    void diffUnchangedCountChanged      (int diffUnchangedCount);
    void diffReadOnlyCountChanged       (int diffReadOnlyCount);
    void diffNoVehicleCountChanged      (int diffNoVehicleCount);
    void diffSendableCountChanged       (int diffSendableCount);
    void diffSelectedCountChanged       (int diffSelectedCount);
    void diffMissingParamsChanged       (const QStringList& diffMissingParams);
    void parametersChanged              (void);

private slots:
    void _currentCategoryChanged(void);
    void _currentGroupChanged   (void);
    void _searchTextChanged     (void);
    void _hideReadOnlyChanged   (void);
    void _buildLists            (void);
    void _buildListsForComponent(int compId);
    void _factAdded             (int compId, Fact* fact);

private:
    bool _shouldShow(Fact *fact) const;
    void _performSearch();
    void _loadFavorites();
    void _saveFavorites();
    void _updateDiffSelectedCount();

    /// Assigns value to member and emits changedSignal only if the value actually changed.
    template <typename T, typename SignalFn>
    void _setDiffProperty(T& member, const T& value, SignalFn changedSignal);

    ParameterManager*           _parameterMgr           = nullptr;
    QString                     _searchText;
    QTimer                      _searchTimer;
    ParameterEditorCategory*    _currentCategory        = nullptr;
    ParameterEditorGroup*       _currentGroup           = nullptr;
    bool                        _showModifiedOnly       = false;
    bool                        _showFavoritesOnly      = false;
    bool                        _hideReadOnly           = false;
    bool                        _diffOtherVehicle       = false;
    bool                        _diffMultipleComponents = false;
    int                         _diffParsedCount        = 0;
    int                         _diffUnchangedCount     = 0;
    int                         _diffReadOnlyCount      = 0;
    int                         _diffNoVehicleCount     = 0;
    int                         _diffSendableCount      = 0;
    int                         _diffSelectedCount      = 0;
    QStringList                 _diffMissingParams;
    QSet<QString>               _favoriteNames;

    QmlObjectListModel          _categories;
    QmlObjectListModel          _diffList;
    ParameterTableModel         _searchParameters;
    QAbstractTableModel*        _parameters             = nullptr;
    QMap<QString, ParameterEditorCategory*> _mapCategoryName2Category;
};
