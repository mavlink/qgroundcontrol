/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParameterEditorController.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QStandardPaths>

ParameterEditorController::ParameterEditorController(void)
    : _parameterMgr(_vehicle->parameterManager())
{
    _buildLists();

    connect(this, &ParameterEditorController::currentCategoryChanged,   this, &ParameterEditorController::_currentCategoryChanged);
    connect(this, &ParameterEditorController::currentGroupChanged,      this, &ParameterEditorController::_currentGroupChanged);
    connect(this, &ParameterEditorController::searchTextChanged,        this, &ParameterEditorController::_searchTextChanged);
    connect(this, &ParameterEditorController::showModifiedOnlyChanged,  this, &ParameterEditorController::_searchTextChanged);

    connect(_parameterMgr, &ParameterManager::factAdded, this, &ParameterEditorController::_factAdded);

    ParameterEditorCategory* category = _categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr;
    setCurrentCategory(category);
}

ParameterEditorController::~ParameterEditorController()
{

}

void ParameterEditorController::_buildListsForComponent(int compId)
{
    for (const QString& factName: _parameterMgr->parameterNames(compId)) {
        Fact* fact = _parameterMgr->getParameter(compId, factName);

        ParameterEditorCategory* category = nullptr;
        if (_mapCategoryName2Category.contains(fact->category())) {
            category = _mapCategoryName2Category[fact->category()];
        } else {
            category        = new ParameterEditorCategory(this);
            category->name  = fact->category();
            _mapCategoryName2Category[fact->category()] = category;
            _categories.append(category);
        }

        ParameterEditorGroup* group = nullptr;
        if (category->mapGroupName2Group.contains(fact->group())) {
            group = category->mapGroupName2Group[fact->group()];
        } else {
            group               = new ParameterEditorGroup(this);
            group->componentId  = compId;
            group->name         = fact->group();
            category->mapGroupName2Group[fact->group()] = group;
            category->groups.append(group);
        }

        group->facts.append(fact);
    }
}

void ParameterEditorController::_buildLists(void)
{
    // Autopilot component should always be first list
    _buildListsForComponent(MAV_COMP_ID_AUTOPILOT1);

    // "Standard" category should always be first
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        if (category->name == "Standard" && i != 0) {
            _categories.removeAt(i);
            _categories.insert(0, category);
            break;
        }
    }

    // Default category should always be last
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        if (category->name == FactMetaData::kDefaultCategory) {
            if (i != _categories.count() - 1) {
                _categories.removeAt(i);
                _categories.append(category);
            }
            break;
        }
    }

    // Now add other random components
    for (int compId: _parameterMgr->componentIds()) {
        if (compId != MAV_COMP_ID_AUTOPILOT1) {
            _buildListsForComponent(compId);
        }
    }

    // Default group should always be last
    for (int i=0; i<_categories.count(); i++) {
        ParameterEditorCategory* category = _categories.value<ParameterEditorCategory*>(i);
        for (int j=0; j<category->groups.count(); j++) {
            ParameterEditorGroup* group = category->groups.value<ParameterEditorGroup*>(j);
            if (group->name == FactMetaData::kDefaultGroup) {
                if (j != _categories.count() - 1) {
                    category->groups.removeAt(j);
                    category->groups.append(category);
                }
                break;
            }
        }
    }
}

