/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParameterEditorController.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "AppSettings.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ParameterEditorControllerLog, "qgc.qmlcontrols.parametereditorcontroller")

ParameterTableModel::ParameterTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{

}

ParameterTableModel::~ParameterTableModel()
{
    
}

int ParameterTableModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _tableData.count();
}

int ParameterTableModel::columnCount(const QModelIndex & /*parent*/) const
{
    return _tableViewColCount;
}

QVariant ParameterTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    
    if (index.row() < 0 || index.row() >= _tableData.count()) {
        return QVariant();
    }
    if (index.column() < 0 || index.column() >= _tableViewColCount) {
        return QVariant();
    }
    
    switch (role) {
        case Qt::DisplayRole:
            return QVariant::fromValue(_tableData[index.row()][index.column()]);
        case FactRole:
            return QVariant::fromValue(_tableData[index.row()][ValueColumn]);
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ParameterTableModel::roleNames() const
{
    return { 
        {Qt::DisplayRole, "display"},
        {FactRole, "fact"}
    };
}

void ParameterTableModel::clear()
{
    if (!_externalBeginResetModel) {
        beginResetModel();
    }
    _tableData.clear();
    if (!_externalBeginResetModel) {
        endResetModel();
        emit rowCountChanged(0);
    }
}

void ParameterTableModel::append(Fact* fact)
{
    insert(rowCount(), fact);
}

void ParameterTableModel::insert(int row, Fact* fact)
{
    if (row < 0 || row > rowCount()) {
        qWarning() << "Invalid row row:rowCount" << row << rowCount() << Q_FUNC_INFO;
        row = qMax(qMin(row, rowCount()), 0);
    }

    ColumnData colData(_tableViewColCount, QString());
    colData[NameColumn] = fact->name();
    colData[ValueColumn] = QVariant::fromValue(fact);
    colData[DescriptionColumn] = fact->shortDescription();

    beginInsertRows(QModelIndex(), row, row);
    _tableData.insert(row, colData);
    endInsertRows();

    emit rowCountChanged(rowCount());
}

void ParameterTableModel::beginReset()
{
    if (_externalBeginResetModel) {
        qWarning() << "ParameterTableModel::beginReset already set";
    }
    _externalBeginResetModel = true;
    beginResetModel();
}

void ParameterTableModel::endReset()
{
    if (!_externalBeginResetModel) {
        qWarning() << "ParameterTableModel::endReset begin not set";
    }
    _externalBeginResetModel = false;
    endResetModel();
}

Fact* ParameterTableModel::factAt(int row) const
{
    if (row < 0 || row >= _tableData.count()) {
        qWarning() << "Invalid row row:rowCount" << row << _tableData.count() << Q_FUNC_INFO;
        return nullptr;
    }

    return _tableData[row][0].value<Fact*>();
}


ParameterEditorGroup::ParameterEditorGroup(QObject* parent) 
    : QObject(parent) 
{ 

}

ParameterEditorController::ParameterEditorController(QObject *parent)
    : FactPanelController(parent)
    , _parameterMgr(_vehicle->parameterManager())
{
    // qCDebug(ParameterEditorControllerLog) << Q_FUNC_INFO << this;

    _buildLists();

    _searchTimer.setSingleShot(true);
    _searchTimer.setInterval(300);

    connect(this, &ParameterEditorController::currentCategoryChanged,   this, &ParameterEditorController::_currentCategoryChanged);
    connect(this, &ParameterEditorController::currentGroupChanged,      this, &ParameterEditorController::_currentGroupChanged);
    connect(this, &ParameterEditorController::searchTextChanged,        this, &ParameterEditorController::_searchTextChanged);
    connect(this, &ParameterEditorController::showModifiedOnlyChanged,  this, &ParameterEditorController::_searchTextChanged);
    connect(&_searchTimer, &QTimer::timeout,                            this, &ParameterEditorController::_performSearch);
    connect(_parameterMgr, &ParameterManager::factAdded,                this, &ParameterEditorController::_factAdded);

    ParameterEditorCategory* category = _categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr;
    setCurrentCategory(category);
}

ParameterEditorController::~ParameterEditorController()
{
    // qCDebug(ParameterEditorControllerLog) << Q_FUNC_INFO << this;
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
                    category->groups.append(group);
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
    auto& facts = group->facts;
    for (int i=0; i<facts.rowCount(); i++) {
        if (facts.factAt(i)->name() > fact->name()) {
            facts.insert(i, fact);
            return;
        }
    }
    facts.append(fact);
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
    _searchTimer.start();
}

void ParameterEditorController::_performSearch(void)
{
    QObjectList newParameterList;

    QStringList rgSearchStrings = _searchText.split(' ', Qt::SkipEmptyParts);

    if (rgSearchStrings.isEmpty() && !_showModifiedOnly) {
        ParameterEditorCategory* category = _categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr;
        setCurrentCategory(category);
        _searchParameters.clear();
    } else {
        QVector<QRegularExpression> regexList;
        regexList.reserve(rgSearchStrings.size());
        for (const QString &searchItem : rgSearchStrings) {
            QRegularExpression re(searchItem, QRegularExpression::CaseInsensitiveOption);
            regexList.append(re.isValid() ? re : QRegularExpression());
        }

        _searchParameters.beginReset();
        _searchParameters.clear();

        for (const QString &paraName: _parameterMgr->parameterNames(_vehicle->defaultComponentId())) {
            Fact* fact = _parameterMgr->getParameter(_vehicle->defaultComponentId(), paraName);
            bool matched = _shouldShow(fact);
            // All of the search items must match in order for the parameter to be added to the list
            if (matched) {
                for (int i = 0; i < rgSearchStrings.size(); ++i) {
                    const QRegularExpression &re = regexList.at(i);
                    if (re.isValid()) {
                        if (!fact->name().contains(re) &&
                                !fact->shortDescription().contains(re) &&
                                !fact->longDescription().contains(re)) {
                            matched = false;
                        }
                    } else {
                        const QString &searchItem = rgSearchStrings.at(i);
                        if (!fact->name().contains(searchItem, Qt::CaseInsensitive) &&
                                !fact->shortDescription().contains(searchItem, Qt::CaseInsensitive) &&
                                !fact->longDescription().contains(searchItem, Qt::CaseInsensitive)) {
                            matched = false;
                        }                    
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
