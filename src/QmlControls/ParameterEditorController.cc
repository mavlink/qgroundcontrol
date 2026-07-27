#include "QmlObjectListModel.h"
#include "ParameterEditorController.h"
#include "AppMessages.h"
#include "ParameterManager.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ParameterEditorControllerLog, "QMLControls.ParameterEditorController")

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

QVariant ParameterTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
    case FavColumn:         return tr("Fav");
    case NameColumn:        return tr("Name");
    case ValueColumn:       return tr("Value");
    case DescriptionColumn: return tr("Description");
    default:                return QVariant();
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
    beginReset();
    _tableData.clear();
    endReset();
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
    colData[FavColumn] = QString();
    colData[NameColumn] = fact->name();
    colData[ValueColumn] = QVariant::fromValue(fact);
    colData[DescriptionColumn] = fact->shortDescription();

    if (!_isResetting()) {
        beginInsertRows(QModelIndex(), row, row);
    }
    _tableData.insert(row, colData);
    if (!_isResetting()) {
        endInsertRows();
        emit rowCountChanged(rowCount());
    }
}

void ParameterTableModel::beginReset()
{
    _resetNestingCount++;

    if (_resetNestingCount == 1) {
        beginResetModel();
    }
}

void ParameterTableModel::endReset()
{
    if (_resetNestingCount == 0) {
        qWarning() << "ParameterTableModel::endReset called without prior beginReset";
        return;
    }
    _resetNestingCount--;
    if (_resetNestingCount == 0) {
        endResetModel();
        emit rowCountChanged(rowCount());
    }
}

Fact* ParameterTableModel::factAt(int row) const
{
    if (row < 0 || row >= _tableData.count()) {
        qWarning() << "Invalid row row:rowCount" << row << _tableData.count() << Q_FUNC_INFO;
        return nullptr;
    }

    return _tableData[row][ValueColumn].value<Fact*>();
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
    connect(this, &ParameterEditorController::showFavoritesOnlyChanged, this, &ParameterEditorController::_searchTextChanged);
    connect(this, &ParameterEditorController::hideReadOnlyChanged,     this, &ParameterEditorController::_hideReadOnlyChanged);
    connect(&_searchTimer, &QTimer::timeout,                            this, &ParameterEditorController::_performSearch);
    connect(_parameterMgr, &ParameterManager::factAdded,                this, &ParameterEditorController::_factAdded);

    _loadFavorites();

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

        if (_hideReadOnly && fact->readOnly()) {
            continue;
        }

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
    _currentCategory = nullptr;
    _currentGroup = nullptr;
    _parameters = nullptr;
    _mapCategoryName2Category.clear();
    _categories.clearAndDeleteContents();
    emit parametersChanged();

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
    if (_hideReadOnly && fact->readOnly()) {
        return;
    }

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
            qCWarning(ParameterEditorControllerLog) << "saveToFile: unable to create file" << parameterFilename;
            QGC::showAppMessage(tr("Unable to create file: %1").arg(parameterFilename));
            return;
        }

        qCDebug(ParameterEditorControllerLog) << "saveToFile:" << parameterFilename;
        QTextStream stream(&file);
        _parameterMgr->writeParametersToStream(stream);
        file.close();
    }
}

template <typename T, typename SignalFn>
void ParameterEditorController::_setDiffProperty(T& member, const T& value, SignalFn changedSignal)
{
    if (member == value) {
        return;
    }
    member = value;
    emit (this->*changedSignal)(member);
}

void ParameterEditorController::clearDiff(void)
{
    _diffList.clearAndDeleteContents();
    _setDiffProperty(_diffOtherVehicle,         false,          &ParameterEditorController::diffOtherVehicleChanged);
    _setDiffProperty(_diffMultipleComponents,   false,          &ParameterEditorController::diffMultipleComponentsChanged);
    _setDiffProperty(_diffParsedCount,          0,              &ParameterEditorController::diffParsedCountChanged);
    _setDiffProperty(_diffUnchangedCount,       0,              &ParameterEditorController::diffUnchangedCountChanged);
    _setDiffProperty(_diffReadOnlyCount,        0,              &ParameterEditorController::diffReadOnlyCountChanged);
    _setDiffProperty(_diffNoVehicleCount,       0,              &ParameterEditorController::diffNoVehicleCountChanged);
    _setDiffProperty(_diffSendableCount,        0,              &ParameterEditorController::diffSendableCountChanged);
    _setDiffProperty(_diffSelectedCount,        0,              &ParameterEditorController::diffSelectedCountChanged);
    _setDiffProperty(_diffMissingParams,        QStringList(),  &ParameterEditorController::diffMissingParamsChanged);
}

