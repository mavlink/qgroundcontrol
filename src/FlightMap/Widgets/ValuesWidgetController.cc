/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ValuesWidgetController.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QSettings>

const char* ValuesWidgetController::_groupKey =         "ValuesWidget2";
const char* ValuesWidgetController::_rowsKey =          "rows";
const char* ValuesWidgetController::_columnsKey =       "columns";

const char* ValuesWidgetController::_deprecatedGroupKey =         "ValuesWidget";
const char* ValuesWidgetController::_deprecatedLargeValuesKey =   "large";
const char* ValuesWidgetController::_deprecatedSmallValuesKey =   "small";

ValuesWidgetController::ValuesWidgetController(bool forDefaultSettingsCreation)
    : _valuesModel(new QmlObjectListModel(this))
{
    QSettings settings;

    settings.beginGroup(_groupKey);

    _multiVehicleMgr = qgcApp()->toolbox()->multiVehicleManager();
    connect(_multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &ValuesWidgetController::_activeVehicleChanged);

    if (!forDefaultSettingsCreation) {
        _loadSettings();
    }
}

void ValuesWidgetController::_connectSignalsToController(InstrumentValue* value, ValuesWidgetController* controller)
{
    connect(value, &InstrumentValue::factNameChanged,       controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::factGroupNameChanged,  controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::textChanged,           controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::fontSizeChanged,       controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::showUnitsChanged,      controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::iconChanged,           controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::labelPositionChanged,   controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::rangeTypeChanged,      controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::rangeValuesChanged,    controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::rangeColorsChanged,    controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::rangeOpacitiesChanged, controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::rangeIconsChanged,     controller, &ValuesWidgetController::_saveSettings);
}

InstrumentValue* ValuesWidgetController::_createNewInstrumentValueWorker(Vehicle* activeVehicle, InstrumentValue::FontSize fontSize, QmlObjectListModel* rowModel)
{
    InstrumentValue* newValue = new InstrumentValue(activeVehicle, fontSize, rowModel);
    _connectSignalsToController(newValue, this);
    return newValue;
}

InstrumentValue* ValuesWidgetController::appendColumn(int rowIndex)
{
    InstrumentValue* newValue = nullptr;

    if (rowIndex >= 0 && rowIndex < _valuesModel->count()) {
        QmlObjectListModel* row = _valuesModel->value<QmlObjectListModel*>(rowIndex);
        InstrumentValue::FontSize fontSize = InstrumentValue::DefaultFontSize;
        if (row->count()) {
            fontSize = row->value<InstrumentValue*>(0)->fontSize();
        }
        newValue  = _createNewInstrumentValueWorker(_currentActiveVehicle(), fontSize, row);
        row->append(newValue);
    }
    _saveSettings();

    return newValue;
}

void ValuesWidgetController::deleteLastColumn(int rowIndex)
{
    if (rowIndex >= 0 && rowIndex < _valuesModel->count()) {
        QmlObjectListModel* row = _valuesModel->value<QmlObjectListModel*>(rowIndex);

        if (rowIndex != 0 || row->count() > 1) {
            row->removeAt(row->count() - 1);
        }
        if (row->count() == 0) {
            // Last column was deleted, delete the row as well
            _valuesModel->removeAt(rowIndex);
        }
    }
    _saveSettings();
}

QmlObjectListModel* ValuesWidgetController::appendRow(bool addBlanksColumn)
{
    QmlObjectListModel* newRow = new QmlObjectListModel(_valuesModel);
    _valuesModel->append(newRow);
    int rowIndex = _valuesModel->count() - 1;
    if (addBlanksColumn) {
        appendColumn(rowIndex);
    }
    _saveSettings();
    return newRow;
}

QmlObjectListModel* ValuesWidgetController::insertRow(int atIndex, bool addBlanksColumn)
{
    QmlObjectListModel* newRow = nullptr;

    if (atIndex >= 0 && atIndex < _valuesModel->count() + 1) {
        QmlObjectListModel* newRow = new QmlObjectListModel(_valuesModel);
        _valuesModel->insert(atIndex, newRow);
        if (addBlanksColumn) {
            appendColumn(atIndex);
        }
        _saveSettings();
    }
    return newRow;
}

void ValuesWidgetController::deleteRow(int rowIndex)
{
    if (rowIndex >= 0 && rowIndex < _valuesModel->count()) {
        if (_valuesModel->count() > 1) {
            _valuesModel->removeAt(rowIndex);
        }
        _saveSettings();
    }
}

Vehicle* ValuesWidgetController::_currentActiveVehicle(void)
{
    Vehicle* activeVehicle = _multiVehicleMgr->activeVehicle();
    if (!activeVehicle) {
        activeVehicle = _multiVehicleMgr->offlineEditingVehicle();
    }
    return activeVehicle;
}

void ValuesWidgetController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (!activeVehicle) {
        activeVehicle = _currentActiveVehicle();
    }

    for (int rowIndex=0; rowIndex<_valuesModel->count(); rowIndex++) {
        QmlObjectListModel* rowModel = _valuesModel->value<QmlObjectListModel*>(rowIndex);
        for (int colIndex=0; colIndex<rowModel->count(); colIndex++) {
            rowModel->value<InstrumentValue*>(colIndex)->activeVehicleChanged(activeVehicle);
        }
    }
}

bool ValuesWidgetController::_validRowIndex(int rowIndex)
{
    return rowIndex >= 0 && rowIndex < _valuesModel->count();
}


