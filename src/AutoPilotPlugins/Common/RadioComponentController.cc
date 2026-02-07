#include "RadioComponentController.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include <QtCore/QLoggingCategory>
#include "Vehicle.h"

#include <QtCore/QSettings>

Q_STATIC_LOGGING_CATEGORY(RadioComponentControllerLog, "AutoPilotPlugins.RadioComponentController")
Q_STATIC_LOGGING_CATEGORY(RadioComponentControllerVerboseLog, "AutoPilotPlugins.RadioComponentController:verbose")

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

    // Let the mav known we are starting calibration. This should turn off motors and so forth.
    _vehicle->startCalibration(QGCMAVLink::CalibrationRadio);
}

RadioComponentController::~RadioComponentController()
{
    // qCDebug(RadioComponentControllerLog) << Q_FUNC_INFO << this;
    if (_vehicle) {
        // Only PX4 is known to support this command in all versions. For other firmware which may or may not
        // support this we don't show errors on failure.
        _vehicle->stopCalibration(_vehicle->px4Firmware() ? true : false /* showError */);
    }
}

void RadioComponentController::start(void)
{
    RemoteControlCalibrationController::start();
    (void) connect(_vehicle, &Vehicle::rcChannelsChanged, this, &RemoteControlCalibrationController::rawChannelValuesChanged);

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
    if (!_vehicle->px4Firmware() && ((_vehicle->vehicleType() == MAV_TYPE_HELICOPTER) || (_vehicle->multiRotor()) &&  _rgChannelInfo[_rgFunctionChannelMapping[stickFunctionThrottle]].channelReversed)) {
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
                paramFact->setRawValue(static_cast<float>(info->channelTrim));
            }
            paramFact = getParameterFact(ParameterManager::defaultComponentId, minTpl.arg(oneBasedChannel));
            if (paramFact) {
                paramFact->setRawValue(static_cast<float>(info->channelMin));
            }
            paramFact = getParameterFact(ParameterManager::defaultComponentId, maxTpl.arg(oneBasedChannel));
            if (paramFact) {
                paramFact->setRawValue(static_cast<float>(info->channelMax));
            }

            // For multi-rotor we can determine reverse setting during radio cal. For anything other than multi-rotor, servo installation
            // may affect channel reversing so we can't automatically determine it. This is ok for PX4 given how it uses mixers, but not for ArduPilot.
            if (_vehicle->px4Firmware() || _vehicle->multiRotor()) {
                // APM multi-rotor has a backwards interpretation of "reversed" on the Pitch control. So be careful.
                bool reversed;
                if (_vehicle->px4Firmware() || info->stickFunction != stickFunctionPitch) {
                    reversed = info->channelReversed;
                } else {
                    reversed = !info->channelReversed;
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

    // Read back since validation may have changed values
    _readStoredCalibrationValues();
}

void RadioComponentController::_readStoredCalibrationValues()
{
    // Initialize all function mappings to not set

    for (int i = 0; i < _chanMax; i++) {
        ChannelInfo *const info = &_rgChannelInfo[i];
        info->stickFunction = stickFunctionMax;
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
            info->channelTrim = 1500;
            info->channelMin = 1100;
            info->channelMax = 1900;
            info->channelReversed = false;
            continue;
        }

        Fact *paramFact = getParameterFact(ParameterManager::defaultComponentId, trimTpl.arg(i+1));
        if (paramFact) {
            info->channelTrim = paramFact->rawValue().toInt();
        }

        paramFact = getParameterFact(ParameterManager::defaultComponentId, minTpl.arg(i+1));
        if (paramFact) {
            info->channelMin = paramFact->rawValue().toInt();
        }

        paramFact = getParameterFact(ParameterManager::defaultComponentId, maxTpl.arg(i+1));
        if (paramFact) {
            info->channelMax = getParameterFact(ParameterManager::defaultComponentId, maxTpl.arg(i+1))->rawValue().toInt();
        }

        info->channelReversed = _channelReversedParamValue(i);
    }

    for (int i=0; i<stickFunctionMax; i++) {
        int32_t paramChannel;

        QString paramName = _stickFunctionToParamName(static_cast<StickFunction>(i));
        Fact *const paramFact = getParameterFact(ParameterManager::defaultComponentId, paramName);
        if (paramFact) {
            paramChannel = paramFact->rawValue().toInt();

            if (paramChannel > 0 && paramChannel <= _chanMax) {
                _rgFunctionChannelMapping[i] = paramChannel - 1;
                _rgChannelInfo[paramChannel - 1].stickFunction = static_cast<StickFunction>(i);
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