// Recomputed whenever a diff row's load checkbox changes. Drives the dialog's Ok button enabled state.
void ParameterEditorController::_updateDiffSelectedCount()
{
    int selectedCount = 0;
    for (int i=0; i<_diffList.count(); i++) {
        const ParameterEditorDiff* paramDiff = _diffList.value<ParameterEditorDiff*>(i);
        if (paramDiff->load && !paramDiff->cannotSend) {
            selectedCount++;
        }
    }
    _setDiffProperty(_diffSelectedCount, selectedCount, &ParameterEditorController::diffSelectedCountChanged);
}

void ParameterEditorController::sendDiff(void)
{
    int sentCount = 0;
    int uncheckedCount = 0;

    for (int i=0; i<_diffList.count(); i++) {
        ParameterEditorDiff* paramDiff = _diffList.value<ParameterEditorDiff*>(i);

        if (paramDiff->cannotSend) {
            qCDebug(ParameterEditorControllerLog) << "sendDiff: skipped (cannot send, not on vehicle) -" << paramDiff->name;
            continue;
        }

        if (paramDiff->load) {
            sentCount++;
            if (paramDiff->noVehicleValue) {
                qCDebug(ParameterEditorControllerLog) << "sendDiff: PARAM_SET new param -" << paramDiff->name
                    << "componentId:" << paramDiff->componentId << "value:" << paramDiff->fileValueVar;
                _parameterMgr->_mavlinkParamSet(paramDiff->componentId, paramDiff->name, paramDiff->valueType, paramDiff->fileValueVar);
            } else {
                qCDebug(ParameterEditorControllerLog) << "sendDiff: fact write -" << paramDiff->name
                    << "componentId:" << paramDiff->componentId << "value:" << paramDiff->fileValueVar;
                Fact* fact = _parameterMgr->getParameter(paramDiff->componentId, paramDiff->name);
                fact->setRawValue(paramDiff->fileValueVar);
            }
        } else {
            uncheckedCount++;
            qCDebug(ParameterEditorControllerLog) << "sendDiff: skipped (unchecked) -" << paramDiff->name;
        }
    }

    qCDebug(ParameterEditorControllerLog) << "sendDiff summary - sent:" << sentCount << "unchecked:" << uncheckedCount;
}

