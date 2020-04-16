/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "InstrumentValue.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QSettings>

const char*  InstrumentValue::_versionKey =             "version";
const char*  InstrumentValue::_factGroupNameKey =       "groupName";
const char*  InstrumentValue::_factNameKey =            "factName";
const char*  InstrumentValue::_textKey =                "text";
const char*  InstrumentValue::_fontSizeKey =            "fontSize";
const char*  InstrumentValue::_showUnitsKey =           "showUnits";
const char*  InstrumentValue::_iconKey =                "icon";
const char*  InstrumentValue::_labelPositionKey =       "labelPosition";
const char*  InstrumentValue::_rangeTypeKey =           "rangeType";
const char*  InstrumentValue::_rangeValuesKey =         "rangeValues";
const char*  InstrumentValue::_rangeColorsKey =         "rangeColors";
const char*  InstrumentValue::_rangeIconsKey =          "rangeIcons";
const char*  InstrumentValue::_rangeOpacitiesKey =      "rangeOpacities";
const char*  InstrumentValue::_vehicleFactGroupName =   "Vehicle";

QStringList InstrumentValue::_iconNames;

// Important: The indices of these strings must match the InstrumentValue::LabelPosition enumconst QStringList InstrumentValue::_labelPositionNames
const QStringList InstrumentValue::_labelPositionNames = {
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

// Important: The indices of these strings must match the InstrumentValue::RangeType enum
const QStringList InstrumentValue::_rangeTypeNames = {
    QT_TRANSLATE_NOOP("InstrumentValue", "None"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Color"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Opacity"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Icon"),
};

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

    activeVehicleChanged(_activeVehicle);

    connect(this, &InstrumentValue::rangeTypeChanged,       this, &InstrumentValue::_resetRangeInfo);
    connect(this, &InstrumentValue::rangeTypeChanged,       this, &InstrumentValue::_updateRanges);
    connect(this, &InstrumentValue::rangeValuesChanged,     this, &InstrumentValue::_updateRanges);
    connect(this, &InstrumentValue::rangeColorsChanged,     this, &InstrumentValue::_updateRanges);
    connect(this, &InstrumentValue::rangeOpacitiesChanged,  this, &InstrumentValue::_updateRanges);
    connect(this, &InstrumentValue::rangeIconsChanged,      this, &InstrumentValue::_updateRanges);
}

void InstrumentValue::activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_fact) {
        disconnect(_fact, &Fact::rawValueChanged, this, &InstrumentValue::_updateColor);
    }

    _activeVehicle = activeVehicle;

    _factGroupNames.clear();
    _factGroupNames = _activeVehicle->factGroupNames();
    for (QString& name: _factGroupNames) {
        name[0] = name[0].toUpper();
    }
    _factGroupNames.prepend(_vehicleFactGroupName);
    emit factGroupNamesChanged(_factGroupNames);

    if (_fact) {
        _fact = nullptr;

        FactGroup* factGroup = nullptr;
        if (_factGroupName == _vehicleFactGroupName) {
            factGroup = _activeVehicle;
        } else {
            factGroup = _activeVehicle->getFactGroup(_factGroupName);
        }

        if (factGroup) {
            _fact = factGroup->getFact(_factName);
        }
        emit factChanged(_fact);

        connect(_fact, &Fact::rawValueChanged, this, &InstrumentValue::_updateRanges);
    }

    _updateRanges();
}

void InstrumentValue::setFact(const QString& factGroupName, const QString& factName)
{
    if (_fact) {
        disconnect(_fact, &Fact::rawValueChanged, this, &InstrumentValue::_updateRanges);
        _fact = nullptr;
    }

    FactGroup* factGroup = nullptr;
    if (factGroupName == _vehicleFactGroupName) {
        factGroup = _activeVehicle;
    } else {
        factGroup = _activeVehicle->getFactGroup(factGroupName);
    }

    _factValueNames.clear();
    _factValueNames = factGroup->factNames();
    for (QString& name: _factValueNames) {
        name[0] = name[0].toUpper();
    }

    QString nonEmptyFactName;
    if (factGroup) {
        if (factName.isEmpty()) {
            nonEmptyFactName = _factValueNames[0];
        } else {
            nonEmptyFactName = factName;
        }
        _fact = factGroup->getFact(nonEmptyFactName);
    }

    if (_fact) {
        _factGroupName = factGroupName;
        _factName =      nonEmptyFactName;

        connect(_fact, &Fact::rawValueChanged, this, &InstrumentValue::_updateRanges);
    } else {
        _factName.clear();
        _factGroupName.clear();
    }

    emit factChanged            (_fact);
    emit factNameChanged        (_factName);
    emit factGroupNameChanged   (_factGroupName);
    emit factValueNamesChanged  (_factValueNames);

    _updateRanges();
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
    settings.setValue(_versionKey,          1);
    settings.setValue(_textKey,             _text);
    settings.setValue(_fontSizeKey,         _fontSize);
    settings.setValue(_showUnitsKey,        _showUnits);
    settings.setValue(_iconKey,             _icon);
    settings.setValue(_labelPositionKey,    _labelPosition);
    settings.setValue(_rangeTypeKey,        _rangeType);

    if (_rangeType != NoRangeInfo) {
        settings.setValue(_rangeValuesKey, _rangeValues);
    }

    switch (_rangeType) {
    case NoRangeInfo:
        break;
    case ColorRange:
        settings.setValue(_rangeColorsKey,      _rangeColors);
        break;
    case OpacityRange:
        settings.setValue(_rangeOpacitiesKey,   _rangeOpacities);
        break;
    case IconSelectRange:
        settings.setValue(_rangeIconsKey,       _rangeIcons);
        break;
    }

    if (_fact) {
        settings.setValue(_factGroupNameKey,    _factGroupName);
        settings.setValue(_factNameKey,         _factName);
    } else {
        settings.setValue(_factGroupNameKey,    "");
        settings.setValue(_factNameKey,         "");
    }
}

