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
const char*  InstrumentValue::_iconKey =                "icon";
const char*  InstrumentValue::_iconPositionKey =        "iconPosition";

QStringList InstrumentValue::_iconNames;

// Important: The indices of these strings must match the InstrumentValue::IconPosition enumconst QStringList InstrumentValue::_iconPositionNames = {
const QStringList InstrumentValue::_iconPositionNames = {
    QT_TRANSLATE_NOOP("InstrumentValue", "Above"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Left"),
};

// Important: The indices of these strings must match the InstrumentValue::FontSize enum
const QStringList InstrumentValue::_fontSizeNames = {
    QT_TRANSLATE_NOOP("InstrumentValue", "Default"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Small"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Medium"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Large"),
};

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
    connect(value, &InstrumentValue::factNameChanged,        controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::factGroupNameChanged,   controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::labelChanged,           controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::fontSizeChanged,        controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::showUnitsChanged,       controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::iconChanged,            controller, &ValuesWidgetController::_saveSettings);
    connect(value, &InstrumentValue::iconPositionChanged,    controller, &ValuesWidgetController::_saveSettings);
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

InstrumentValue::InstrumentValue(Vehicle* activeVehicle, FontSize fontSize, QmlObjectListModel* rowModel)
    : QObject       (rowModel)
    , _activeVehicle(activeVehicle)
    , _rowModel     (rowModel)
    , _fontSize     (fontSize)
{
    if (_iconNames.isEmpty()) {
        QDir iconDir(":/InstrumentValueIcons/");
        _iconNames = iconDir.entryList();
    }
}

void InstrumentValue::activeVehicleChanged(Vehicle* activeVehicle)
{
    _activeVehicle = activeVehicle;

    if (_fact) {
        _fact = nullptr;

        FactGroup* factGroup = nullptr;
        if (_factGroupName == QStringLiteral("Vehicle")) {
            factGroup = _activeVehicle;
        } else {
            factGroup = _activeVehicle->getFactGroup(_factGroupName);
        }

        if (factGroup) {
            _fact = factGroup->getFact(_factName);
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
        _factName = factName;
        _factGroupName = factGroupName;
        _label = label;
    } else {
        _factName.clear();
        _factGroupName.clear();
        _label.clear();
    }

    emit labelChanged(_label);
    emit factChanged(_fact);
    emit factNameChanged(_factName);
    emit factGroupNameChanged(_factGroupName);
}

void InstrumentValue::_setFontSize(FontSize fontSize)
{
    if (fontSize != _fontSize) {
        _fontSize = fontSize;
        emit fontSizeChanged(fontSize);
    }
}

void InstrumentValue::setFontSize(FontSize fontSize)
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
    settings.setValue(_iconKey,         _icon);
    settings.setValue(_iconPositionKey, _iconPosition);
}

void InstrumentValue::readFromSettings(const QSettings& settings)
{
    _factGroupName =    settings.value(_factGroupNameKey).toString();
    _label =            settings.value(_labelKey).toString();
    _fontSize =         settings.value(_fontSizeKey, DefaultFontSize).value<FontSize>();
    _showUnits =        settings.value(_showUnitsKey, true).toBool();
    _icon =             settings.value(_iconKey).toString();
    _iconPosition =     settings.value(_iconPositionKey, IconLeft).value<IconPosition>();

    QString factName = settings.value(_factNameKey).toString();
    if (!factName.isEmpty()) {
        setFact(_factGroupName, factName, _label);
    }

    emit factChanged            (_fact);
    emit factGroupNameChanged   (_factGroupName);
    emit labelChanged           (_label);
    emit fontSizeChanged        (_fontSize);
    emit showUnitsChanged       (_showUnits);
    emit iconChanged            (_icon);
    emit iconPositionChanged    (_iconPosition);
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
    _icon.clear();
    _showUnits = true;

    emit factChanged            (_fact);
    emit factGroupNameChanged   (_factGroupName);
    emit labelChanged           (_label);
    emit iconChanged            (_icon);
    emit showUnitsChanged       (_showUnits);
}

void InstrumentValue::setIcon(const QString& icon)
{
    if (icon != _icon) {
        _icon = icon;
        emit iconChanged(_icon);
    }
}

void InstrumentValue::setIconPosition(IconPosition iconPosition)
{
    if (iconPosition != _iconPosition) {
        _iconPosition = iconPosition;
        emit iconPositionChanged(iconPosition);
    }
}