int ValuesWidgetController::fontSizeForRow(int rowIndex)
{
    return _validRowIndex(rowIndex) ? _rgFontSizeByRow[rowIndex].toInt() : _rgFontSizeByRow[0].toInt();
}

void ValuesWidgetController::setFontSizeForRow(int rowIndex, int fontSize)
{
    if (_validRowIndex(rowIndex)) {
        _rgFontSizeByRow[rowIndex] = fontSize;
    }
}

void ValuesWidgetController::_saveSettings(void)
{
    if (_preventSaveSettings) {
        return;
    }

    QSettings settings;

    settings.beginGroup(_groupKey);
    settings.remove(QStringLiteral(""));
    settings.beginWriteArray(_rowsKey);

    for (int rowIndex=0; rowIndex<_valuesModel->count(); rowIndex++) {
        QmlObjectListModel* colValuesModel = _valuesModel->value<QmlObjectListModel*>(rowIndex);

        settings.setArrayIndex(rowIndex);
        settings.beginWriteArray(_columnsKey);

        for (int colIndex=0; colIndex<colValuesModel->count(); colIndex++) {
            settings.setArrayIndex(colIndex);
            colValuesModel->value<InstrumentValue*>(colIndex)->saveToSettings(settings);
        }

        settings.endArray();
    }

    settings.endArray();
}


void ValuesWidgetController::_loadSettings(void)
{
    QSettings settings;

    _valuesModel->deleteLater();
    _valuesModel = new QmlObjectListModel(this);
    emit valuesModelChanged(_valuesModel);

    if (settings.childGroups().contains(_deprecatedGroupKey)) {
        settings.beginGroup(_deprecatedGroupKey);

        QStringList largeValues = settings.value(_deprecatedLargeValuesKey).toStringList();
        QStringList smallValues = settings.value(_deprecatedSmallValuesKey).toStringList();
        QStringList altitudeProperties = { "AltitudeRelative" , "AltitudeAMSL" };

        int rowIndex = -1;
        int valueCount = 0;
        QmlObjectListModel* rowModel = nullptr;
        for (const QString& largeValue: largeValues) {
            QStringList parts =     largeValue.split(".");
            QString factGroupName = _pascalCase(parts[0]);
            QString factName =      _pascalCase(parts[1]);

            rowModel = appendRow(false /* addBlankColumn */);
            rowIndex++;

            InstrumentValue* colValue = appendColumn(rowIndex);
            colValue->setFact(factGroupName, factName);
            colValue->setText(colValue->fact()->shortDescription());
            colValue->setShowUnits(true);
            colValue->setFontSize(altitudeProperties.contains(factName) ? InstrumentValue::LargeFontSize : InstrumentValue::DefaultFontSize);
        }

        valueCount = 0;
        rowModel = nullptr;
        for (const QString& smallValue: smallValues) {
            QStringList parts =     smallValue.split(".");
            QString factGroupName = _pascalCase(parts[0]);
            QString factName =      _pascalCase(parts[1]);

            if (!(valueCount++ & 1)) {
                rowModel = appendRow(false /* addBlankColumn */);
                rowIndex++;
            }

            InstrumentValue* colValue = appendColumn(rowIndex);
            colValue->setFact(factGroupName, factName);
            colValue->setText(colValue->fact()->shortDescription());
            colValue->setShowUnits(true);
            colValue->setFontSize(InstrumentValue::SmallFontSize);
        }

        settings.endGroup();
        settings.remove(_deprecatedGroupKey);
    } else {
        _preventSaveSettings = true;

        settings.beginGroup(_groupKey);
        int cRows = settings.beginReadArray(_rowsKey);

        for (int rowIndex=0; rowIndex<cRows; rowIndex++) {
            appendRow(false /* addBlankColumns */);

            settings.setArrayIndex(rowIndex);
            int cCols = settings.beginReadArray(_columnsKey);

            for (int colIndex=0; colIndex<cCols; colIndex++) {
                settings.setArrayIndex(colIndex);
                appendColumn(rowIndex)->readFromSettings(settings);
            }

            settings.endArray();
        }

        settings.endArray();

        _preventSaveSettings = false;
    }

    // Use defaults if nothing there
    if (_valuesModel->count() == 0) {
        _valuesModel->deleteLater();
        _valuesModel = qgcApp()->toolbox()->corePlugin()->valuesWidgetDefaultSettings(this);
        emit valuesModelChanged(_valuesModel);
    }
}

void ValuesWidgetController::resetToDefaults(void)
{
    QSettings settings;

    settings.beginGroup(_groupKey);
    settings.remove("");

    _loadSettings();
}

void ValuesWidgetController::setPreventSaveSettings(bool preventSaveSettings)
{
    _preventSaveSettings = preventSaveSettings;
}

void ValuesWidgetController::setValuesModelParentController(ValuesWidgetController* newParentController)
{
    _valuesModel->setParent(newParentController);

    // Signalling must be reconnected to new controller as well
    for (int rowIndex=0; rowIndex<_valuesModel->count(); rowIndex++) {
        QmlObjectListModel* rowModel = _valuesModel->value<QmlObjectListModel*>(rowIndex);
        for (int colIndex=0; colIndex<rowModel->count(); colIndex++) {
            _connectSignalsToController(rowModel->value<InstrumentValue*>(colIndex), newParentController);
        }
    }
}

QString ValuesWidgetController::_pascalCase(const QString& text)
{
    return text[0].toUpper() + text.right(text.length() - 1);
}