void ParameterEditorController::_factAdded(int compId, Fact* fact)
{
    bool                        inserted = false;
    ParameterEditorCategory*    category = nullptr;

    if (_mapCategoryName2Category.contains(fact->category())) {
        category = _mapCategoryName2Category[fact->category()];
    } else {
        category        = new ParameterEditorCategory(this);
        category->name  = fact->category();
        _mapCategoryName2Category[fact->category()] = category;

        // Insert in sorted order
        inserted = false;
        for (int i=0; i<_categories.count(); i++) {
            if (_categories.value<ParameterEditorCategory*>(i)->name > category->name) {
                _categories.insert(i, category);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            _categories.append(category);
        }
    }

    ParameterEditorGroup* group = nullptr;
    if (category->mapGroupName2Group.contains(fact->group())) {
        group = category->mapGroupName2Group[fact->group()];
    } else {
        group               = new ParameterEditorGroup(this);
        group->componentId  = compId;
        group->name         = fact->group();
        category->mapGroupName2Group[fact->group()] = group;

        // Insert in sorted order
        QmlObjectListModel& groups = category->groups;
        inserted = false;
        for (int i=0; i<groups.count(); i++) {
            if (groups.value<ParameterEditorGroup*>(i)->name > group->name) {
                groups.insert(i, group);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            groups.append(group);
        }
    }

    // Insert in sorted order
    QmlObjectListModel& facts = group->facts;
    inserted = false;
    for (int i=0; i<facts.count(); i++) {
        if (facts.value<Fact*>(i)->name() > fact->name()) {
            facts.insert(i, fact);
            inserted = true;
            break;
        }
    }
    if (!inserted) {
        facts.append(fact);
    }
}

QStringList ParameterEditorController::searchParameters(const QString& searchText, bool searchInName, bool searchInDescriptions)
{
    QStringList list;

    for(const QString &paramName: _parameterMgr->parameterNames(_vehicle->defaultComponentId())) {
        if (searchText.isEmpty()) {
            list += paramName;
        } else {
            Fact* fact = _parameterMgr->getParameter(_vehicle->defaultComponentId(), paramName);

            if (searchInName && fact->name().contains(searchText, Qt::CaseInsensitive)) {
                list += paramName;
            } else if (searchInDescriptions && (fact->shortDescription().contains(searchText, Qt::CaseInsensitive) || fact->longDescription().contains(searchText, Qt::CaseInsensitive))) {
                list += paramName;
            }
        }
    }
    list.sort();

    return list;
}

void ParameterEditorController::saveToFile(const QString& filename)
{
    if (!filename.isEmpty()) {
        QString parameterFilename = filename;
        if (!QFileInfo(filename).fileName().contains(".")) {
            parameterFilename += QString(".%1").arg(AppSettings::parameterFileExtension);
        }

        QFile file(parameterFilename);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showAppMessage(tr("Unable to create file: %1").arg(parameterFilename));
            return;
        }

        QTextStream stream(&file);
        _parameterMgr->writeParametersToStream(stream);
        file.close();
    }
}

void ParameterEditorController::clearDiff(void)
{
    _diffList.clearAndDeleteContents();
    _diffOtherVehicle = false;
    _diffMultipleComponents = false;

    emit diffOtherVehicleChanged(_diffOtherVehicle);
    emit diffMultipleComponentsChanged(_diffMultipleComponents);
}

void ParameterEditorController::sendDiff(void)
{
    for (int i=0; i<_diffList.count(); i++) {
        ParameterEditorDiff* paramDiff = _diffList.value<ParameterEditorDiff*>(i);

        if (paramDiff->load) {
            if (paramDiff->noVehicleValue) {
                _parameterMgr->_factRawValueUpdateWorker(paramDiff->componentId, paramDiff->name, paramDiff->valueType, paramDiff->fileValueVar);
            } else {
                Fact* fact = _parameterMgr->getParameter(paramDiff->componentId, paramDiff->name);
                fact->setRawValue(paramDiff->fileValueVar);
            }
        }
    }
}

bool ParameterEditorController::buildDiffFromFile(const QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(tr("Unable to open file: %1").arg(filename));
        return false;
    }

    clearDiff();

    QTextStream stream(&file);

    int firstComponentId = -1;
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (!line.startsWith("#")) {
            QStringList wpParams = line.split("\t");
            if (wpParams.size() == 5) {
                int         vehicleId       = wpParams.at(0).toInt();
                int         componentId     = wpParams.at(1).toInt();
                QString     paramName       = wpParams.at(2);
                QString     fileValueStr    = wpParams.at(3);
                int         mavParamType    = wpParams.at(4).toInt();
                QString     vehicleValueStr;
                QString     units;
                QVariant    fileValueVar    = fileValueStr;
                bool        noVehicleValue   = false;
                bool        readOnly         = false;

                if (_vehicle->id() != vehicleId) {
                    _diffOtherVehicle = true;
                }
                if (firstComponentId == -1) {
                    firstComponentId = componentId;
                } else if (firstComponentId != componentId) {
                    _diffMultipleComponents = true;
                }

                if (_parameterMgr->parameterExists(componentId, paramName)) {
                    Fact*           vehicleFact         = _parameterMgr->getParameter(componentId, paramName);
                    FactMetaData*   vehicleFactMetaData = vehicleFact->metaData();
                    Fact*           fileFact            = new Fact(vehicleFact->componentId(), vehicleFact->name(), vehicleFact->type(), this);

                    // Turn off reboot messaging before setting value in fileFact
                    bool vehicleRebootRequired = vehicleFactMetaData->vehicleRebootRequired();
                    vehicleFactMetaData->setVehicleRebootRequired(false);
                    fileFact->setMetaData(vehicleFact->metaData());
                    fileFact->setRawValue(fileValueStr);
                    vehicleFactMetaData->setVehicleRebootRequired(vehicleRebootRequired);
                    readOnly = vehicleFact->readOnly();

                    if (vehicleFact->rawValue() == fileFact->rawValue()) {
                        continue;
                    }
                    fileValueStr    = fileFact->enumOrValueString();
                    fileValueVar    = fileFact->rawValue();
                    vehicleValueStr = vehicleFact->enumOrValueString();
                    units           = vehicleFact->cookedUnits();
                } else {
                    noVehicleValue = true;
                }

                if (!readOnly) {
                    ParameterEditorDiff* paramDiff = new ParameterEditorDiff(this);

                    paramDiff->componentId      = componentId;
                    paramDiff->name             = paramName;
                    paramDiff->valueType        = ParameterManager::mavTypeToFactType(static_cast<MAV_PARAM_TYPE>(mavParamType));
                    paramDiff->fileValue        = fileValueStr;
                    paramDiff->fileValueVar     = fileValueVar;
                    paramDiff->vehicleValue     = vehicleValueStr;
                    paramDiff->noVehicleValue   = noVehicleValue;
                    paramDiff->units            = units;

                    _diffList.append(paramDiff);
                }
            }
        }
    }

    file.close();

    emit diffOtherVehicleChanged(_diffOtherVehicle);
    emit diffMultipleComponentsChanged(_diffMultipleComponents);

    return true;
}

