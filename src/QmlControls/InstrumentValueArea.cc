/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "InstrumentValueArea.h"
#include "InstrumentValueData.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QSettings>

const QString InstrumentValueArea::_valuePageUserSettingsGroup      ("ValuePage.userSettings");
const QString InstrumentValueArea::valuePageDefaultSettingsGroup    ("ValuePage.defaultSettings");

const char* InstrumentValueArea::_rowsKey =             "rows";
const char* InstrumentValueArea::_columnsKey =          "columns";
const char* InstrumentValueArea::_fontSizeKey =         "fontSize";
const char* InstrumentValueArea::_orientationKey =      "orientation";
const char* InstrumentValueArea::_versionKey =          "version";
const char* InstrumentValueArea::_factGroupNameKey =    "groupName";
const char* InstrumentValueArea::_factNameKey =         "factName";
const char* InstrumentValueArea::_textKey =             "text";
const char* InstrumentValueArea::_showUnitsKey =        "showUnits";
const char* InstrumentValueArea::_iconKey =             "icon";
const char* InstrumentValueArea::_rangeTypeKey =        "rangeType";
const char* InstrumentValueArea::_rangeValuesKey =      "rangeValues";
const char* InstrumentValueArea::_rangeColorsKey =      "rangeColors";
const char* InstrumentValueArea::_rangeIconsKey =       "rangeIcons";
const char* InstrumentValueArea::_rangeOpacitiesKey =   "rangeOpacities";

const char* InstrumentValueArea::_deprecatedGroupKey =  "ValuesWidget";

QStringList InstrumentValueArea::_iconNames;

// Important: The indices of these strings must match the InstrumentValueArea::FontSize enum
const QStringList InstrumentValueArea::_fontSizeNames = {
    QT_TRANSLATE_NOOP("InstrumentValueArea", "Default"),
    QT_TRANSLATE_NOOP("InstrumentValueArea", "Small"),
    QT_TRANSLATE_NOOP("InstrumentValueArea", "Medium"),
    QT_TRANSLATE_NOOP("InstrumentValueArea", "Large"),
};

InstrumentValueArea::InstrumentValueArea(QQuickItem* parent)
    : QQuickItem(parent)
    , _rowValues(new QmlObjectListModel(this))
{
    if (_iconNames.isEmpty()) {
        QDir iconDir(":/InstrumentValueIcons/");
        _iconNames = iconDir.entryList();
    }

    _connectSignals();
}

InstrumentValueArea::InstrumentValueArea(const QString& defaultSettingsGroup)
    : QQuickItem            (nullptr)
    , _defaultSettingsGroup (defaultSettingsGroup)
    , _rowValues            (new QmlObjectListModel(this))
{

    _connectSignals();
}

void InstrumentValueArea::_connectSignals(void)
{
    connect(this, &InstrumentValueArea::fontSizeChanged,    this, &InstrumentValueArea::_saveSettings);
    connect(this, &InstrumentValueArea::orientationChanged, this, &InstrumentValueArea::_saveSettings);
}

void InstrumentValueArea::componentComplete(void)
{
    QQuickItem::componentComplete();

    // We should know settingsGroup/defaultSettingsGroup now so we can load settings
    _loadSettings();
}

InstrumentValueData* InstrumentValueArea::_createNewInstrumentValueWorker(QmlObjectListModel* rowModel)
{
    InstrumentValueData* value = new InstrumentValueData(this, rowModel);

    connect(value, &InstrumentValueData::factNameChanged,       this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::factGroupNameChanged,  this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::textChanged,           this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::showUnitsChanged,      this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::iconChanged,           this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::rangeTypeChanged,      this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::rangeValuesChanged,    this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::rangeColorsChanged,    this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::rangeOpacitiesChanged, this, &InstrumentValueArea::_saveSettings);
    connect(value, &InstrumentValueData::rangeIconsChanged,     this, &InstrumentValueArea::_saveSettings);

    return value;
}

InstrumentValueData* InstrumentValueArea::appendColumn(int rowIndex)
{
    InstrumentValueData* newValue = nullptr;

    if (rowIndex >= 0 && rowIndex < _rowValues->count()) {
        QmlObjectListModel* row = _rowValues->value<QmlObjectListModel*>(rowIndex);
        newValue  = _createNewInstrumentValueWorker(row);
        row->append(newValue);
    }
    _saveSettings();

    return newValue;
}

