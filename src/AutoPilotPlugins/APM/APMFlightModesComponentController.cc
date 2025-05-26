/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMFlightModesComponentController.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"

#include <QtCore/QVariant>
#include <QtQml/QQmlEngine>

APMFlightModesComponentController::APMFlightModesComponentController(QObject *parent)
    : FactPanelController(parent)
    , _simpleModeFact(parameterExists(-1, _simpleParamName) ? getParameterFact(-1, _simpleParamName) : nullptr)
    , _superSimpleModeFact(parameterExists(-1, _superSimpleParamName) ? getParameterFact(-1, _superSimpleParamName) : nullptr)
    , _simpleModesSupported(_simpleModeFact && _superSimpleModeFact)
{
    (void) qmlRegisterUncreatableType<APMFlightModesComponentController>("QGroundControl.Controllers", 1, 0, "APMFlightModesComponentController", "Reference only");

    const bool arduRoverFirmware = parameterExists(-1, QStringLiteral("MODE1"));
    _modeParamPrefix = arduRoverFirmware ? QStringLiteral("MODE") : QStringLiteral("FLTMODE");
    _modeChannelParam = arduRoverFirmware ? QStringLiteral("MODE_CH") : QStringLiteral("FLTMODE_CH");

    for (int i = 0; i < _cFltModes; i++) {
        _simpleModeEnabled.append(QVariant(false));
        _superSimpleModeEnabled.append(QVariant(false));
    }

    if (_simpleModesSupported) {
        _setupSimpleModeEnabled();

        const uint8_t simpleModeValue = static_cast<uint8_t>(_simpleModeFact->rawValue().toUInt());
        const uint8_t superSimpleModeValue = static_cast<uint8_t>(_superSimpleModeFact->rawValue().toUInt());
        if ((simpleModeValue == 0) && (superSimpleModeValue == 0)) {
            _simpleMode = SimpleModeStandard;
        } else if ((simpleModeValue == _allSimpleBits) && (superSimpleModeValue == 0)) {
            _simpleMode = SimpleModeSimple;
        } else if ((simpleModeValue == 0) && (superSimpleModeValue == _allSimpleBits)) {
            _simpleMode = SimpleModeSuperSimple;
        } else {
            _simpleMode = SimpleModeCustom;
        }

        (void) connect(this, &APMFlightModesComponentController::simpleModeChanged, this, &APMFlightModesComponentController::_updateSimpleParamsFromSimpleMode);
    }

    QStringList usedParams;
    for (int i = 1; i < 7; i++) {
        usedParams << QStringLiteral("%1%2").arg(_modeParamPrefix).arg(i);
    }
    if (!_allParametersExists(ParameterManager::defaultComponentId, usedParams)) {
        return;
    }

    for (int i = 0; i < _cChannelOptions; i++) {
        _rgChannelOptionEnabled.append(QVariant(false));
    }

    (void) connect(_vehicle, &Vehicle::rcChannelsChanged, this, &APMFlightModesComponentController::_rcChannelsChanged);
}

void APMFlightModesComponentController::_rcChannelsChanged(int channelCount, int pwmValues[QGCMAVLink::maxRcChannels])
{
    int flightModeChannel = 4;

    if (parameterExists(ParameterManager::defaultComponentId, _modeChannelParam)) {
        flightModeChannel = getParameterFact(ParameterManager::defaultComponentId, _modeChannelParam)->rawValue().toInt() - 1;
    }

    if (flightModeChannel >= channelCount) {
        return;
    }

    _activeFlightMode = 0;
    int channelValue = pwmValues[flightModeChannel];
    if (channelValue != -1) {
        bool found = false;
        static constexpr const int rgThreshold[] = { 1230, 1360, 1490, 1620, 1749 };
        for (int i = 0; i < std::size(rgThreshold); i++) {
            if (channelValue <= rgThreshold[i]) {
                _activeFlightMode = i + 1;
                found = true;
                break;
            }
        }
        if (!found) {
            _activeFlightMode = 6;
        }
    }
    emit activeFlightModeChanged(_activeFlightMode);

    for (int i = 0; i < _cChannelOptions; i++) {
        _rgChannelOptionEnabled[i] = QVariant(false);
        channelValue = pwmValues[i + 5];
        if (channelValue > 1800) {
            _rgChannelOptionEnabled[i] = QVariant(true);
        }
    }
    emit channelOptionEnabledChanged();
}

void APMFlightModesComponentController::_updateSimpleParamsFromSimpleMode()
{
    int newSimpleModeValue = 0;
    int newSuperSimpleModeValue = 0;

    if (_simpleMode == SimpleModeSimple) {
        newSimpleModeValue = _allSimpleBits;
    } else if (_simpleMode == SimpleModeSuperSimple) {
        newSuperSimpleModeValue = _allSimpleBits;
    }

    for (int i = 0; i < _cFltModes; i++) {
        _simpleModeEnabled[i] = false;
        _superSimpleModeEnabled[i] = false;
    }
    emit simpleModeEnabledChanged();
    emit superSimpleModeEnabledChanged();

    _simpleModeFact->setRawValue(newSimpleModeValue);
    _superSimpleModeFact->setRawValue(newSuperSimpleModeValue);
}

void APMFlightModesComponentController::setSimpleMode(int fltModeIndex, bool enabled)
{
    if (fltModeIndex < _cFltModes) {
        uint8_t mode = static_cast<uint8_t>(_simpleModeFact->rawValue().toInt());
        if (enabled) {
            mode |= 1 << fltModeIndex;
        } else {
            mode &= ~(1 << fltModeIndex);
        }
        _simpleModeFact->setRawValue(mode);
    }
}

void APMFlightModesComponentController::setSuperSimpleMode(int fltModeIndex, bool enabled)
{
    if (fltModeIndex < _cFltModes) {
        uint8_t mode = static_cast<uint8_t>(_superSimpleModeFact->rawValue().toInt());
        if (enabled) {
            mode |= 1 << fltModeIndex;
        } else {
            mode &= ~(1 << fltModeIndex);
        }
        _superSimpleModeFact->setRawValue(mode);
    }
}

void APMFlightModesComponentController::_setupSimpleModeEnabled()
{
    const uint8_t simpleMode = static_cast<uint8_t>(_simpleModeFact->rawValue().toUInt());
    const uint8_t superSimpleMode = static_cast<uint8_t>(_superSimpleModeFact->rawValue().toUInt());

    for (int i = 0; i < _cFltModes; i++) {
        const uint8_t bitSet = static_cast<uint8_t>(1 << i);
        _simpleModeEnabled[i] = !!(simpleMode & bitSet);
        _superSimpleModeEnabled[i] = !!(superSimpleMode & bitSet);
    }

    emit simpleModeEnabledChanged();
    emit superSimpleModeEnabledChanged();
}
