/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RadioComponentController.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(RadioComponentControllerLog, "AutoPilotPlugins.RadioComponentController")
QGC_LOGGING_CATEGORY(RadioComponentControllerVerboseLog, "AutoPilotPlugins.RadioComponentController:verbose")

RadioComponentController::RadioComponentController(QObject *parent)
    : RemoteControlCalibrationController(parent)
{
    // qCDebug(RadioComponentControllerLog) << Q_FUNC_INFO << this;

    // Channels values are in PWM
    _calValidMinValue = 1300;
    _calValidMaxValue = 1700;
    _calCenterPoint= ((_calValidMaxValue - _calValidMinValue) / 2.0f) + _calValidMinValue;
    _calDefaultMinValue = 1000;
    _calDefaultMaxValue = 2000;
    _calRoughCenterDelta = 50;
    _calMoveDelta = 300;
    _calSettleDelta = 20;
    _calMinDelta = 100;

    // Deal with parameter differences between PX4 and Ardupilot
    if (parameterExists(ParameterManager::defaultComponentId, QStringLiteral("RC1_REVERSED"))) {
        // Newer ardupilot firmwares have a different reverse param naming scheme and value scheme
        _revParamFormat = "RC%1_REVERSED";
        _revParamIsBool = true; // param value is boolean 0/1 for reversed or not
    } else {
        // Older ardupilot firmwares share the same naming convention as PX4
        _revParamFormat = "RC%1_REV";
        _revParamIsBool = false; // param value if -1 indicates reversed
    }

    (void) connect(_vehicle, &Vehicle::rcChannelsChanged, this, &RemoteControlCalibrationController::channelValuesChanged);
}

RadioComponentController::~RadioComponentController()
{
    // qCDebug(RadioComponentControllerLog) << Q_FUNC_INFO << this;
}

void RadioComponentController::spektrumBindMode(int mode)
{
    _vehicle->pairRX(RC_TYPE_SPEKTRUM, mode);
}

void RadioComponentController::crsfBindMode()
{
    _vehicle->pairRX(RC_TYPE_CRSF, 0);
}

bool RadioComponentController::_channelReversedParamValue(int channel)
{
    Fact *const paramFact = getParameterFact(ParameterManager::defaultComponentId, _revParamFormat.arg(channel+1));
    if (paramFact) {
        if (_revParamIsBool) {
            return paramFact->rawValue().toBool();
        } else {
            bool convertOk;
            float floatReversed = paramFact->rawValue().toFloat(&convertOk);
            if (!convertOk) {
                floatReversed = 1.0f;
            }

            return floatReversed == -1.0f;
        }
    }

    return false;
}

void RadioComponentController::_setChannelReversedParamValue(int channel, bool reversed)
{
    Fact *const paramFact = getParameterFact(ParameterManager::defaultComponentId, _revParamFormat.arg(channel+1));
    if (paramFact) {
        if (_revParamIsBool) {
            paramFact->setRawValue(reversed);
        } else {
            paramFact->setRawValue(reversed ? -1.0f : 1.0f);
        }
    }
}

void RadioComponentController::_saveStoredCalibrationValues()
{
    if (!_vehicle->px4Firmware() && ((_vehicle->vehicleType() == MAV_TYPE_HELICOPTER) || (_vehicle->multiRotor()) &&  _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionThrottle]].reversed)) {
        // A reversed throttle could lead to dangerous power up issues if the firmware doesn't handle it absolutely correctly in all places.
        // So in this case fail the calibration for anything other than PX4 which is known to be able to handle this correctly.
        emit throttleReversedCalFailure();
    } else {
        _validateAndAdjustCalibrationValues();

        const QString minTpl("RC%1_MIN");
        const QString maxTpl("RC%1_MAX");
        const QString trimTpl("RC%1_TRIM");

        // Note that the rc parameters are all float, so you must cast to float in order to get the right QVariant
        for (int chan = 0; chan<_chanMax; chan++) {
            ChannelInfo *const info = &_rgChannelInfo[chan];
            const int oneBasedChannel = chan + 1;

            if (!parameterExists(ParameterManager::defaultComponentId, minTpl.arg(chan+1))) {
                continue;
            }

            Fact* paramFact = getParameterFact(ParameterManager::defaultComponentId, trimTpl.arg(oneBasedChannel));
            if (paramFact) {
                paramFact->setRawValue(static_cast<float>(info->rcTrim));
            }
            paramFact = getParameterFact(ParameterManager::defaultComponentId, minTpl.arg(oneBasedChannel));
            if (paramFact) {
                paramFact->setRawValue(static_cast<float>(info->rcMin));
            }
            paramFact = getParameterFact(ParameterManager::defaultComponentId, maxTpl.arg(oneBasedChannel));
            if (paramFact) {
                paramFact->setRawValue(static_cast<float>(info->rcMax));
            }

            // For multi-rotor we can determine reverse setting during radio cal. For anything other than multi-rotor, servo installation
            // may affect channel reversing so we can't automatically determine it. This is ok for PX4 given how it uses mixers, but not for ArduPilot.
            if (_vehicle->px4Firmware() || _vehicle->multiRotor()) {
                // APM multi-rotor has a backwards interpretation of "reversed" on the Pitch control. So be careful.
                bool reversed;
                if (_vehicle->px4Firmware() || info->function != stickFunctionPitch) {
                    reversed = info->reversed;
                } else {
                    reversed = !info->reversed;
                }
                _setChannelReversedParamValue(chan, reversed);
            }
        }

        // Write function mapping parameters
        for (size_t stickFunctionIndex = 0; stickFunctionIndex < stickFunctionMax; stickFunctionIndex++) {
            int32_t paramChannel;
            if (_rgFunctionChannelMapping[stickFunctionIndex] == _chanMax) {
                // 0 signals no mapping
                paramChannel = 0;
            } else {
                // Note that the channel value is 1-based
                paramChannel = _rgFunctionChannelMapping[stickFunctionIndex] + 1;
            }

            QString paramName = _stickFunctionToParamName(static_cast<StickFunction>(stickFunctionIndex));
            Fact* paramFact = getParameterFact(ParameterManager::defaultComponentId, paramName);

            if (paramFact && paramFact->rawValue().toInt() != paramChannel) {
                paramFact = getParameterFact(ParameterManager::defaultComponentId, paramName);
                if (paramFact) {
                    paramFact->setRawValue(paramChannel);
                }
            }
        }
    }

    if (_vehicle->px4Firmware()) {
        // If the RC_CHAN_COUNT parameter is available write the channel count
        if (parameterExists(ParameterManager::defaultComponentId, QStringLiteral("RC_CHAN_CNT"))) {
            getParameterFact(ParameterManager::defaultComponentId, QStringLiteral("RC_CHAN_CNT"))->setRawValue(_chanCount);
        }
    }

    _stopCalibration();
    _readStoredCalibrationValues();
}