bool ParameterEditorController::buildDiffFromFile(const QString& filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(ParameterEditorControllerLog) << "buildDiffFromFile: unable to open file" << filename;
        QGC::showAppMessage(tr("Unable to open file: %1").arg(filename));
        return false;
    }

    clearDiff();

    qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile:" << filename;

    QTextStream stream(&file);

    // Accumulate in locals; property setters at the end emit only for values that changed.
    bool        diffOtherVehicle        = false;
    bool        diffMultipleComponents  = false;
    int         diffParsedCount         = 0;
    int         diffUnchangedCount      = 0;
    int         diffReadOnlyCount       = 0;
    int         diffNoVehicleCount      = 0;
    int         diffSendableCount       = 0;
    QStringList diffMissingParams;

    int firstComponentId = -1;
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (!line.startsWith("#") && !line.trimmed().isEmpty()) {
            QStringList wpParams = line.trimmed().split(QRegularExpression("[\\t ,]+"));

            int         componentId     = -1;
            QString     paramName;
            QString     fileValueStr;
            int         mavParamType    = -1;
            bool        isMPFormat      = false;

            if (wpParams.size() == 5) {
                // QGC tab-delimited: VehicleId ComponentId Name Value Type
                int vehicleId   = wpParams.at(0).toInt();
                componentId     = wpParams.at(1).toInt();
                paramName       = wpParams.at(2);
                fileValueStr    = wpParams.at(3);
                mavParamType    = wpParams.at(4).toInt();

                if (_vehicle->id() != vehicleId) {
                    if (!diffOtherVehicle) {
                        qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: file is from other vehicle - file vehicleId:" << vehicleId << "connected vehicleId:" << _vehicle->id();
                    }
                    diffOtherVehicle = true;
                }
                if (firstComponentId == -1) {
                    firstComponentId = componentId;
                } else if (firstComponentId != componentId) {
                    if (!diffMultipleComponents) {
                        qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: file contains multiple componentIds:" << firstComponentId << componentId;
                    }
                    diffMultipleComponents = true;
                }
            } else if (wpParams.size() == 2) {
                // Mission Planner 2-column: Name Value
                paramName       = wpParams.at(0);
                fileValueStr    = wpParams.at(1);
                componentId     = ParameterManager::defaultComponentId;
                isMPFormat      = true;
            } else {
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: skipping unparseable line:" << line.trimmed().left(80);
                continue;
            }

            diffParsedCount++;

            QString     vehicleValueStr;
            QString     units;
            QVariant    fileValueVar    = fileValueStr;
            bool        noVehicleValue  = false;
            bool        readOnly        = false;

            if (_parameterMgr->parameterExists(componentId, paramName)) {
                Fact*           vehicleFact         = _parameterMgr->getParameter(componentId, paramName);
                FactMetaData*   vehicleFactMetaData = vehicleFact->metaData();
                Fact            fileFact(vehicleFact->componentId(), vehicleFact->name(), vehicleFact->type());

                if (mavParamType == -1) {
                    mavParamType = ParameterManager::factTypeToMavType(vehicleFact->type());
                }

                // Turn off reboot messaging before setting value in fileFact
                bool vehicleRebootRequired = vehicleFactMetaData->vehicleRebootRequired();
                vehicleFactMetaData->setVehicleRebootRequired(false);
                fileFact.setMetaData(vehicleFact->metaData());
                fileFact.setRawValue(fileValueStr);
                vehicleFactMetaData->setVehicleRebootRequired(vehicleRebootRequired);
                readOnly = vehicleFact->readOnly();

                if (vehicleFact->rawValue() == fileFact.rawValue()) {
                    diffUnchangedCount++;
                    continue;
                }
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: changed -" << paramName
                    << "vehicle:" << vehicleFact->rawValue() << "file:" << fileFact.rawValue();
                fileValueStr    = fileFact.enumOrValueString();
                fileValueVar    = fileFact.rawValue();
                vehicleValueStr = vehicleFact->enumOrValueString();
                units           = vehicleFact->cookedUnits();
            } else if (isMPFormat) {
                // MP format: param not on vehicle and file carries no type info, so it can never
                // be sent. Show it in the diff list as a disabled (cannot-send) row.
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: not on vehicle, cannot send (MP format) -" << paramName;
                diffMissingParams.append(paramName);

                ParameterEditorDiff* paramDiff = new ParameterEditorDiff(this);

                paramDiff->componentId  = componentId;
                paramDiff->name         = paramName;
                paramDiff->valueType    = FactMetaData::valueTypeFloat;    // Unknown - never sent
                paramDiff->fileValue    = fileValueStr;
                paramDiff->fileValueVar = fileValueVar;
                paramDiff->cannotSend   = true;
                paramDiff->load         = false;

                _diffList.append(paramDiff);
                continue;
            } else {
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: not on vehicle, will send as new (QGC format) -" << paramName << "value:" << fileValueStr;
                noVehicleValue = true;

                // fileValueVar is still a QString variant. Convert it to the typed variant matching the
                // file's param type, otherwise the PARAM_VALUE ack type check in _mavlinkParamSet fails
                // and the send would retry/time out.
                const FactMetaData metaData(ParameterManager::mavTypeToFactType(static_cast<MAV_PARAM_TYPE>(mavParamType)));
                QVariant    typedValue;
                QString     errorString;
                if (metaData.convertAndValidateRaw(fileValueVar, true /* convertOnly */, typedValue, errorString)) {
                    fileValueVar = typedValue;
                } else {
                    qCWarning(ParameterEditorControllerLog) << "buildDiffFromFile: value conversion failed, skipping -" << paramName
                        << "value:" << fileValueStr << "error:" << errorString;
                    diffParsedCount--;
                    continue;
                }
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

                (void) connect(paramDiff, &ParameterEditorDiff::loadChanged, this, &ParameterEditorController::_updateDiffSelectedCount);

                _diffList.append(paramDiff);
                diffSendableCount++;

                if (noVehicleValue) {
                    diffNoVehicleCount++;
                }
            } else {
                qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile: skipping read-only param -" << paramName;
                diffReadOnlyCount++;
            }
        }
    }

    file.close();

    if (diffParsedCount == 0) {
        QGC::showAppMessage(tr("No valid parameters found in file. Check that the file is in QGC or Mission Planner format."));
        return false;
    }

    qCDebug(ParameterEditorControllerLog) << "buildDiffFromFile summary -"
        << "parsed:" << diffParsedCount
        << "changed:" << diffSendableCount
        << "unchanged:" << diffUnchangedCount
        << "readOnly:" << diffReadOnlyCount
        << "noVehicleValue:" << diffNoVehicleCount
        << "missing:" << diffMissingParams.count()
        << (diffMissingParams.isEmpty() ? QString() : diffMissingParams.join(QStringLiteral(", ")));

    _setDiffProperty(_diffOtherVehicle,         diffOtherVehicle,       &ParameterEditorController::diffOtherVehicleChanged);
    _setDiffProperty(_diffMultipleComponents,   diffMultipleComponents, &ParameterEditorController::diffMultipleComponentsChanged);
    _setDiffProperty(_diffParsedCount,          diffParsedCount,        &ParameterEditorController::diffParsedCountChanged);
    _setDiffProperty(_diffUnchangedCount,       diffUnchangedCount,     &ParameterEditorController::diffUnchangedCountChanged);
    _setDiffProperty(_diffReadOnlyCount,        diffReadOnlyCount,      &ParameterEditorController::diffReadOnlyCountChanged);
    _setDiffProperty(_diffNoVehicleCount,       diffNoVehicleCount,     &ParameterEditorController::diffNoVehicleCountChanged);
    _setDiffProperty(_diffSendableCount,        diffSendableCount,      &ParameterEditorController::diffSendableCountChanged);
    _setDiffProperty(_diffSelectedCount,        diffSendableCount,      &ParameterEditorController::diffSelectedCountChanged); // All sendable rows start checked
    _setDiffProperty(_diffMissingParams,        diffMissingParams,      &ParameterEditorController::diffMissingParamsChanged);

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
    if (_hideReadOnly && fact->readOnly()) {
        return false;
    }
    if (_showModifiedOnly) {
        if (!fact->defaultValueAvailable() || fact->valueEqualsDefault()) {
            return false;
        }
    }
    if (_showFavoritesOnly) {
        if (!_favoriteNames.contains(fact->name())) {
            return false;
        }
    }
    return true;
}

