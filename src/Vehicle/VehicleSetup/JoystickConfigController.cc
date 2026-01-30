#include "JoystickConfigController.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(JoystickConfigControllerLog, "AutoPilotPlugins.JoystickConfigController")
QGC_LOGGING_CATEGORY(JoystickConfigControllerVerboseLog, "AutoPilotPlugins.JoystickConfigController:verbose")

JoystickConfigController::JoystickConfigController(QObject *parent)
    : RemoteControlCalibrationController(parent)
{
    // Channel values are based on -32768:32767 range from SDL

    _calCenterPoint =       0;
    _calDefaultMinValue =   -32768;     ///< Default value for Min if not set
    _calDefaultMaxValue =   32767;      ///< Default value for Max if not set
    _calRoughCenterDelta =  700;        ///< Delta around center point which is considered to be roughly centered (Note: PS5 controller has very noisy center, hence 700)
    _calMoveDelta =         32768/2;    ///< Amount of delta past center which is considered stick movement
    _calSettleDelta =       1000;       ///< Amount of delta which is considered no stick movement (increased for noisy joysticks at extreme positions)
    _stickDetectSettleMSecs = 300;      ///< Reduced settle time for joysticks (faster response than RC transmitters)

    int valueRange = _calDefaultMaxValue - _calDefaultMinValue;
    _calValidMinValue = _calDefaultMinValue + (valueRange * 0.3f);
    _calValidMaxValue = _calDefaultMaxValue - (valueRange * 0.3f);
}

JoystickConfigController::~JoystickConfigController()
{
    if (_joystick) {
        _joystick->_stopPollingForConfiguration();
    }
}

void JoystickConfigController::start(void)
{
    if (!_joystick) {
        qCWarning(JoystickConfigControllerLog) << "No joystick set!";
        return;
    }

    qCDebug(JoystickConfigControllerLog) << "Starting joystick configuration for joystick:" << _joystick->name();
    RemoteControlCalibrationController::start();
    _joystick->_startPollingForConfiguration();
}

void JoystickConfigController::_setJoystick(Joystick* joystick)
{
    if (!joystick) {
        qCWarning(JoystickConfigControllerLog) << "Cannot set joystick: null joystick";
        return;
    }
    if (_joystick) {
        qCWarning(JoystickConfigControllerLog) << "Joystick should only be set a single time";
        return;
    }

    _joystick = joystick;
    _readStoredCalibrationValues();
    connect(_joystick, &Joystick::rawChannelValuesChanged, this, &JoystickConfigController::rawChannelValuesChanged);

    // Connect to settings changes to emit extension enabled signals
    connect(_joystick->settings()->enableManualControlPitchExtension(), &Fact::rawValueChanged, this, [this]() {
        emit pitchExtensionEnabledChanged(pitchExtensionEnabled());
        emit anyExtensionEnabledChanged(anyExtensionEnabled());
    });
    connect(_joystick->settings()->enableManualControlRollExtension(), &Fact::rawValueChanged, this, [this]() {
        emit rollExtensionEnabledChanged(rollExtensionEnabled());
        emit anyExtensionEnabledChanged(anyExtensionEnabled());
    });
    connect(_joystick->settings()->enableManualControlAux1(), &Fact::rawValueChanged, this, [this]() {
        emit aux1ExtensionEnabledChanged(aux1ExtensionEnabled());
        emit anyExtensionEnabledChanged(anyExtensionEnabled());
    });
    connect(_joystick->settings()->enableManualControlAux2(), &Fact::rawValueChanged, this, [this]() {
        emit aux2ExtensionEnabledChanged(aux2ExtensionEnabled());
        emit anyExtensionEnabledChanged(anyExtensionEnabled());
    });
    connect(_joystick->settings()->enableManualControlAux3(), &Fact::rawValueChanged, this, [this]() {
        emit aux3ExtensionEnabledChanged(aux3ExtensionEnabled());
        emit anyExtensionEnabledChanged(anyExtensionEnabled());
    });
    connect(_joystick->settings()->enableManualControlAux4(), &Fact::rawValueChanged, this, [this]() {
        emit aux4ExtensionEnabledChanged(aux4ExtensionEnabled());
        emit anyExtensionEnabledChanged(anyExtensionEnabled());
    });
    connect(_joystick->settings()->enableManualControlAux5(), &Fact::rawValueChanged, this, [this]() {
        emit aux5ExtensionEnabledChanged(aux5ExtensionEnabled());
        emit anyExtensionEnabledChanged(anyExtensionEnabled());
    });
    connect(_joystick->settings()->enableManualControlAux6(), &Fact::rawValueChanged, this, [this]() {
        emit aux6ExtensionEnabledChanged(aux6ExtensionEnabled());
        emit anyExtensionEnabledChanged(anyExtensionEnabled());
    });

    emit joystickChanged(_joystick);
}

