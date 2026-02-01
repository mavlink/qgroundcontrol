#include "JoystickConfigController.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include <QtCore/QLoggingCategory>
#include "Vehicle.h"

#include <QtCore/QSettings>

Q_STATIC_LOGGING_CATEGORY(JoystickConfigControllerLog, "AutoPilotPlugins.JoystickConfigController")
Q_STATIC_LOGGING_CATEGORY(JoystickConfigControllerVerboseLog, "AutoPilotPlugins.JoystickConfigController:verbose")

JoystickConfigController::JoystickConfigController(QObject *parent)
    : RemoteControlCalibrationController(parent)
{
    // Channel values are based on -32768:32767 range from SDL
    _calCenterPoint =       0;
    _calValidMinValue =     -32768;     ///< Largest valid minimum axis value
    _calValidMaxValue =     32767;      ///< Smallest valid maximum axis value
    _calDefaultMinValue =   -32768;     ///< Default value for Min if not set
    _calDefaultMaxValue =   32767;      ///< Default value for Max if not set
    _calRoughCenterDelta =  700;        ///< Delta around center point which is considered to be roughly centered (Note: PS5 controller has very noisy center, hence 700)
    _calMoveDelta =         32768/2;    ///< Amount of delta past center which is considered stick movement
    _calSettleDelta =       1000;       ///< Amount of delta which is considered no stick movement (increased for noisy joysticks at extreme positions)
    _stickDetectSettleMSecs = 300;      ///< Reduced settle time for joysticks (faster response than RC transmitters)
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
    emit joystickChanged(_joystick);
}

void JoystickConfigController::_saveStoredCalibrationValues()
{
    if (!_joystick) {
        qCWarning(JoystickConfigControllerLog) << "Internal Error: No joystick to save calibration for";
        return;
    }

    _validateAndAdjustCalibrationValues();

    _joystick->settings()->calibrated()->setRawValue(true);
    _joystick->settings()->transmitterMode()->setRawValue(transmitterMode());

    for (int chan = 0; chan < _joystick->axisCount(); chan++) {
        ChannelInfo *const info = &_rgChannelInfo[chan];
        Joystick::AxisCalibration_t joystickAxisInfo;

        joystickAxisInfo.center = info->channelTrim;
        joystickAxisInfo.min = info->channelMin;
        joystickAxisInfo.max = info->channelMax;
        joystickAxisInfo.reversed = info->channelReversed;
        joystickAxisInfo.deadband = info->deadband;
        _joystick->setAxisCalibration(chan, joystickAxisInfo);
    }

    // Write function mapping parameters
    for (size_t stickFunctionIndex = 0; stickFunctionIndex < stickFunctionMax; stickFunctionIndex++) {
        _joystick->setFunctionForChannel(static_cast<RemoteControlCalibrationController::StickFunction>(stickFunctionIndex), _rgFunctionChannelMapping[stickFunctionIndex]);
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

    for (int axisFunction = 0; axisFunction < Joystick::maxAxisFunction; axisFunction++) {
        if (axisFunction == Joystick::gimbalPitchFunction || axisFunction == Joystick::gimbalYawFunction) {
            continue;
        }
        StickFunction stickFunction = _joystick->mapAxisFunctionToRCCStickFunction(static_cast<Joystick::AxisFunction_t>(axisFunction));
        int functionChannel = _joystick->getChannelForFunction(stickFunction);
        if (functionChannel >= 0 && functionChannel < _chanMax) {
            _rgFunctionChannelMapping[stickFunction] = functionChannel;
            _rgChannelInfo[functionChannel].stickFunction = stickFunction;
        }
    }

    _signalAllAttitudeValueChanges();
}
