/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactPanelController.h"
#include "QmlObjectListModel.h"
#include "FactMetaData.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(ParameterEditorControllerLog)

class ParameterManager;

class ParameterTableModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    ParameterTableModel(QObject* parent = nullptr);
    ~ParameterTableModel() override;

    typedef QVector<QVariant> ColumnData;
    
    enum {
        FactRole = Qt::UserRole + 1
    };

    enum {
        NameColumn = 0,
        ValueColumn,
        DescriptionColumn,
    };

    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
    
    void append      (Fact* fact);
    void insert      (int row, Fact* fact);
    void clear       ();
    void beginReset  ();
    void endReset    ();
    Fact*            factAt(int row) const;

    // Overrides from QAbstractTableModel
    int         rowCount    (const QModelIndex & parent = QModelIndex()) const override;
    int         columnCount (const QModelIndex &parent = QModelIndex()) const override;
    QVariant    data        (const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames(void) const override;

    signals:
    void rowCountChanged(int count);

private:
    int                 _tableViewColCount = 3;
    QList<ColumnData>   _tableData;
    bool                _externalBeginResetModel = false;
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
    Q_PROPERTY(QString  units               MEMBER units            CONSTANT)
    Q_PROPERTY(bool     load                MEMBER load             NOTIFY loadChanged)

    int                         componentId;
    QString                     name;
    FactMetaData::ValueType_t   valueType;
    QString                     fileValue;
    QVariant                    fileValueVar;
    QString                     vehicleValue;
    bool                        noVehicleValue  = false;
    QString                     units;
    bool                        load            = true;

signals:
    void loadChanged(bool load);
};

class ParameterEditorController : public FactPanelController
{
    Q_OBJECT
    // QML_ELEMENT
    Q_PROPERTY(QString              searchText              MEMBER _searchText                                          NOTIFY searchTextChanged)
    Q_PROPERTY(QmlObjectListModel*  categories              READ categories                                             CONSTANT)
    Q_PROPERTY(QObject*             currentCategory         READ currentCategory            WRITE setCurrentCategory    NOTIFY currentCategoryChanged)
    Q_PROPERTY(QObject*             currentGroup            READ currentGroup               WRITE setCurrentGroup       NOTIFY currentGroupChanged)
    Q_PROPERTY(QAbstractTableModel* parameters              MEMBER _parameters                                          NOTIFY parametersChanged)
    Q_PROPERTY(bool                 showModifiedOnly        MEMBER _showModifiedOnly                                    NOTIFY showModifiedOnlyChanged)

    // These property are related to the diff associated with a load from file
    Q_PROPERTY(bool                 diffOtherVehicle        MEMBER _diffOtherVehicle                                    NOTIFY diffOtherVehicleChanged)
    Q_PROPERTY(bool                 diffMultipleComponents  MEMBER _diffMultipleComponents                              NOTIFY diffMultipleComponentsChanged)
    Q_PROPERTY(QmlObjectListModel*  diffList                READ diffList                                               CONSTANT)

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

    QObject*            currentCategory     (void) { return _currentCategory; }
    QObject*            currentGroup        (void) { return _currentGroup; }
    QmlObjectListModel* categories          (void) { return &_categories; }
    QmlObjectListModel* diffList            (void) { return &_diffList; }
    void                setCurrentCategory  (QObject* currentCategory);
    void                setCurrentGroup     (QObject* currentGroup);

signals:
    void searchTextChanged              (QString searchText);
    void currentCategoryChanged         (void);
    void currentGroupChanged            (void);
    void showModifiedOnlyChanged        (void);
    void diffOtherVehicleChanged        (bool diffOtherVehicle);
    void diffMultipleComponentsChanged  (bool diffMultipleComponents);
    void parametersChanged              (void);

private slots:
    void _currentCategoryChanged(void);
    void _currentGroupChanged   (void);
    void _searchTextChanged     (void);
    void _buildLists            (void);
    void _buildListsForComponent(int compId);
    void _factAdded             (int compId, Fact* fact);

private:
    bool _shouldShow(Fact *fact) const;
    void _performSearch();

private:
    ParameterManager*           _parameterMgr           = nullptr;
    QString                     _searchText;
    QTimer                      _searchTimer;
    ParameterEditorCategory*    _currentCategory        = nullptr;
    ParameterEditorGroup*       _currentGroup           = nullptr;
    bool                        _showModifiedOnly       = false;
    bool                        _diffOtherVehicle       = false;
    bool                        _diffMultipleComponents = false;

    QmlObjectListModel          _categories;
    QmlObjectListModel          _diffList;
    ParameterTableModel         _searchParameters;
    QAbstractTableModel*        _parameters             = nullptr;
    QMap<QString, ParameterEditorCategory*> _mapCategoryName2Category;
};