void InstrumentValueArea::deleteLastColumn(int rowIndex)
{
    if (rowIndex >= 0 && rowIndex < _rowValues->count()) {
        QmlObjectListModel* row = _rowValues->value<QmlObjectListModel*>(rowIndex);

        if (rowIndex != 0 || row->count() > 1) {
            row->removeAt(row->count() - 1);
        }
        if (row->count() == 0) {
            // Last column was deleted, delete the row as well
            _rowValues->removeAt(rowIndex);
        }
    }
    _saveSettings();
}

QmlObjectListModel* InstrumentValueArea::appendRow(bool addBlanksColumn)
{
    QmlObjectListModel* newRow = new QmlObjectListModel(_rowValues);
    _rowValues->append(newRow);
    int rowIndex = _rowValues->count() - 1;
    if (addBlanksColumn) {
        appendColumn(rowIndex);
    }
    _saveSettings();
    return newRow;
}

QmlObjectListModel* InstrumentValueArea::insertRow(int atIndex, bool addBlanksColumn)
{
    QmlObjectListModel* newRow = nullptr;

    if (atIndex >= 0 && atIndex < _rowValues->count() + 1) {
        QmlObjectListModel* newRow = new QmlObjectListModel(_rowValues);
        _rowValues->insert(atIndex, newRow);
        if (addBlanksColumn) {
            appendColumn(atIndex);
        }
        _saveSettings();
    }
    return newRow;
}

void InstrumentValueArea::deleteRow(int rowIndex)
{
    if (rowIndex >= 0 && rowIndex < _rowValues->count()) {
        if (_rowValues->count() > 1) {
            _rowValues->removeAt(rowIndex);
        }
        _saveSettings();
    }
}

bool InstrumentValueArea::_validRowIndex(int rowIndex)
{
    return rowIndex >= 0 && rowIndex < _rowValues->count();
}

void InstrumentValueArea::_checkForDeprecatedSettings(void)
{
    QSettings settings;

    if (settings.childGroups().contains(_deprecatedGroupKey)) {
        settings.remove(_deprecatedGroupKey);
        qgcApp()->showAppMessage(tr("The support for custom instrument value display has changed. The display will be reset to the new default setup. You will need to modify it again to suit your needs."), tr("Instrument Values"));
    }
}

void InstrumentValueArea::_saveSettings(void)
{
    if (_preventSaveSettings) {
        return;
    }

    QSettings settings;

    if (_userSettingsGroup.isEmpty()) {
        // This means we are setting up default settings
        settings.beginGroup(_defaultSettingsGroup);
    } else {
        // This means we are saving user modifications
        settings.remove(_defaultSettingsGroup);
        settings.beginGroup(_userSettingsGroup);
    }

    settings.remove(""); // Remove any previous settings

    settings.setValue(_versionKey,      1);
    settings.setValue(_fontSizeKey,     _fontSize);
    settings.setValue(_orientationKey,  _orientation);

    settings.beginWriteArray(_rowsKey);

    for (int rowIndex=0; rowIndex<_rowValues->count(); rowIndex++) {
        QmlObjectListModel* colValues = _rowValues->value<QmlObjectListModel*>(rowIndex);

        settings.setArrayIndex(rowIndex);
        settings.beginWriteArray(_columnsKey);

        for (int colIndex=0; colIndex<colValues->count(); colIndex++) {
            InstrumentValueData* value = colValues->value<InstrumentValueData*>(colIndex);

            settings.setArrayIndex(colIndex);

            settings.setValue(_textKey,         value->text());
            settings.setValue(_showUnitsKey,    value->showUnits());
            settings.setValue(_iconKey,         value->icon());
            settings.setValue(_rangeTypeKey,    value->rangeType());

            if (value->rangeType() != InstrumentValueData::NoRangeInfo) {
                settings.setValue(_rangeValuesKey, value->rangeValues());
            }

            switch (value->rangeType()) {
            case InstrumentValueData::NoRangeInfo:
                break;
            case InstrumentValueData::ColorRange:
                settings.setValue(_rangeColorsKey,      value->rangeColors());
                break;
            case InstrumentValueData::OpacityRange:
                settings.setValue(_rangeOpacitiesKey,   value->rangeOpacities());
                break;
            case InstrumentValueData::IconSelectRange:
                settings.setValue(_rangeIconsKey,       value->rangeIcons());
                break;
            }

            if (value->fact()) {
                settings.setValue(_factGroupNameKey,    value->factGroupName());
                settings.setValue(_factNameKey,         value->factName());
            } else {
                settings.setValue(_factGroupNameKey,    "");
                settings.setValue(_factNameKey,         "");
            }
        }

        settings.endArray();
    }

    settings.endArray();
}