void InstrumentValue::readFromSettings(const QSettings& settings)
{
    _factGroupName =    settings.value(_factGroupNameKey,   QString()).toString();
    _text =             settings.value(_textKey,            QString()).toString();
    _fontSize =         settings.value(_fontSizeKey,        DefaultFontSize).value<FontSize>();
    _showUnits =        settings.value(_showUnitsKey,       true).toBool();
    _icon =             settings.value(_iconKey,            QString()).toString();
    _labelPosition =     settings.value(_labelPositionKey,  LabelLeft).value<LabelPosition>();
    _rangeType =        settings.value(_rangeTypeKey,       NoRangeInfo).value<RangeType>();

    // Do this now, since the signal will cause _resetRangeInfo to be called trashing values
    emit rangeTypeChanged(_rangeType);

    _rangeValues.clear();
    _rangeColors.clear();
    _rangeOpacities.clear();
    _rangeIcons.clear();
    if (_rangeType != NoRangeInfo) {
        _rangeValues = settings.value(_rangeValuesKey).value<QVariantList>();
    }
    switch (_rangeType) {
    case NoRangeInfo:
        break;
    case ColorRange:
        _rangeColors = settings.value(_rangeColorsKey).value<QVariantList>();
        break;
    case OpacityRange:
        _rangeOpacities = settings.value(_rangeOpacitiesKey).value<QVariantList>();
        break;
    case IconSelectRange:
        _rangeIcons = settings.value(_rangeIconsKey).value<QVariantList>();
        break;
    }

    QString factName = settings.value(_factNameKey).toString();
    if (!factName.isEmpty()) {
        setFact(_factGroupName, factName);
    }

    emit factChanged            (_fact);
    emit factGroupNameChanged   (_factGroupName);
    emit textChanged            (_text);
    emit fontSizeChanged        (_fontSize);
    emit showUnitsChanged       (_showUnits);
    emit iconChanged            (_icon);
    emit labelPositionChanged   (_labelPosition);
    emit rangeValuesChanged     (_rangeValues);
    emit rangeColorsChanged     (_rangeColors);
    emit rangeOpacitiesChanged  (_rangeOpacities);
    emit rangeIconsChanged      (_rangeIcons);
}

