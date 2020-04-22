/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "InstrumentValueData.h"
#include "InstrumentValueArea.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"

#include <QSettings>

const char*  InstrumentValueData::_vehicleFactGroupName =   "Vehicle";

// Important: The indices of these strings must match the InstrumentValueData::RangeType enum
const QStringList InstrumentValueData::_rangeTypeNames = {
    QT_TRANSLATE_NOOP("InstrumentValue", "None"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Color"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Opacity"),
    QT_TRANSLATE_NOOP("InstrumentValue", "Icon"),
};

InstrumentValueData::InstrumentValueData(InstrumentValueArea* instrumentValueArea, QObject *parent)
    : QObject               (parent)
    , _instrumentValueArea  (instrumentValueArea)
{
    MultiVehicleManager* multiVehicleManager = qgcApp()->toolbox()->multiVehicleManager();
    connect(multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this, &InstrumentValueData::_activeVehicleChanged);
    _activeVehicleChanged(multiVehicleManager->activeVehicle());

    connect(this, &InstrumentValueData::rangeTypeChanged,       this, &InstrumentValueData::_resetRangeInfo);
    connect(this, &InstrumentValueData::rangeTypeChanged,       this, &InstrumentValueData::_updateRanges);
    connect(this, &InstrumentValueData::rangeValuesChanged,     this, &InstrumentValueData::_updateRanges);
    connect(this, &InstrumentValueData::rangeColorsChanged,     this, &InstrumentValueData::_updateRanges);
    connect(this, &InstrumentValueData::rangeOpacitiesChanged,  this, &InstrumentValueData::_updateRanges);
    connect(this, &InstrumentValueData::rangeIconsChanged,      this, &InstrumentValueData::_updateRanges);
}

void InstrumentValueData::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (!activeVehicle) {
        activeVehicle = qgcApp()->toolbox()->multiVehicleManager()->offlineEditingVehicle();
    }

    if (_fact) {
        disconnect(_fact, &Fact::rawValueChanged, this, &InstrumentValueData::_updateColor);
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

        connect(_fact, &Fact::rawValueChanged, this, &InstrumentValueData::_updateRanges);
    }

    _updateRanges();
}

void InstrumentValueData::clearFact(void)
{
    _fact = nullptr;
    _factName.clear();
    _factGroupName.clear();
    _factValueNames.clear();
    _text.clear();
    _icon.clear();
    _showUnits = true;

    emit factValueNamesChanged  (_factValueNames);
    emit factChanged            (_fact);
    emit factNameChanged        (_factName);
    emit factGroupNameChanged   (_factGroupName);
    emit textChanged            (_text);
    emit iconChanged            (_icon);
    emit showUnitsChanged       (_showUnits);
}

void InstrumentValueData::setFact(const QString& factGroupName, const QString& factName)
{
    if (_fact) {
        disconnect(_fact, &Fact::rawValueChanged, this, &InstrumentValueData::_updateRanges);
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

        connect(_fact, &Fact::rawValueChanged, this, &InstrumentValueData::_updateRanges);
    } else {
        _factName.clear();
        _factGroupName.clear();
    }

    emit factValueNamesChanged  (_factValueNames);
    emit factChanged            (_fact);
    emit factNameChanged        (_factName);
    emit factGroupNameChanged   (_factGroupName);

    _updateRanges();
}

void InstrumentValueData::setText(const QString& text)
{
    if (text != _text) {
        _text = text;
        emit textChanged(text);
    }
}

void InstrumentValueData::setShowUnits(bool showUnits)
{
    if (showUnits != _showUnits) {
        _showUnits = showUnits;
        emit showUnitsChanged(showUnits);
    }
}

void InstrumentValueData::setIcon(const QString& icon)
{
    if (icon != _icon) {
        _icon = icon;
        emit iconChanged(_icon);
    }
}

void InstrumentValueData::setRangeType(RangeType rangeType)
{
    if (rangeType != _rangeType) {
        _rangeType = rangeType;
        emit rangeTypeChanged(rangeType);
    }
}

void InstrumentValueData::setRangeValues(const QVariantList& rangeValues)
{
    _rangeValues = rangeValues;
    emit rangeValuesChanged(rangeValues);
}

void InstrumentValueData::setRangeColors (const QVariantList& rangeColors)
{
    _rangeColors = rangeColors;
    emit rangeColorsChanged(rangeColors);
}

void InstrumentValueData::setRangeIcons(const QVariantList& rangeIcons)
{
    _rangeIcons = rangeIcons;
    emit rangeIconsChanged(rangeIcons);
}

void InstrumentValueData::setRangeOpacities(const QVariantList& rangeOpacities)
{
    _rangeOpacities = rangeOpacities;
    emit rangeOpacitiesChanged(rangeOpacities);
}

void InstrumentValueData::_resetRangeInfo(void)
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
            _rangeIcons.append(_instrumentValueArea->iconNames()[0]);
            break;
        }
    }

    emit rangeValuesChanged     (_rangeValues);
    emit rangeColorsChanged     (_rangeColors);
    emit rangeOpacitiesChanged  (_rangeOpacities);
    emit rangeIconsChanged      (_rangeIcons);
}

void InstrumentValueData::addRangeValue(void)
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
        _rangeIcons.append(_instrumentValueArea->iconNames()[0]);
        break;
    }

    emit rangeValuesChanged     (_rangeValues);
    emit rangeColorsChanged     (_rangeColors);
    emit rangeOpacitiesChanged  (_rangeOpacities);
    emit rangeIconsChanged      (_rangeIcons);
}

void InstrumentValueData::removeRangeValue(int index)
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

void InstrumentValueData::_updateRanges(void)
{
    _updateColor();
    _updateIcon();
    _updateOpacity();
}

void InstrumentValueData::_updateColor(void)
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

void InstrumentValueData::_updateOpacity(void)
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

void InstrumentValueData::_updateIcon(void)
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

int InstrumentValueData::_currentRangeIndex(const QVariant& value)
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
