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

const char*  InstrumentValue::_factGroupNameKey =       "groupName";
const char*  InstrumentValue::_factNameKey =             "factName";
const char*  InstrumentValue::_labelKey =               "label";
const char*  InstrumentValue::_fontSizeKey =            "fontSize";
const char*  InstrumentValue::_showUnitsKey =           "showUnits";

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

InstrumentValue* ValuesWidgetController::_createNewInstrumentValueWorker(Vehicle* activeVehicle, int fontSize, QmlObjectListModel* rowModel)
{
    InstrumentValue* newValue = new InstrumentValue(activeVehicle, fontSize, rowModel);

    connect(newValue, &InstrumentValue::factChanged,            this, &ValuesWidgetController::_saveSettings);
    connect(newValue, &InstrumentValue::factGroupNameChanged,   this, &ValuesWidgetController::_saveSettings);
    connect(newValue, &InstrumentValue::labelChanged,           this, &ValuesWidgetController::_saveSettings);
    connect(newValue, &InstrumentValue::fontSizeChanged,        this, &ValuesWidgetController::_saveSettings);
    connect(newValue, &InstrumentValue::showUnitsChanged,       this, &ValuesWidgetController::_saveSettings);

    return newValue;
}


InstrumentValue* ValuesWidgetController::appendColumn(int rowIndex)
{
    InstrumentValue* newValue = nullptr;

    if (rowIndex >= 0 && rowIndex < _valuesModel->count()) {
        QmlObjectListModel* row = _valuesModel->value<QmlObjectListModel*>(rowIndex);
        int fontSize = InstrumentValue::DefaultFontSize;
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
        QStringList altitudeProperties = { "altitudeRelative" , "altitudeAMSL" };

        int rowIndex = -1;
        int valueCount = 0;
        QmlObjectListModel* rowModel = nullptr;
        for (const QString& largeValue: largeValues) {
            QStringList parts = largeValue.split(".");

            rowModel = appendRow(false /* addBlankColumn */);
            rowIndex++;

            InstrumentValue* colValue = appendColumn(rowIndex);
            colValue->setFact(parts[0], parts[1], QString());
            colValue->setLabel(colValue->fact()->shortDescription());
            colValue->setShowUnits(true);
            colValue->setFontSize(altitudeProperties.contains(parts[1]) ? InstrumentValue::LargeFontSize : InstrumentValue::DefaultFontSize);
        }

        valueCount = 0;
        rowModel = nullptr;
        for (const QString& smallValue: smallValues) {
            QStringList parts = smallValue.split(".");

            if (!(valueCount++ & 1)) {
                rowModel = appendRow(false /* addBlankColumn */);
                rowIndex++;
            }

            InstrumentValue* colValue = appendColumn(rowIndex);
            colValue->setFact(parts[0], parts[1], QString());
            colValue->setLabel(colValue->fact()->shortDescription());
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

InstrumentValue::InstrumentValue(Vehicle* activeVehicle, int fontSize, QmlObjectListModel* rowModel)
    : QObject       (rowModel)
    , _activeVehicle(activeVehicle)
    , _rowModel     (rowModel)
    , _fontSize     (fontSize)
{

}

void InstrumentValue::activeVehicleChanged(Vehicle* activeVehicle)
{
    _activeVehicle = activeVehicle;

    if (_fact) {
        _fact = nullptr;

        FactGroup* factGroup = nullptr;
        QString factName;
        if (_factGroupName == QStringLiteral("Vehicle")) {
            factGroup = _activeVehicle;
        } else {
            factGroup = _activeVehicle->getFactGroup(_factGroupName);
        }

        if (factGroup) {
            _fact = factGroup->getFact(factName);
        }
        emit factChanged(_fact);
    }
}

void InstrumentValue::setFact(QString factGroupName, QString factName, QString label)
{
    if (_fact) {
        _fact = nullptr;
    }

    FactGroup* factGroup = nullptr;
    if (factGroupName == QStringLiteral("Vehicle")) {
        factGroup = _activeVehicle;
    } else {
        factGroup = _activeVehicle->getFactGroup(factGroupName);
    }

    if (factGroup) {
        _fact = factGroup->getFact(factName);
    }

    if (_fact) {
        _factGroupName = factGroupName;
        _label = label;
    } else {
        _factGroupName.clear();
        _label.clear();
    }

    emit labelChanged(_label);
    emit factChanged(_fact);
    emit factGroupNameChanged(_factGroupName);
}

void InstrumentValue::_setFontSize(int fontSize)
{
    if (fontSize != _fontSize) {
        _fontSize = fontSize;
        emit fontSizeChanged(fontSize);
    }
}

void InstrumentValue::setFontSize(int fontSize)
{
    _setFontSize(fontSize);

    // All other items in row must change to match
    for (int i=0; i<_rowModel->count(); i++) {
        InstrumentValue* instrumentValue = _rowModel->value<InstrumentValue*>(i);
        if (instrumentValue != this) {
            instrumentValue->_setFontSize(fontSize);
        }
    }
}

void InstrumentValue::saveToSettings(QSettings& settings) const
{
    if (_fact) {
        settings.setValue(_factGroupNameKey,    _factGroupName);
        settings.setValue(_factNameKey,         _fact->name());
    } else {
        settings.setValue(_factGroupNameKey,    "");
        settings.setValue(_factNameKey,         "");
    }
    settings.setValue(_labelKey,        _label);
    settings.setValue(_fontSizeKey,     _fontSize);
    settings.setValue(_showUnitsKey,    _showUnits);
}

void InstrumentValue::readFromSettings(const QSettings& settings)
{
    _factGroupName =    settings.value(_factGroupNameKey).toString();
    _label =            settings.value(_labelKey).toString();
    _fontSize =         settings.value(_fontSizeKey).toInt();
    _showUnits =        settings.value(_showUnitsKey).toBool();

    QString factName = settings.value(_factNameKey).toString();
    if (!factName.isEmpty()) {
        setFact(_factGroupName, factName, _label);
    }

    emit factChanged            (_fact);
    emit factGroupNameChanged   (_factGroupName);
    emit labelChanged           (_label);
    emit fontSizeChanged        (_fontSize);
    emit showUnitsChanged       (_showUnits);
}

void InstrumentValue::setLabel(const QString& label)
{
    if (label != _label) {
        _label = label;
        emit labelChanged(label);
    }
}

void InstrumentValue::setShowUnits(bool showUnits)
{
    if (showUnits != _showUnits) {
        _showUnits = showUnits;
        emit showUnitsChanged(showUnits);
    }
}

void InstrumentValue::clearFact(void)
{
    _fact = nullptr;
    _factGroupName.clear();
    _label.clear();
    _showUnits = true;

    emit factChanged            (_fact);
    emit factGroupNameChanged   (_factGroupName);
    emit labelChanged           (_label);
    emit showUnitsChanged       (_showUnits);
}