void ParameterEditorController::_searchTextChanged(void)
{
    _searchTimer.start();
}

void ParameterEditorController::_hideReadOnlyChanged(void)
{
    _buildLists();

    ParameterEditorCategory* category = _categories.count() ? _categories.value<ParameterEditorCategory*>(0) : nullptr;
    setCurrentCategory(category);

    // Re-trigger search if active
    if (!_searchText.isEmpty() || _showModifiedOnly) {
        _performSearch();
    }
}

void ParameterEditorController::_performSearch(void)
{
    QObjectList newParameterList;

    QStringList rgSearchStrings = _searchText.split(' ', Qt::SkipEmptyParts);

    if (rgSearchStrings.isEmpty() && !_showModifiedOnly && !_showFavoritesOnly) {
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

        for (int compId : _parameterMgr->componentIds()) {
            for (const QString &paraName: _parameterMgr->parameterNames(compId)) {
                Fact* fact = _parameterMgr->getParameter(compId, paraName);
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

QStringList ParameterEditorController::favoriteParameterNames(void) const
{
    QStringList list(_favoriteNames.begin(), _favoriteNames.end());
    list.sort();
    return list;
}

void ParameterEditorController::toggleFavorite(const QString& paramName)
{
    if (_favoriteNames.contains(paramName)) {
        _favoriteNames.remove(paramName);
    } else {
        _favoriteNames.insert(paramName);
    }
    _saveFavorites();
    emit favoritesChanged();

    if (_showFavoritesOnly) {
        _performSearch();
    }
}

bool ParameterEditorController::isFavorite(const QString& paramName) const
{
    return _favoriteNames.contains(paramName);
}

void ParameterEditorController::clearAllFavorites(void)
{
    _favoriteNames.clear();
    _saveFavorites();
    emit favoritesChanged();

    if (_showFavoritesOnly) {
        _performSearch();
    }
}

void ParameterEditorController::_loadFavorites()
{
    Fact* fact = SettingsManager::instance()->appSettings()->favoriteParameters();
    const QStringList list = fact->rawValue().toString().split(",", Qt::SkipEmptyParts);
    _favoriteNames = QSet<QString>(list.begin(), list.end());
}

void ParameterEditorController::_saveFavorites()
{
    QStringList list(_favoriteNames.begin(), _favoriteNames.end());
    list.sort();
    Fact* fact = SettingsManager::instance()->appSettings()->favoriteParameters();
    fact->setRawValue(list.join(","));
}