void RadioComponentController::_readStoredCalibrationValues()
{
    // Initialize all function mappings to not set

    for (int i = 0; i < _chanMax; i++) {
        ChannelInfo *const info = &_rgChannelInfo[i];
        info->function = stickFunctionMax;
    }

    for (size_t i = 0; i < stickFunctionMax; i++) {
        _rgFunctionChannelMapping[i] = _chanMax;
    }

    // Pull parameters and update

    const QString minTpl("RC%1_MIN");
    const QString maxTpl("RC%1_MAX");
    const QString trimTpl("RC%1_TRIM");

    for (int i = 0; i < _chanMax; ++i) {
        ChannelInfo *const info = &_rgChannelInfo[i];

        if (!parameterExists(ParameterManager::defaultComponentId, minTpl.arg(i+1))) {
            info->rcTrim = 1500;
            info->rcMin = 1100;
            info->rcMax = 1900;
            info->reversed = false;
            continue;
        }

        Fact *paramFact = getParameterFact(ParameterManager::defaultComponentId, trimTpl.arg(i+1));
        if (paramFact) {
            info->rcTrim = paramFact->rawValue().toInt();
        }

        paramFact = getParameterFact(ParameterManager::defaultComponentId, minTpl.arg(i+1));
        if (paramFact) {
            info->rcMin = paramFact->rawValue().toInt();
        }

        paramFact = getParameterFact(ParameterManager::defaultComponentId, maxTpl.arg(i+1));
        if (paramFact) {
            info->rcMax = getParameterFact(ParameterManager::defaultComponentId, maxTpl.arg(i+1))->rawValue().toInt();
        }

        info->reversed = _channelReversedParamValue(i);
    }

    for (int i=0; i<stickFunctionMax; i++) {
        int32_t paramChannel;

        QString paramName = _stickFunctionToParamName(static_cast<StickFunction>(i));
        Fact *const paramFact = getParameterFact(ParameterManager::defaultComponentId, paramName);
        if (paramFact) {
            paramChannel = paramFact->rawValue().toInt();

            if (paramChannel > 0 && paramChannel <= _chanMax) {
                _rgFunctionChannelMapping[i] = paramChannel - 1;
                _rgChannelInfo[paramChannel - 1].function = static_cast<StickFunction>(i);
            }
        }
    }

    _signalAllAttitudeValueChanges();
}

QString RadioComponentController::_stickFunctionToParamName(RemoteControlCalibrationController::StickFunction function) const
{
    static const QStringList rgStickFunctionParamsPX4({
        { "RC_MAP_ROLL" },
        { "RC_MAP_PITCH" },
        { "RC_MAP_YAW" },
        { "RC_MAP_THROTTLE" }
    });

    static const QStringList rgStickFunctionParamsAPM({
        { "RCMAP_ROLL" },
        { "RCMAP_PITCH" },
        { "RCMAP_YAW" },
        { "RCMAP_THROTTLE" }
    });

    if (rgStickFunctionParamsPX4.size() != stickFunctionMax ||
        rgStickFunctionParamsAPM.size() != stickFunctionMax) {
        qCCritical(RadioComponentControllerLog) << "Internal Error: Param name arrays incorrect size";
        return QString();
    }
    if (function < 0 || function >= stickFunctionMax) {
        qCCritical(RadioComponentControllerLog) << "Internal Error: Invalid stick function index";
        return QString();
    }

    auto &rgStickFunctionParams = _vehicle->px4Firmware() ? rgStickFunctionParamsPX4 : rgStickFunctionParamsAPM;

    return rgStickFunctionParams[function];
}