void InstrumentValueArea::_loadSettings(void)
{
    _checkForDeprecatedSettings();

   QSettings settings;

    _rowValues->deleteLater();
    _rowValues = new QmlObjectListModel(this);
    emit rowValuesChanged(_rowValues);

    if (!settings.childGroups().contains(_userSettingsGroup)) {
        qgcApp()->toolbox()->corePlugin()->instrumentValueAreaCreateDefaultSettings(_defaultSettingsGroup);
    }

    _preventSaveSettings = true;

    if (settings.childGroups().contains(_defaultSettingsGroup)) {
        settings.beginGroup(_defaultSettingsGroup);
    } else {
        settings.beginGroup(_userSettingsGroup);
    }

    int version = settings.value(_versionKey, 0).toInt();
    if (version != 1) {
        qgcApp()->showAppMessage(tr("Load Settings"), tr("Settings version %1 for %2 is not supported. Setup will be reset to defaults.").arg(version).arg(_userSettingsGroup));
        settings.remove("");
        qgcApp()->toolbox()->corePlugin()->instrumentValueAreaCreateDefaultSettings(_defaultSettingsGroup);
    }
    _fontSize = settings.value(_fontSizeKey, DefaultFontSize).value<FontSize>();
    _orientation = settings.value(_orientationKey, VerticalOrientation).value<Orientation>();

    int cRows = settings.beginReadArray(_rowsKey);

    for (int rowIndex=0; rowIndex<cRows; rowIndex++) {
        appendRow(false /* addBlankColumns */);

        settings.setArrayIndex(rowIndex);
        int cCols = settings.beginReadArray(_columnsKey);

        for (int colIndex=0; colIndex<cCols; colIndex++) {
            settings.setArrayIndex(colIndex);

            InstrumentValueData* value = appendColumn(rowIndex);

            QString factName = settings.value(_factNameKey).toString();
            if (!factName.isEmpty()) {
                value->setFact(settings.value(_factGroupNameKey).toString(), factName);
            }

            value->setText      (settings.value(_textKey).toString());
            value->setShowUnits (settings.value(_showUnitsKey, true).toBool());
            value->setIcon      (settings.value(_iconKey).toString());
            value->setRangeType (settings.value(_rangeTypeKey, InstrumentValueData::NoRangeInfo).value<InstrumentValueData::RangeType>());

            if (value->rangeType() != InstrumentValueData::NoRangeInfo) {
                value->setRangeValues(settings.value(_rangeValuesKey).value<QVariantList>());
            }
            switch (value->rangeType()) {
            case InstrumentValueData::NoRangeInfo:
                break;
            case InstrumentValueData::ColorRange:
                value->setRangeColors(settings.value(_rangeColorsKey).value<QVariantList>());
                break;
            case InstrumentValueData::OpacityRange:
                value->setRangeOpacities(settings.value(_rangeOpacitiesKey).value<QVariantList>());
                break;
            case InstrumentValueData::IconSelectRange:
                value->setRangeIcons(settings.value(_rangeIconsKey).value<QVariantList>());
                break;
            }
        }

        settings.endArray();
    }

    settings.endArray();

    _preventSaveSettings = false;

    // Use defaults if nothing there
    if (_rowValues->count() == 0) {
        _rowValues->deleteLater();
        emit rowValuesChanged(_rowValues);
    }
}

void InstrumentValueArea::resetToDefaults(void)
{
    QSettings settings;

    settings.remove(_userSettingsGroup);

    _loadSettings();
}

QString InstrumentValueArea::_pascalCase(const QString& text)
{
    return text[0].toUpper() + text.right(text.length() - 1);
}

void InstrumentValueArea::setFontSize(FontSize fontSize)
{
    if (fontSize != _fontSize) {
        _fontSize = fontSize;
        emit fontSizeChanged(fontSize);
    }
}
