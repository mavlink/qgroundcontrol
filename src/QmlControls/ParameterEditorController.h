/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QList>

#include "AutoPilotPlugin.h"
#include "UASInterface.h"
#include "FactPanelController.h"
#include "QmlObjectListModel.h"
#include "ParameterManager.h"

class ParameterEditorGroup : public QObject
{
    Q_OBJECT

public:
    ParameterEditorGroup(QObject* parent) : QObject(parent) { }

    Q_PROPERTY(QString              name    MEMBER name     CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  facts   READ getFacts   CONSTANT)

    QmlObjectListModel*  getFacts(void) { return &facts; }

    int                 componentId;
    QString             name;
    QmlObjectListModel  facts;
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

public:
    ParameterEditorController(void);
    ~ParameterEditorController();

    Q_PROPERTY(QString              searchText          MEMBER _searchText                                          NOTIFY searchTextChanged)
    Q_PROPERTY(QmlObjectListModel*  categories          READ categories                                             CONSTANT)
    Q_PROPERTY(QObject*             currentCategory     READ currentCategory        WRITE setCurrentCategory        NOTIFY currentCategoryChanged)
    Q_PROPERTY(QObject*             currentGroup        READ currentGroup           WRITE setCurrentGroup           NOTIFY currentGroupChanged)
    Q_PROPERTY(QmlObjectListModel*  parameters          MEMBER _parameters                                          NOTIFY parametersChanged)
    Q_PROPERTY(bool                 showModifiedOnly    MEMBER _showModifiedOnly                                    NOTIFY showModifiedOnlyChanged)

    // These property are related to the diff associated with a load from file
    Q_PROPERTY(bool                 diffOtherVehicle        MEMBER _diffOtherVehicle        NOTIFY diffOtherVehicleChanged)
    Q_PROPERTY(bool                 diffMultipleComponents  MEMBER _diffMultipleComponents  NOTIFY diffMultipleComponentsChanged)
    Q_PROPERTY(QmlObjectListModel*  diffList                READ diffList                   CONSTANT)

    Q_INVOKABLE QStringList searchParameters(const QString& searchText, bool searchInName=true, bool searchInDescriptions=true);

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

private:
    ParameterManager*           _parameterMgr           = nullptr;
    QString                     _searchText;
    ParameterEditorCategory*    _currentCategory        = nullptr;
    ParameterEditorGroup*       _currentGroup           = nullptr;
    bool                        _showModifiedOnly       = false;
    bool                        _diffOtherVehicle       = false;
    bool                        _diffMultipleComponents = false;

    QmlObjectListModel          _categories;
    QmlObjectListModel          _diffList;
    QmlObjectListModel          _searchParameters;
    QmlObjectListModel*         _parameters             = nullptr;
    QMap<QString, ParameterEditorCategory*> _mapCategoryName2Category;
};
