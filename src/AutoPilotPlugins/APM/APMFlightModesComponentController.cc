/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMFlightModesComponentController.h"
#include "QGCMAVLink.h"

#include <QVariant>
#include <QQmlProperty>

bool APMFlightModesComponentController::_typeRegistered = false;

const char* APMFlightModesComponentController::_simpleParamName =       "SIMPLE";
const char* APMFlightModesComponentController::_superSimpleParamName =  "SUPER_SIMPLE";

APMFlightModesComponentController::APMFlightModesComponentController(void)
    : _activeFlightMode     (0)
    , _channelCount         (Vehicle::cMaxRcChannels)
    , _simpleMode           (SimpleModeStandard)
    , _simpleModeFact       (parameterExists(-1, _simpleParamName)      ? getParameterFact(-1, _simpleParamName) : nullptr)
    , _superSimpleModeFact  (parameterExists(-1, _superSimpleParamName) ? getParameterFact(-1, _superSimpleParamName) : nullptr)
    , _simpleModesSupported (_simpleModeFact && _superSimpleModeFact)
{
    if (!_typeRegistered) {
        qmlRegisterUncreatableType<APMFlightModesComponentController>("QGroundControl.Controllers", 1, 0, "APMFlightModesComponentController", "Reference only");
    }

    bool arduRoverFirmware = parameterExists(-1, QStringLiteral("MODE1"));
    _modeParamPrefix = arduRoverFirmware ? QStringLiteral("MODE") : QStringLiteral("FLTMODE");
    _modeChannelParam = arduRoverFirmware ? QStringLiteral("MODE_CH") : QStringLiteral("FLTMODE_CH");

    _simpleModeNames << tr("Off") << tr("Simple") << tr("Super-Simple") << tr("Custom");
    for (int i=0; i<_cFltModes; i++) {
        _simpleModeEnabled.append(QVariant(false));
        _superSimpleModeEnabled.append(QVariant(false));
    }

    if (_simpleModesSupported) {
        _setupSimpleModeEnabled();

        uint8_t simpleModeValue = static_cast<uint8_t>(_simpleModeFact->rawValue().toUInt());
        uint8_t superSimpleModeValue = static_cast<uint8_t>(_superSimpleModeFact->rawValue().toUInt());
        if (simpleModeValue == 0 && superSimpleModeValue == 0) {
            _simpleMode = SimpleModeStandard;
        } else if (simpleModeValue == _allSimpleBits && superSimpleModeValue == 0) {
            _simpleMode = SimpleModeSimple;
        } else if (simpleModeValue == 0 && superSimpleModeValue == _allSimpleBits) {
            _simpleMode = SimpleModeSuperSimple;
        } else {
            _simpleMode = SimpleModeCustom;
        }

        connect(this, &APMFlightModesComponentController::simpleModeChanged, this, &APMFlightModesComponentController::_updateSimpleParamsFromSimpleMode);
    }

    QStringList usedParams;
    for (int i=1; i<7; i++) {
        usedParams << QStringLiteral("%1%2").arg(_modeParamPrefix).arg(i);
    }
    if (!_allParametersExists(FactSystem::defaultComponentId, usedParams)) {
        return;
    }

    for (int i=0; i<_cChannelOptions; i++) {
        _rgChannelOptionEnabled.append(QVariant(false));
    }

    connect(_vehicle, &Vehicle::rcChannelsChanged, this, &APMFlightModesComponentController::_rcChannelsChanged);
}

/// Connected to Vehicle::rcChannelsChanged signal
void APMFlightModesComponentController::_rcChannelsChanged(int channelCount, int pwmValues[Vehicle::cMaxRcChannels])
{
    int flightModeChannel = 4;

    if (parameterExists(FactSystem::defaultComponentId, _modeChannelParam)) {
        flightModeChannel = getParameterFact(FactSystem::defaultComponentId, _modeChannelParam)->rawValue().toInt() - 1;
    }

    if (flightModeChannel >= channelCount) {
        return;
    }

    _activeFlightMode = 0;
    int channelValue = pwmValues[flightModeChannel];
    if (channelValue != -1) {
        bool found = false;
        int rgThreshold[] = { 1230, 1360, 1490, 1620, 1749 };
        for (int i=0; i<5; i++) {
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

    for (int i=0; i<_cChannelOptions; i++) {
        _rgChannelOptionEnabled[i] = QVariant(false);
        channelValue = pwmValues[i+6];
        if (channelValue > 1800) {
            _rgChannelOptionEnabled[i] = QVariant(true);
        }
    }
    emit channelOptionEnabledChanged();
}

void APMFlightModesComponentController::_updateSimpleParamsFromSimpleMode(void)
{
    int newSimpleModeValue = 0;
    int newSuperSimpleModeValue = 0;

    if (_simpleMode == SimpleModeSimple) {
        newSimpleModeValue = _allSimpleBits;
    } else if (_simpleMode == SimpleModeSuperSimple) {
        newSuperSimpleModeValue = _allSimpleBits;
    }

    for (int i=0; i<_cFltModes; i++) {
        _simpleModeEnabled[i] =         false;
        _superSimpleModeEnabled[i] =    false;
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

void APMFlightModesComponentController::_setupSimpleModeEnabled(void)
{
    uint8_t simpleMode =        static_cast<uint8_t>(_simpleModeFact->rawValue().toUInt());
    uint8_t superSimpleMode =   static_cast<uint8_t>(_superSimpleModeFact->rawValue().toUInt());

    for (int i=0; i<_cFltModes; i++) {
        uint8_t bitSet = static_cast<uint8_t>(1 << i);
        _simpleModeEnabled[i] = !!(simpleMode & bitSet);
        _superSimpleModeEnabled[i] = !!(superSimpleMode & bitSet);
    }

    emit simpleModeEnabledChanged();
    emit superSimpleModeEnabledChanged();
}