void InstrumentValue::setText(const QString& text)
{
    if (text != _text) {
        _text = text;
        emit textChanged(text);
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
    _text.clear();
    _icon.clear();
    _showUnits = true;

    emit factChanged            (_fact);
    emit factGroupNameChanged   (_factGroupName);
    emit textChanged            (_text);
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

void InstrumentValue::setLabelPosition(LabelPosition labelPosition)
{
    if (labelPosition != _labelPosition) {
        _labelPosition = labelPosition;
        emit labelPositionChanged(labelPosition);
    }
}

void InstrumentValue::setRangeType(RangeType rangeType)
{
    if (rangeType != _rangeType) {
        _rangeType = rangeType;
        emit rangeTypeChanged(rangeType);
    }
}

void InstrumentValue::setRangeValues(const QVariantList& rangeValues)
{
    _rangeValues = rangeValues;
    emit rangeValuesChanged(rangeValues);
}

void InstrumentValue::setRangeColors (const QVariantList& rangeColors)
{
    _rangeColors = rangeColors;
    emit rangeColorsChanged(rangeColors);
}

void InstrumentValue::setRangeIcons(const QVariantList& rangeIcons)
{
    _rangeIcons = rangeIcons;
    emit rangeIconsChanged(rangeIcons);
}

void InstrumentValue::setRangeOpacities(const QVariantList& rangeOpacities)
{
    _rangeOpacities = rangeOpacities;
    emit rangeOpacitiesChanged(rangeOpacities);
}

void InstrumentValue::_resetRangeInfo(void)
{
    _rangeValues.clear();
    _rangeColors.clear();
    _rangeOpacities.clear();
    _rangeIcons.clear();

    if (_rangeType != NoRangeInfo) {
        _rangeValues = { 0.0, 100.0 };
    }
    for (int i=0; i<_rangeValues.count() + 1; i++) {
        switch (_rangeType) {
        case NoRangeInfo:
            break;
        case ColorRange:
            _rangeColors.append(QColor("green"));
            break;
        case OpacityRange:
            _rangeOpacities.append(1.0);
            break;
        case IconSelectRange:
            _rangeIcons.append(_iconNames[0]);
            break;
        }
    }

    emit rangeValuesChanged     (_rangeValues);
    emit rangeColorsChanged     (_rangeColors);
    emit rangeOpacitiesChanged  (_rangeOpacities);
    emit rangeIconsChanged      (_rangeIcons);
}

void InstrumentValue::addRangeValue(void)
{
    _rangeValues.append(_rangeValues.last().toDouble() + 1);

    switch (_rangeType) {
    case NoRangeInfo:
        break;
    case ColorRange:
        _rangeColors.append(QColor("green"));
        break;
    case OpacityRange:
        _rangeOpacities.append(1.0);
        break;
    case IconSelectRange:
        _rangeIcons.append(_iconNames[0]);
        break;
    }

    emit rangeValuesChanged     (_rangeValues);
    emit rangeColorsChanged     (_rangeColors);
    emit rangeOpacitiesChanged  (_rangeOpacities);
    emit rangeIconsChanged      (_rangeIcons);
}

void InstrumentValue::removeRangeValue(int index)
{
    if (_rangeValues.count() < 2 || index <0 || index >= _rangeValues.count()) {
        return;
    }

    _rangeValues.removeAt(index);

    switch (_rangeType) {
    case NoRangeInfo:
        break;
    case ColorRange:
        _rangeColors.removeAt(index + 1);
        break;
    case OpacityRange:
        _rangeOpacities.removeAt(index + 1);
        break;
    case IconSelectRange:
        _rangeIcons.removeAt(index + 1);
        break;
    }

    emit rangeValuesChanged     (_rangeValues);
    emit rangeColorsChanged     (_rangeColors);
    emit rangeOpacitiesChanged  (_rangeOpacities);
    emit rangeIconsChanged      (_rangeIcons);
}

void InstrumentValue::_updateRanges(void)
{
    _updateColor();
    _updateIcon();
    _updateOpacity();
}

void InstrumentValue::_updateColor(void)
{
    QColor newColor;

    int rangeIndex = -1;

    if (_rangeType == ColorRange && _fact) {
        rangeIndex =_currentRangeIndex(_fact->rawValue().toDouble());
    }
    if (rangeIndex != -1) {
        newColor = _rangeColors[rangeIndex].value<QColor>();
    }

    if (newColor != _currentColor) {
        _currentColor = newColor;
        emit currentColorChanged(_currentColor);
    }
}

void InstrumentValue::_updateOpacity(void)
{
    double newOpacity = 1.0;

    int rangeIndex = -1;

    if (_rangeType == OpacityRange && _fact) {
        rangeIndex =_currentRangeIndex(_fact->rawValue().toDouble());
    }
    if (rangeIndex != -1) {
        newOpacity = _rangeOpacities[rangeIndex].toDouble();
    }

    if (!qFuzzyCompare(newOpacity, _currentOpacity)) {
        _currentOpacity = newOpacity;
        emit currentOpacityChanged(newOpacity);
    }
}

void InstrumentValue::_updateIcon(void)
{
    QString newIcon;

    int rangeIndex = -1;

    if (_rangeType == IconSelectRange && _fact) {
        rangeIndex =_currentRangeIndex(_fact->rawValue().toDouble());
    }
    if (rangeIndex != -1) {
        newIcon = _rangeIcons[rangeIndex].toString();
    }

    if (newIcon != _currentIcon) {
        _currentIcon = newIcon;
        emit currentIconChanged(newIcon);
    }
}

int InstrumentValue::_currentRangeIndex(const QVariant& value)
{
    if (qIsNaN(value.toDouble())) {
        return 0;
    }
    for (int i=0; i<_rangeValues.count(); i++) {
        if (value.toDouble() <= _rangeValues[i].toDouble()) {
            return i;
        }
    }
    return _rangeValues.count();
}