void ParameterEditorController::refresh(void)
{
    _parameterMgr->refreshAllParameters();
}

void ParameterEditorController::resetAllToDefaults(void)
{
    _parameterMgr->resetAllParametersToDefaults();
    refresh();
}

void ParameterEditorController::resetAllToVehicleConfiguration(void)
{
    _parameterMgr->resetAllToVehicleConfiguration();
    refresh();
}

bool ParameterEditorController::_shouldShow(Fact* fact) const
{
    if (!_showModifiedOnly) {
        return true;
    }

    return fact->defaultValueAvailable() && !fact->valueEqualsDefault();
}

void ParameterEditorController::_searchTextChanged(void)
{
    QObjectList newParameterList;

    QStringList rgSearchStrings = _searchText.split(' ', Qt::SkipEmptyParts);

    if (rgSearchStrings.isEmpty() && !_showModifiedOnly) {
        ParameterEditorCategory* category = _categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr;
        setCurrentCategory(category);
        _searchParameters.clear();
    } else {
        _searchParameters.beginReset();
        _searchParameters.clear();

        for (const QString &paraName: _parameterMgr->parameterNames(_vehicle->defaultComponentId())) {
            Fact* fact = _parameterMgr->getParameter(_vehicle->defaultComponentId(), paraName);
            bool matched = _shouldShow(fact);
            // All of the search items must match in order for the parameter to be added to the list
            if (matched) {
                for (const auto& searchItem : rgSearchStrings) {
                    if (!fact->name().contains(searchItem, Qt::CaseInsensitive) &&
                            !fact->shortDescription().contains(searchItem, Qt::CaseInsensitive) &&
                            !fact->longDescription().contains(searchItem, Qt::CaseInsensitive)) {
                        matched = false;
                    }
                }
            }
            if (matched) {
                _searchParameters.append(fact);
            }
        }

        _searchParameters.endReset();

        if (_parameters != &_searchParameters) {
            _parameters = &_searchParameters;
            emit parametersChanged();

            _currentCategory    = nullptr;
            _currentGroup       = nullptr;
        }
    }
}

void ParameterEditorController::_currentCategoryChanged(void)
{
    ParameterEditorGroup* group = nullptr;
    if (_currentCategory) {
        // Select first group when category changes
        group = _currentCategory->groups.value<ParameterEditorGroup*>(0);
    } else {
        group = nullptr;
    }
    setCurrentGroup(group);
}

void ParameterEditorController::_currentGroupChanged(void)
{
    _parameters = _currentGroup ? &_currentGroup->facts : nullptr;
    emit parametersChanged();
}

void ParameterEditorController::setCurrentCategory(QObject* currentCategory)
{
    ParameterEditorCategory* category = qobject_cast<ParameterEditorCategory*>(currentCategory);
    if (category != _currentCategory) {
        _currentCategory = category;
        emit currentCategoryChanged();
    }
}

void ParameterEditorController::setCurrentGroup(QObject* currentGroup)
{
    ParameterEditorGroup* group = qobject_cast<ParameterEditorGroup*>(currentGroup);
    if (group != _currentGroup) {
        _currentGroup = group;
        emit currentGroupChanged();
    }
}