void JoystickConfigController::_saveStoredCalibrationValues()
{
    if (!_joystick) {
        qCWarning(JoystickConfigControllerLog) << "Internal Error: No joystick to save calibration for";
        return;
    }

    qCDebug(JoystickConfigControllerLog) << _joystick->name();

    _validateAndAdjustCalibrationValues();

    _joystick->settings()->calibrated()->setRawValue(true);
    _joystick->settings()->transmitterMode()->setRawValue(transmitterMode());

    for (int chan = 0; chan < _joystick->axisCount(); chan++) {
        ChannelInfo *const channelInfo = &_rgChannelInfo[chan];
        Joystick::AxisCalibration_t joystickAxisInfo;

        qCDebug(JoystickConfigControllerLog)
            << "chan:" <<       chan
            << "function:" <<   _stickFunctionToString(channelInfo->stickFunction)
            << "min:" <<        channelInfo->channelMin
            << "max:" <<        channelInfo->channelMax
            << "trim:" <<       channelInfo->channelTrim
            << "reversed:" <<   channelInfo->channelReversed
            << "deadband:" <<   channelInfo->deadband;

        joystickAxisInfo.center = channelInfo->channelTrim;
        joystickAxisInfo.min = channelInfo->channelMin;
        joystickAxisInfo.max = channelInfo->channelMax;
        joystickAxisInfo.reversed = channelInfo->channelReversed;
        joystickAxisInfo.deadband = channelInfo->deadband;
        _joystick->setAxisCalibration(chan, joystickAxisInfo);

        _joystick->setFunctionForChannel(channelInfo->stickFunction, chan);
    }

    _joystick->_saveFromCalibrationDataIntoSettings();

    // Read back since validation may have changed values
    _readStoredCalibrationValues();
}

void JoystickConfigController::_readStoredCalibrationValues()
{
    if (!_joystick) {
        qCWarning(JoystickConfigControllerLog) << "Internal Error: No joystick to read calibration for";
        return;
    }

    setTransmitterMode(_joystick->settings()->transmitterMode()->rawValue().toInt());

    // Initialize all function mappings to not set
    for (int i = 0; i < _chanMax; i++) {
        ChannelInfo *const info = &_rgChannelInfo[i];
        info->stickFunction = stickFunctionMax;
    }
    for (size_t i = 0; i < stickFunctionMax; i++) {
        _rgFunctionChannelMapping[i] = _chanMax;
    }

    for (int i = 0; i < _joystick->axisCount(); ++i) {
        ChannelInfo *const info = &_rgChannelInfo[i];
        Joystick::AxisCalibration_t calibration = _joystick->getAxisCalibration(i);

        info->channelTrim  = calibration.center;
        info->channelMin   = calibration.min;
        info->channelMax   = calibration.max;
        info->channelReversed  = calibration.reversed;
        info->deadband  = calibration.deadband;
    }

    for (int stickFunctionInt = 0; stickFunctionInt < Joystick::maxAxisFunction; stickFunctionInt++) {
        auto stickFunction = static_cast<StickFunction>(stickFunctionInt);
        int functionChannel = _joystick->getChannelForFunction(stickFunction);
        if (functionChannel >= 0 && functionChannel < _chanMax) {
            _rgFunctionChannelMapping[stickFunction] = functionChannel;
            _rgChannelInfo[functionChannel].stickFunction = stickFunction;
        }
    }

    _signalAllAttitudeValueChanges();
}


bool JoystickConfigController::_stickFunctionEnabled(StickFunction stickFunction)
{
    if (RemoteControlCalibrationController::_stickFunctionEnabled(stickFunction)) {
        return true;
    }

    switch (stickFunction) {
    case stickFunctionRollExtension:
        return _joystick->settings()->enableManualControlRollExtension()->rawValue().toBool();
    case stickFunctionPitchExtension:
        return _joystick->settings()->enableManualControlPitchExtension()->rawValue().toBool();
    case stickFunctionAux1Extension:
        return _joystick->settings()->enableManualControlAux1()->rawValue().toBool();
    case stickFunctionAux2Extension:
        return _joystick->settings()->enableManualControlAux2()->rawValue().toBool();
    case stickFunctionAux3Extension:
        return _joystick->settings()->enableManualControlAux3()->rawValue().toBool();
    case stickFunctionAux4Extension:
        return _joystick->settings()->enableManualControlAux4()->rawValue().toBool();
    case stickFunctionAux5Extension:
        return _joystick->settings()->enableManualControlAux5()->rawValue().toBool();
    case stickFunctionAux6Extension:
        return _joystick->settings()->enableManualControlAux6()->rawValue().toBool();
    case stickFunctionRoll:
    case stickFunctionPitch:
    case stickFunctionYaw:
    case stickFunctionThrottle:
        return RemoteControlCalibrationController::_stickFunctionEnabled(stickFunction);
    case stickFunctionMax:
        qCWarning(JoystickConfigControllerLog) << "Invalid stick function requested: stickFunctionMax";
        break;
    }

    return false;
}
