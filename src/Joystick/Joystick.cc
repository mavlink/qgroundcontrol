/****************************************************************************
 *
 *   (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "Joystick.h"
#include "QGC.h"
#include "AutoPilotPlugin.h"
#include "UAS.h"
#include "QGCApplication.h"
#include "VideoManager.h"
#include "QGCCameraManager.h"
#include "QGCCameraControl.h"

#include <QSettings>

QGC_LOGGING_CATEGORY(JoystickLog,       "JoystickLog")
QGC_LOGGING_CATEGORY(JoystickValuesLog, "JoystickValuesLog")

const char* Joystick::_settingsGroup =                  "Joysticks";
const char* Joystick::_calibratedSettingsKey =          "Calibrated4"; // Increment number to force recalibration
const char* Joystick::_buttonActionNameKey =            "ButtonActionName%1";
const char* Joystick::_buttonSecondaryActionNameKey =   "ButtonSecondaryActionName%1";
const char* Joystick::_buttonActionRepeatKey =          "ButtonActionRepeat%1";
const char* Joystick::_buttonComboKey =                 "ButtonCombo%1";
const char* Joystick::_buttonSecondaryActionTypeKey =   "ButtonSecondaryActionType%1";
const char* Joystick::_throttleModeSettingsKey =        "ThrottleMode";
const char* Joystick::_exponentialSettingsKey =         "Exponential";
const char* Joystick::_accumulatorSettingsKey =         "Accumulator";
const char* Joystick::_deadbandSettingsKey =            "Deadband";
const char* Joystick::_circleCorrectionSettingsKey =    "Circle_Correction";
const char* Joystick::_axisFrequencySettingsKey =       "AxisFrequency";
const char* Joystick::_buttonFrequencySettingsKey =     "ButtonFrequency";
const char* Joystick::_txModeSettingsKey =              nullptr;
const char* Joystick::_fixedWingTXModeSettingsKey =     "TXMode_FixedWing";
const char* Joystick::_multiRotorTXModeSettingsKey =    "TXMode_MultiRotor";
const char* Joystick::_roverTXModeSettingsKey =         "TXMode_Rover";
const char* Joystick::_vtolTXModeSettingsKey =          "TXMode_VTOL";
const char* Joystick::_submarineTXModeSettingsKey =     "TXMode_Submarine";
const char* Joystick::_gimbalSettingsKey =              "GimbalEnabled";

const char* Joystick::_buttonActionNone =               QT_TR_NOOP("No Action");
const char* Joystick::_buttonActionArm =                QT_TR_NOOP("Arm");
const char* Joystick::_buttonActionDisarm =             QT_TR_NOOP("Disarm");
const char* Joystick::_buttonActionToggleArm =          QT_TR_NOOP("Toggle Arm");
const char* Joystick::_buttonActionVTOLFixedWing =      QT_TR_NOOP("VTOL: Fixed Wing");
const char* Joystick::_buttonActionVTOLMultiRotor =     QT_TR_NOOP("VTOL: Multi-Rotor");
const char* Joystick::_buttonActionContinuousZoomIn =   QT_TR_NOOP("Continuous Zoom In");
const char* Joystick::_buttonActionContinuousZoomOut =  QT_TR_NOOP("Continuous Zoom Out");
const char* Joystick::_buttonActionStepZoomIn =         QT_TR_NOOP("Step Zoom In");
const char* Joystick::_buttonActionStepZoomOut =        QT_TR_NOOP("Step Zoom Out");
const char* Joystick::_buttonActionNextStream =         QT_TR_NOOP("Next Video Stream");
const char* Joystick::_buttonActionPreviousStream =     QT_TR_NOOP("Previous Video Stream");
const char* Joystick::_buttonActionNextCamera =         QT_TR_NOOP("Next Camera");
const char* Joystick::_buttonActionPreviousCamera =     QT_TR_NOOP("Previous Camera");
const char* Joystick::_buttonActionTriggerCamera =      QT_TR_NOOP("Trigger Camera");
const char* Joystick::_buttonActionToggleCameraMode =   QT_TR_NOOP("Toggle Camera Mode");
const char* Joystick::_buttonActionStartVideoRecord =   QT_TR_NOOP("Start Recording Video");
const char* Joystick::_buttonActionStopVideoRecord =    QT_TR_NOOP("Stop Recording Video");
const char* Joystick::_buttonActionToggleVideoRecord =  QT_TR_NOOP("Toggle Recording Video");
const char* Joystick::_buttonActionGimbalDown =         QT_TR_NOOP("Gimbal Down");
const char* Joystick::_buttonActionGimbalUp =           QT_TR_NOOP("Gimbal Up");
const char* Joystick::_buttonActionGimbalLeft =         QT_TR_NOOP("Gimbal Left");
const char* Joystick::_buttonActionGimbalRight =        QT_TR_NOOP("Gimbal Right");
const char* Joystick::_buttonActionGimbalCenter =       QT_TR_NOOP("Gimbal Center");
const char* Joystick::_buttonActionGimbalPitchUp =      QT_TR_NOOP("Gimbal Pitching Up");
const char* Joystick::_buttonActionGimbalPitchDown =    QT_TR_NOOP("Gimbal Pitching Down");
const char* Joystick::_buttonActionToggleThermal =      QT_TR_NOOP("Thermal ON/OFF");
const char* Joystick::_buttonActionThermalOn =          QT_TR_NOOP("Thermal ON");
const char* Joystick::_buttonActionThermalOff =         QT_TR_NOOP("Thermal OFF");
const char* Joystick::_buttonActionThermalNextPalette = QT_TR_NOOP("Thermal Next Palette");

const char* Joystick::_rgFunctionSettingsKey[Joystick::maxFunction] = {
    "RollAxis",
    "PitchAxis",
    "YawAxis",
    "ThrottleAxis",
    "GimbalPitchAxis",
    "GimbalYawAxis"
};

int Joystick::_transmitterMode = 2;

AssignedButtonAction::AssignedButtonAction(QObject* parent, const QString name)
    : QObject(parent)
    , action(name)
{
}

AssignableButtonAction::AssignableButtonAction(QObject* parent, QString action_, bool canRepeat_)
    : QObject(parent)
    , _action(action_)
    , _repeat(canRepeat_)
{
}


JoystickCustomAction::JoystickCustomAction(QObject *parent, int buttonCount, QStringList buttonOptions) :
    QObject(parent),
    _buttonCount(buttonCount),
    _buttonOptions(buttonOptions)
{
}

int JoystickCustomAction::button1()
{
    return _button1;
}

int JoystickCustomAction::button2()
{
    return _button2;
}

QString JoystickCustomAction::action()
{
    return _action;
}

int JoystickCustomAction::secondaryActionType()
{
    return _secondaryActionType;
}

QStringList JoystickCustomAction::buttonOptions()
{
    return _buttonOptions;
}

int JoystickCustomAction::selectedButtonIndex1()
{
    return _selectedButtonIndex1;
}

int JoystickCustomAction::selectedButtonIndex2()
{
    return _selectedButtonIndex2;
}

void JoystickCustomAction::setButton1(const int &buttonNumber)
{
    _button1 = buttonNumber;
    emit button1Changed();
}

void JoystickCustomAction::setButton2(const int &buttonNumber)
{
    _button2 = buttonNumber;
    emit button2Changed();
}

void JoystickCustomAction::setAction(const QString &action)
{
    _action = action;
    emit actionChanged();
}

void JoystickCustomAction::setSecondaryActionType(int secondaryActionType)
{
    _secondaryActionType = secondaryActionType;
    emit secondaryActionTypeChanged();
}

void JoystickCustomAction::setButtonOptions(QStringList options)
{
    _buttonOptions = options;
    emit buttonOptionsChanged();
}

void JoystickCustomAction::setSelectedButtonIndex1(int button)
{
    _selectedButtonIndex1 = button;
    emit selectedButtonIndex1Changed();
}

void JoystickCustomAction::setSelectedButtonIndex2(int button)
{
    _selectedButtonIndex2 = button;
    emit selectedButtonIndex2Changed();
}

void JoystickCustomAction::setCurrentButton1(QString button)
{
    _currentButton1 = button;
}

void JoystickCustomAction::setCurrentButton2(QString button)
{
    _currentButton2 = button;
}

void JoystickCustomAction::addButtonToOptionsList(int button)
{
    if(button < 0 || button > _buttonCount){
        return;
    }

    for(int i = 1; i < _buttonOptions.size(); i++){
        if(button == _buttonOptions[i].toInt()){
            break;
        } else if(button < _buttonOptions[i].toInt() || i == _buttonOptions.size()-1){
            _buttonOptions.insert(i, QString::number(button));
            break;
        }
    }
    _selectedButtonIndex1 = _buttonOptions.indexOf(_currentButton1);
    _selectedButtonIndex2 = _buttonOptions.indexOf(_currentButton2);

    emit buttonOptionsChanged();
    emit selectedButtonIndex1Changed();
    emit selectedButtonIndex2Changed();
}


void JoystickCustomAction::removeButtonFromOptionsList(int button)
{
    if(button < 0 || button > _buttonCount){
        return;
    }
    for(int i = 1; i < _buttonOptions.size(); i++){
        if(button == _buttonOptions[i].toInt()){
            _buttonOptions.removeAt(i);
            break;
        }
    }
    _selectedButtonIndex1 = _buttonOptions.indexOf(_currentButton1);
    _selectedButtonIndex2 = _buttonOptions.indexOf(_currentButton2);

    emit buttonOptionsChanged();
    emit selectedButtonIndex1Changed();
    emit selectedButtonIndex2Changed();
}

void JoystickCustomAction::updateCustomActionIndex()
{
    _selectedButtonIndex1 = _buttonOptions.indexOf(_currentButton1);
    _selectedButtonIndex2 = _buttonOptions.indexOf(_currentButton2);

    emit selectedButtonIndex1Changed();
    emit selectedButtonIndex2Changed();
}

Joystick::Joystick(const QString& name, int axisCount, int buttonCount, int hatCount, MultiVehicleManager* multiVehicleManager)
    : _name(name)
    , _axisCount(axisCount)
    , _buttonCount(buttonCount)
    , _hatCount(hatCount)
    , _hatButtonCount(4 * hatCount)
    , _totalButtonCount(_buttonCount+_hatButtonCount)
    , _multiVehicleManager(multiVehicleManager)
{
    _rgAxisValues   = new int[static_cast<size_t>(_axisCount)];
    _rgCalibration  = new Calibration_t[static_cast<size_t>(_axisCount)];
    _rgButtonValues = new uint8_t[static_cast<size_t>(_totalButtonCount)];
    for (int i = 0; i < _axisCount; i++) {
        _rgAxisValues[i] = 0;
    }
    for (int i = 0; i < _totalButtonCount; i++) {
        _rgButtonValues[i] = BUTTON_UP;
        _buttonActionArray.append(nullptr);
    }

    connect(_multiVehicleManager, &MultiVehicleManager::activeVehicleChanged, this, &Joystick::_activeVehicleChanged);
    emit buttonActionsChanged();
}

Joystick::~Joystick()
{
    // Crash out of the thread if it is still running
    terminate();
    wait();
    delete[] _rgAxisValues;
    delete[] _rgCalibration;
    delete[] _rgButtonValues;
    _assignableButtonActions.clearAndDeleteContents();
    for (int button = 0; button < _totalButtonCount; button++) {
        if(_buttonActionArray[button]) {
            _buttonActionArray[button]->deleteLater();
        }
    }
}

void Joystick::addButtonGlobalCustomActionsList(int button)
{
    if(button < 0 || button > _buttonCount){
        return;
    }
    for(int i = 1; i < _buttonOptions.size(); i++){
        if(button == _buttonOptions[i].toInt()){
            break;
        } else if(button < _buttonOptions[i].toInt()){
            _buttonOptions.insert(i, QString::number(button));
            break;
        }
    }
}

void Joystick::removeButtonGlobalCustomActionsList(int button)
{
    if(button < 0 || button > _buttonCount){
        return;
    }
    for(int i = 1; i < _buttonOptions.size(); i++){
        if(button == _buttonOptions[i].toInt()){
            _buttonOptions.removeAt(i);
            break;
        }
    }
}

void Joystick::addButtonLocalCustomActionsLists(int button, int customIndex)
{
    if(button < 0 || button > _buttonCount){
        return;
    }
    for(int i = 0; i < _customActions.size(); i++){
        JoystickCustomAction* customActionFromList = qobject_cast<JoystickCustomAction*>(_customActions[i]);
        if(i != customIndex){
            customActionFromList->addButtonToOptionsList(button);
        } else {
            customActionFromList->updateCustomActionIndex();
        }
    }
}

void Joystick::removeButtonLocalCustomActionsLists(int button, int customIndex)
{
    if(button < 0 || button > _buttonCount){
        return;
    }
    for(int i = 0; i < _customActions.size(); i++){
        JoystickCustomAction* customActionFromList = qobject_cast<JoystickCustomAction*>(_customActions[i]);

        if(i != customIndex){
            customActionFromList->removeButtonFromOptionsList(button);
        } else {
            customActionFromList->updateCustomActionIndex();
        }
    }
}

void Joystick::handleManualControlGimbal(float gimbalPitch, float gimbalYaw) {
    _localPitch += static_cast<double>(gimbalPitch);
    _localYaw += static_cast<double>(gimbalYaw);
    emit gimbalControlValue(_localPitch, _localYaw);
}

void Joystick::_setDefaultCalibration(void) {
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    _calibrated = settings.value(_calibratedSettingsKey, false).toBool();

    // Only set default calibrations if we do not have a calibration for this gamecontroller
    if(_calibrated) return;

    for(int axis = 0; axis < _axisCount; axis++) {
        Joystick::Calibration_t calibration;
        _rgCalibration[axis] = calibration;
    }

    _rgCalibration[1].reversed = true;
    _rgCalibration[3].reversed = true;

    // Default TX Mode 2 axis assignments for gamecontrollers
    _rgFunctionAxis[rollFunction]       = 2;
    _rgFunctionAxis[pitchFunction]      = 3;
    _rgFunctionAxis[yawFunction]        = 0;
    _rgFunctionAxis[throttleFunction]   = 1;

    _rgFunctionAxis[gimbalPitchFunction]= 4;
    _rgFunctionAxis[gimbalYawFunction]  = 5;

    _exponential    = 0;
    _accumulator    = false;
    _deadband       = false;
    _axisFrequency  = 25.0f;
    _buttonFrequency= 5.0f;
    _throttleMode   = ThrottleModeDownZero;
    _calibrated     = true;
    _circleCorrection = false;

    _saveSettings();
}

void Joystick::_updateTXModeSettingsKey(Vehicle* activeVehicle)
{
    if(activeVehicle) {
        if(activeVehicle->fixedWing()) {
            _txModeSettingsKey = _fixedWingTXModeSettingsKey;
        } else if(activeVehicle->multiRotor()) {
            _txModeSettingsKey = _multiRotorTXModeSettingsKey;
        } else if(activeVehicle->rover()) {
            _txModeSettingsKey = _roverTXModeSettingsKey;
        } else if(activeVehicle->vtol()) {
            _txModeSettingsKey = _vtolTXModeSettingsKey;
        } else if(activeVehicle->sub()) {
            _txModeSettingsKey = _submarineTXModeSettingsKey;
        } else {
            _txModeSettingsKey = nullptr;
            qWarning() << "No valid joystick TXmode settings key for selected vehicle";
            return;
        }
    } else {
        _txModeSettingsKey = nullptr;
    }
}

void Joystick::_activeVehicleChanged(Vehicle* activeVehicle)
{
    _updateTXModeSettingsKey(activeVehicle);
    if(activeVehicle) {
        _buildActionList(activeVehicle);
        _updateTXModeSettingsKey(activeVehicle);
        _loadSettings();
    }
}

void Joystick::_loadSettings()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    Vehicle* activeVehicle = _multiVehicleManager->activeVehicle();

    if(_txModeSettingsKey && activeVehicle)
        _transmitterMode = settings.value(_txModeSettingsKey, activeVehicle->firmwarePlugin()->defaultJoystickTXMode()).toInt();

    settings.beginGroup(_name);

    bool badSettings = false;
    bool convertOk;

    qCDebug(JoystickLog) << "_loadSettings " << _name;

    _calibrated     = settings.value(_calibratedSettingsKey, false).toBool();
    _exponential    = settings.value(_exponentialSettingsKey, 0).toFloat();
    _accumulator    = settings.value(_accumulatorSettingsKey, false).toBool();
    _deadband       = settings.value(_deadbandSettingsKey, false).toBool();
    _axisFrequency  = settings.value(_axisFrequencySettingsKey, 25.0f).toFloat();
    _buttonFrequency= settings.value(_buttonFrequencySettingsKey, 5.0f).toFloat();
    _circleCorrection = settings.value(_circleCorrectionSettingsKey, false).toBool();
    _gimbalEnabled  = settings.value(_gimbalSettingsKey, false).toBool();

    _throttleMode   = static_cast<ThrottleMode_t>(settings.value(_throttleModeSettingsKey, ThrottleModeDownZero).toInt(&convertOk));
    badSettings |= !convertOk;

    qCDebug(JoystickLog) << "_loadSettings calibrated:txmode:throttlemode:exponential:deadband:badsettings" << _calibrated << _transmitterMode << _throttleMode << _exponential << _deadband << badSettings;

    QString minTpl      ("Axis%1Min");
    QString maxTpl      ("Axis%1Max");
    QString trimTpl     ("Axis%1Trim");
    QString revTpl      ("Axis%1Rev");
    QString deadbndTpl  ("Axis%1Deadbnd");

    for (int axis = 0; axis < _axisCount; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];

        calibration->center = settings.value(trimTpl.arg(axis), 0).toInt(&convertOk);
        badSettings |= !convertOk;

        calibration->min = settings.value(minTpl.arg(axis), -32768).toInt(&convertOk);
        badSettings |= !convertOk;

        calibration->max = settings.value(maxTpl.arg(axis), 32767).toInt(&convertOk);
        badSettings |= !convertOk;

        calibration->deadband = settings.value(deadbndTpl.arg(axis), 0).toInt(&convertOk);
        badSettings |= !convertOk;

        calibration->reversed = settings.value(revTpl.arg(axis), false).toBool();

        qCDebug(JoystickLog) << "_loadSettings axis:min:max:trim:reversed:deadband:badsettings" << axis << calibration->min << calibration->max << calibration->center << calibration->reversed << calibration->deadband << badSettings;
    }

    int workingAxis = 0;
    for (int function = 0; function < maxFunction; function++) {
        int functionAxis;
        functionAxis = settings.value(_rgFunctionSettingsKey[function], -1).toInt(&convertOk);
        badSettings |= !convertOk || (functionAxis >= _axisCount);
        if(functionAxis >= 0) {
            workingAxis++;
        }
        if(functionAxis < _axisCount) {
            _rgFunctionAxis[function] = functionAxis;
        }
        qCDebug(JoystickLog) << "_loadSettings function:axis:badsettings" << function << functionAxis << badSettings;
    }
    badSettings |= workingAxis < 4;

    // FunctionAxis mappings are always stored in TX mode 2
    // Remap to stored TX mode in settings
    _remapAxes(2, _transmitterMode, _rgFunctionAxis);

    // Loop through all the button actions and secondary actions stored in the settings. We need to create the button object if one of the two is available or if
    // both are available. We just skip the button in case that no action is assigned.
    for (int button = 0; button < _totalButtonCount; button++) {
        QString a = settings.value(QString(_buttonActionNameKey).arg(button), QString()).toString();
        QString b = settings.value(QString(_buttonSecondaryActionNameKey).arg(button), QString()).toString();
        qCDebug(JoystickLog) << "Button" << button << QString(_buttonActionNameKey).arg(button) << a;
        // Do we have an action assigned to this button?
        if(!a.isEmpty()) {
            if(_findAssignableButtonAction(a) >= 0) {
                if(a != _buttonActionNone) {
                    if(_buttonActionArray[button]) {
                        _buttonActionArray[button]->deleteLater();
                    }
                    // Create an instance of this button passing the primary action to the constructor
                    AssignedButtonAction* ap = new AssignedButtonAction(this, a);
                    ap->repeat = settings.value(QString(_buttonActionRepeatKey).arg(button), false).toBool();
                    _buttonActionArray[button] = ap;
                    _buttonActionArray[button]->buttonTime.start();
                    // Do we also have a secondary action assigned to this button?
                    if(!b.isEmpty() && (_findAssignableButtonAction(b) >= 0) && (b != _buttonActionNone)){
                        _buttonActionArray[button]->secondary_action = b;
                        _buttonActionArray[button]->secondaryActionType = static_cast<AssignedButtonAction::SecondaryAction_t>(settings.value(QString(_buttonSecondaryActionTypeKey).arg(button), 0).toInt());
                        // If the secondary action is a combo we also need to fetch the button with with it should combine with
                        if(_buttonActionArray[button]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO) {
                            _buttonActionArray[button]->buttonCombo = settings.value(QString(_buttonComboKey).arg(button), 0).toInt();
                        }
                    }
                    qCDebug(JoystickLog) << "_loadSettings button:action" << button << _buttonActionArray[button]->action << _buttonActionArray[button]->repeat;
                }
            }
        // In case no primary action is assigned, do we have a secondary action assigned to this button?
        } else if (!b.isEmpty()){
            if(_findAssignableButtonAction(b) >= 0) {
                if(b != _buttonActionNone) {
                    if(_buttonActionArray[button]) {
                        _buttonActionArray[button]->deleteLater();
                    }
                    // Create an instance of this button leaving the primary action empty (TODO: make this cleaner)
                    AssignedButtonAction* ap = new AssignedButtonAction(this, _buttonActionNone);
                    ap->repeat = false;
                    _buttonActionArray[button] = ap;
                    _buttonActionArray[button]->buttonTime.start();
                    _buttonActionArray[button]->secondary_action = b;
                    _buttonActionArray[button]->secondaryActionType = static_cast<AssignedButtonAction::SecondaryAction_t>(settings.value(QString(_buttonSecondaryActionTypeKey).arg(button), 0).toInt());
                    // If the secondary action is a combo we also need to fetch the button with with it should combine with
                    if(_buttonActionArray[button]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO) {
                        _buttonActionArray[button]->buttonCombo = settings.value(QString(_buttonComboKey).arg(button), 0).toInt();
                    }
                    qCDebug(JoystickLog) << "_loadSettings button:secondaryaction" << button << _buttonActionArray[button]->action << _buttonActionArray[button]->repeat;
                }
            }
        }
    }

    if (badSettings) {
        _calibrated = false;
        settings.setValue(_calibratedSettingsKey, false);
    }
}

void Joystick::_saveButtonSettings()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    for (int button = 0; button < _totalButtonCount; button++) {
        if(_buttonActionArray[button]) {
            settings.setValue(QString(_buttonActionNameKey).arg(button), _buttonActionArray[button]->action);
            settings.setValue(QString(_buttonSecondaryActionNameKey).arg(button), _buttonActionArray[button]->secondary_action);
            settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
            settings.setValue(QString(_buttonComboKey).arg(button), _buttonActionArray[button]->buttonCombo);
            settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(button), _buttonActionArray[button]->secondaryActionType);
            qCDebug(JoystickLog) << "_saveButtonSettings button:action:secondary_action:repeat:buttonCombo:secondaryActionType" << button 
                    << _buttonActionArray[button]->action << _buttonActionArray[button]->secondary_action << _buttonActionArray[button]->repeat
                    << _buttonActionArray[button]->buttonCombo << _buttonActionArray[button]->secondaryActionType;
        }
    }
}

void Joystick::_saveSettings()
{
    QSettings settings;
    settings.beginGroup(_settingsGroup);

    // Transmitter mode is static
    // Save the mode we are using
    if(_txModeSettingsKey)
        settings.setValue(_txModeSettingsKey,       _transmitterMode);

    settings.beginGroup(_name);
    settings.setValue(_calibratedSettingsKey,       _calibrated);
    settings.setValue(_exponentialSettingsKey,      _exponential);
    settings.setValue(_accumulatorSettingsKey,      _accumulator);
    settings.setValue(_deadbandSettingsKey,         _deadband);
    settings.setValue(_axisFrequencySettingsKey,    _axisFrequency);
    settings.setValue(_buttonFrequencySettingsKey,  _buttonFrequency);
    settings.setValue(_throttleModeSettingsKey,     _throttleMode);
    settings.setValue(_gimbalSettingsKey,           _gimbalEnabled);
    settings.setValue(_circleCorrectionSettingsKey, _circleCorrection);

    qCDebug(JoystickLog) << "_saveSettings calibrated:throttlemode:deadband:txmode" << _calibrated << _throttleMode << _deadband << _circleCorrection << _transmitterMode;

    QString minTpl      ("Axis%1Min");
    QString maxTpl      ("Axis%1Max");
    QString trimTpl     ("Axis%1Trim");
    QString revTpl      ("Axis%1Rev");
    QString deadbndTpl  ("Axis%1Deadbnd");

    for (int axis = 0; axis < _axisCount; axis++) {
        Calibration_t* calibration = &_rgCalibration[axis];
        settings.setValue(trimTpl.arg(axis), calibration->center);
        settings.setValue(minTpl.arg(axis), calibration->min);
        settings.setValue(maxTpl.arg(axis), calibration->max);
        settings.setValue(revTpl.arg(axis), calibration->reversed);
        settings.setValue(deadbndTpl.arg(axis), calibration->deadband);
        qCDebug(JoystickLog) << "_saveSettings name:axis:min:max:trim:reversed:deadband"
                                << _name
                                << axis
                                << calibration->min
                                << calibration->max
                                << calibration->center
                                << calibration->reversed
                                << calibration->deadband;
    }

    // Always save function Axis mappings in TX Mode 2
    // Write mode 2 mappings without changing mapping currently in use
    int temp[maxFunction];
    _remapAxes(_transmitterMode, 2, temp);
    for (int function = 0; function < maxFunction; function++) {
        settings.setValue(_rgFunctionSettingsKey[function], temp[function]);
        qCDebug(JoystickLog) << "_saveSettings name:function:axis" << _name << function << _rgFunctionSettingsKey[function];
    }
    _saveButtonSettings();
}

// Relative mappings of axis functions between different TX modes
int Joystick::_mapFunctionMode(int mode, int function) {
    static const int mapping[][6] = {
        { yawFunction, pitchFunction, rollFunction, throttleFunction, gimbalPitchFunction, gimbalYawFunction },
        { yawFunction, throttleFunction, rollFunction, pitchFunction, gimbalPitchFunction, gimbalYawFunction },
        { rollFunction, pitchFunction, yawFunction, throttleFunction, gimbalPitchFunction, gimbalYawFunction },
        { rollFunction, throttleFunction, yawFunction, pitchFunction, gimbalPitchFunction, gimbalYawFunction }};
    return mapping[mode-1][function];
}

// Remap current axis functions from current TX mode to new TX mode
void Joystick::_remapAxes(int currentMode, int newMode, int (&newMapping)[maxFunction]) {
    int temp[maxFunction];
    for(int function = 0; function < maxFunction; function++) {
        temp[_mapFunctionMode(newMode, function)] = _rgFunctionAxis[_mapFunctionMode(currentMode, function)];
    }
    for(int function = 0; function < maxFunction; function++) {
        newMapping[function] = temp[function];
    }
}

void Joystick::setTXMode(int mode) {
    if(mode > 0 && mode <= 4) {
        if(_transmitterMode != mode) {
            _remapAxes(_transmitterMode, mode, _rgFunctionAxis);
            _transmitterMode = mode;
            _saveSettings();
        }
    } else {
        qCWarning(JoystickLog) << "Invalid mode:" << mode;
    }
}

/// Adjust the raw axis value to the -1:1 range given calibration information
float Joystick::_adjustRange(int value, Calibration_t calibration, bool withDeadbands)
{
    float valueNormalized;
    float axisLength;
    float axisBasis;

    if (value > calibration.center) {
        axisBasis = 1.0f;
        valueNormalized = value - calibration.center;
        axisLength =  calibration.max - calibration.center;
    } else {
        axisBasis = -1.0f;
        valueNormalized = calibration.center - value;
        axisLength =  calibration.center - calibration.min;
    }

    float axisPercent;

    if (withDeadbands) {
        if (valueNormalized>calibration.deadband) {
            axisPercent = (valueNormalized - calibration.deadband) / (axisLength - calibration.deadband);
        } else if (valueNormalized<-calibration.deadband) {
            axisPercent = (valueNormalized + calibration.deadband) / (axisLength - calibration.deadband);
        } else {
            axisPercent = 0.f;
        }
    }
    else {
        axisPercent = valueNormalized / axisLength;
    }

    float correctedValue = axisBasis * axisPercent;

    if (calibration.reversed) {
        correctedValue *= -1.0f;
    }

#if 0
    qCDebug(JoystickLog) << "_adjustRange corrected:value:min:max:center:reversed:deadband:basis:normalized:length"
                            << correctedValue
                            << value
                            << calibration.min
                            << calibration.max
                            << calibration.center
                            << calibration.reversed
                            << calibration.deadband
                            << axisBasis
                            << valueNormalized
                            << axisLength;
#endif

    return std::max(-1.0f, std::min(correctedValue, 1.0f));
}


void Joystick::run()
{
    //-- Joystick thread
    _open();
    //-- Reset timers
    _axisTime.start();
    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        if(_buttonActionArray[buttonIndex]) {
            _buttonActionArray[buttonIndex]->buttonTime.start();
        }
    }
    while (!_exitThread) {
        _update();
        _handleButtons();
        _handleAxis();
        QGC::SLEEP::msleep(20);
    }
    _close();
}

void Joystick::_handleButtons()
{
    //-- Update button states
    for (int buttonIndex = 0; buttonIndex < _buttonCount; buttonIndex++) {
        bool newButtonValue = _getButton(buttonIndex);
        if (newButtonValue && _rgButtonValues[buttonIndex] == BUTTON_UP) {
            _rgButtonValues[buttonIndex] = BUTTON_DOWN;
            emit rawButtonPressedChanged(buttonIndex, newButtonValue);
        } else if (!newButtonValue && _rgButtonValues[buttonIndex] != BUTTON_UP) {
            _rgButtonValues[buttonIndex] = BUTTON_UP;
            emit rawButtonPressedChanged(buttonIndex, newButtonValue);
        }
    }
    //-- Update hat - append hat buttons to the end of the normal button list
    int numHatButtons = 4;
    for (int hatIndex = 0; hatIndex < _hatCount; hatIndex++) {
        for (int hatButtonIndex = 0; hatButtonIndex<numHatButtons; hatButtonIndex++) {
            // Create new index value that includes the normal button list
            int rgButtonValueIndex = hatIndex*numHatButtons + hatButtonIndex + _buttonCount;
            // Get hat value from joystick
            bool newButtonValue = _getHat(hatIndex, hatButtonIndex);
            if (newButtonValue && _rgButtonValues[rgButtonValueIndex] == BUTTON_UP) {
                _rgButtonValues[rgButtonValueIndex] = BUTTON_DOWN;
                emit rawButtonPressedChanged(rgButtonValueIndex, newButtonValue);
            } else if (!newButtonValue && _rgButtonValues[rgButtonValueIndex] != BUTTON_UP) {
                _rgButtonValues[rgButtonValueIndex] = BUTTON_UP;
                emit rawButtonPressedChanged(rgButtonValueIndex, newButtonValue);
            }
        }
    }

    //-- Process button press/release
    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        if(_buttonActionArray[buttonIndex]) {
            QString buttonAction = _buttonActionArray[buttonIndex]->action;
            QString buttonSecondaryAction = _buttonActionArray[buttonIndex]->secondary_action;

            bool hasAction = !(buttonAction.isEmpty() || buttonAction == _buttonActionNone);
            bool hasSecondaryAction = !(buttonSecondaryAction.isEmpty() || buttonSecondaryAction == _buttonActionNone || (_buttonActionArray[buttonIndex]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE));
            
            // At least one action has to be available on this button in order to move forward
            if(!hasAction && !hasSecondaryAction){
                continue;
            }
            
            int double_press_time_threshold = 350; //ms
            int long_press_time_threshold = 800; //ms
            int combo_press_time_threshold = 200; //ms
            int repeat_press_time_threshold = static_cast<int>(1000.0f / _buttonFrequency);

            // Finite State Machine to handle all types of button actions (single, repeat, double, long or combo presses)
            switch(_buttonActionArray[buttonIndex]->buttonCurrentState) {
                // BUTTON_IDLE_STATE: This is the idle state in which all buttons are initialized in and where all buttons end up after and action
                // is completed. From this state all the buttons go to BUTTON_DOWN_STATE once they are pressed.
                case AssignedButtonAction::BUTTON_IDLE_STATE:
                    if(_rgButtonValues[buttonIndex] == BUTTON_DOWN) {
                        // Start counter on falling edge of the button
                        _buttonActionArray[buttonIndex]->buttonTime.start();
                        _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DOWN_STATE;
                    }
                    break;
                // BUTTON_DOWN_STATE: This state is common to all button functionalities (single, repeat, double and long press). It is triggered every time a 
                // button is pressed down from the BUTTON_IDLE_STATE.
                case AssignedButtonAction::BUTTON_DOWN_STATE:
                    switch(_buttonActionArray[buttonIndex]->secondaryActionType){
                        // SINGLE PRESS OR REPEAT PRESS: No other type of button press is assigned to a button so as soon as we press on a button it will
                        // directly execute the action without waiting for other button actions. If in repeat mode it will repeat the action until the button 
                        // is released.
                        case AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE:
                            _executeButtonAction(buttonAction, true);
                            qCDebug(JoystickLog) << "Single button triggered" << buttonIndex << buttonAction;
                            if(!_buttonActionArray[buttonIndex]->repeat){
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                            } else {
                                _buttonActionArray[buttonIndex]->buttonTime.start();
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DOWNREPEAT_STATE;
                            }
                            break;
                        // DOUBLE PRESS: Press button twice in less that 500ms. Pressing it only once will automatically trigger a short press action as soon
                        // as the 500ms timeout for the double press expires. In this case the single press' delay will always be 500ms plus the delay of the loop.
                        case AssignedButtonAction::SECONDARY_BUTTON_PRESS_DOUBLE:
                            if(_buttonActionArray[buttonIndex]->buttonTime.elapsed() > double_press_time_threshold) {
                                _executeButtonAction(buttonAction, true);
                                qCDebug(JoystickLog) << "Single button triggered" << buttonIndex << buttonAction;
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                            } else if(_rgButtonValues[buttonIndex] == BUTTON_UP){
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DOWNUP_STATE;
                            } else {
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DOWN_STATE;
                            }
                            break;
                        // LONG PRESS: Hold button down for more that 1 second. Holding it less that 1 second will automatically trigger a short press action
                        // as soon as the button is released. In this case the short press' delay is affected by the time it takes the user to 
                        // release the button.
                        case AssignedButtonAction::SECONDARY_BUTTON_PRESS_LONG:
                            if(_rgButtonValues[buttonIndex] == BUTTON_UP){
                                _executeButtonAction(buttonAction, true);
                                qCDebug(JoystickLog) << "Single button triggered" << buttonIndex << buttonAction;
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                            } else if(_buttonActionArray[buttonIndex]->buttonTime.elapsed() > long_press_time_threshold) {
                                _executeButtonAction(buttonSecondaryAction, true);
                                qCDebug(JoystickLog) << "Long button triggered" << buttonIndex << buttonSecondaryAction;
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                            } else {
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DOWN_STATE;
                            }
                            break;
                        // COMBO PRESS: Press button together with its assigned combo button. Since the two buttons are pressed with two different fingers there
                        // is a chance that the user doesn't press them simultaneously so we add a timeout after which the combo is not valid anymore and the user
                        // will then execute two single presses instead of a combo. Since both buttons will be marked as combo as soon as we execute the first one
                        // we will change the state also for the second one so that we don't execute the combo twice.
                        case AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO:
                            if(_buttonActionArray[buttonIndex]->buttonTime.elapsed() > combo_press_time_threshold) {
                                _executeButtonAction(buttonAction, true);
                                qCDebug(JoystickLog) << "Single button triggered" << buttonIndex << buttonAction;
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                            } else if(_buttonActionArray[_buttonActionArray[buttonIndex]->buttonCombo]->buttonCurrentState == AssignedButtonAction::BUTTON_DOWN_STATE){
                                _executeButtonAction(buttonSecondaryAction, true);
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                                _buttonActionArray[_buttonActionArray[buttonIndex]->buttonCombo]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                                qCDebug(JoystickLog) << "Combo button triggered" << buttonIndex << ":" << _buttonActionArray[buttonIndex]->buttonCombo << buttonSecondaryAction;
                            } else {
                                _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DOWN_STATE;
                            }
                            break;
                        // UNKNOWN: Go back to the idle state as safety precaution
                        default:
                            _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_IDLE_STATE;
                            break;
                    }
                    break;
                // BUTTON_DOWNREPEAT_STATE: this is the state where a button cycles in while it's repeating an action. When the button is pulled up
                // the action is over.
                case AssignedButtonAction::BUTTON_DOWNREPEAT_STATE:
                    if(_buttonActionArray[buttonIndex]->buttonTime.elapsed() > repeat_press_time_threshold) {
                        _executeButtonAction(buttonAction, true);
                        _buttonActionArray[buttonIndex]->buttonTime.start();
                        qCDebug(JoystickLog) << "Repeat button triggered" << buttonIndex << buttonAction;
                    }
                    if(_rgButtonValues[buttonIndex] == BUTTON_UP){
                        _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                    } else {
                        _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DOWNREPEAT_STATE;
                    }
                    break;
                // BUTTON_DOWNUP_STATE: this is the state where a button has just been released and we will evaluate if this will be a double press (button has 
                // will be pressed again) or if it will timeout and result in a single action.
                case AssignedButtonAction::BUTTON_DOWNUP_STATE:
                    if(_buttonActionArray[buttonIndex]->buttonTime.elapsed() > double_press_time_threshold) {
                        _executeButtonAction(buttonAction, true);
                        qCDebug(JoystickLog) << "Single button triggered" << buttonIndex << buttonAction;
                        _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                    } else if(_rgButtonValues[buttonIndex] == BUTTON_DOWN){
                        _executeButtonAction(buttonSecondaryAction, true);
                        qCDebug(JoystickLog) << "Double button triggered" << buttonIndex << buttonSecondaryAction;
                        _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                    } else {
                        _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DOWNUP_STATE;
                    }
                    break;
                // BUTTON_DONE_STATE: this is the state where a button has completed it's action but we don't want to put it back to IDLE before the button
                // will be released. Without this state we would go back to idle and repeat the action if the button is still pressed down.
                case AssignedButtonAction::BUTTON_DONE_STATE:
                    if(_rgButtonValues[buttonIndex] == BUTTON_UP){
                        if(_buttonActionArray[buttonIndex]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE){
                            // Actions when releasing the button are only available if no secondary action is assigned to a buttons
                            _executeButtonAction(buttonAction, false);
                        }
                        _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_IDLE_STATE;
                    } else {
                        _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_DONE_STATE;
                    }
                    break;
                default:
                    _buttonActionArray[buttonIndex]->buttonCurrentState = AssignedButtonAction::BUTTON_IDLE_STATE;
                    break;
            }
        }
    }
}

void Joystick::_handleAxis()
{
    //-- Get frequency
    int axisDelay = static_cast<int>(1000.0f / _axisFrequency);
    //-- Check elapsed time since last run
    if(_axisTime.elapsed() > axisDelay) {
        _axisTime.start();
        //-- Update axis
        for (int axisIndex = 0; axisIndex < _axisCount; axisIndex++) {
            int newAxisValue = _getAxis(axisIndex);
            // Calibration code requires signal to be emitted even if value hasn't changed
            _rgAxisValues[axisIndex] = newAxisValue;
            emit rawAxisValueChanged(axisIndex, newAxisValue);
        }
        if (_activeVehicle->joystickEnabled() && !_calibrationMode && _calibrated) {
            int     axis = _rgFunctionAxis[rollFunction];
            float   roll = _adjustRange(_rgAxisValues[axis],    _rgCalibration[axis], _deadband);

                    axis = _rgFunctionAxis[pitchFunction];
            float   pitch = _adjustRange(_rgAxisValues[axis],   _rgCalibration[axis], _deadband);

                    axis = _rgFunctionAxis[yawFunction];
            float   yaw = _adjustRange(_rgAxisValues[axis],     _rgCalibration[axis],_deadband);

                    axis = _rgFunctionAxis[throttleFunction];
            float   throttle = _adjustRange(_rgAxisValues[axis],_rgCalibration[axis], _throttleMode==ThrottleModeDownZero?false:_deadband);

            float   gimbalPitch = 0.0f;
            float   gimbalYaw   = 0.0f;

            if(_axisCount > 4) {
                axis = _rgFunctionAxis[gimbalPitchFunction];
                gimbalPitch = _adjustRange(_rgAxisValues[axis], _rgCalibration[axis],_deadband);
            }

            if(_axisCount > 5) {
                axis = _rgFunctionAxis[gimbalYawFunction];
                gimbalYaw = _adjustRange(_rgAxisValues[axis],   _rgCalibration[axis],_deadband);
            }

            if (_accumulator) {
                static float throttle_accu = 0.f;
                throttle_accu += throttle * (40 / 1000.f); //for throttle to change from min to max it will take 1000ms (40ms is a loop time)
                throttle_accu = std::max(static_cast<float>(-1.f), std::min(throttle_accu, static_cast<float>(1.f)));
                throttle = throttle_accu;
            }

            if (_circleCorrection) {
                float roll_limited      = std::max(static_cast<float>(-M_PI_4), std::min(roll,      static_cast<float>(M_PI_4)));
                float pitch_limited     = std::max(static_cast<float>(-M_PI_4), std::min(pitch,     static_cast<float>(M_PI_4)));
                float yaw_limited       = std::max(static_cast<float>(-M_PI_4), std::min(yaw,       static_cast<float>(M_PI_4)));
                float throttle_limited  = std::max(static_cast<float>(-M_PI_4), std::min(throttle,  static_cast<float>(M_PI_4)));

                // Map from unit circle to linear range and limit
                roll =      std::max(-1.0f, std::min(tanf(asinf(roll_limited)),     1.0f));
                pitch =     std::max(-1.0f, std::min(tanf(asinf(pitch_limited)),    1.0f));
                yaw =       std::max(-1.0f, std::min(tanf(asinf(yaw_limited)),      1.0f));
                throttle =  std::max(-1.0f, std::min(tanf(asinf(throttle_limited)), 1.0f));
            }

            if ( _exponential < -0.01f) {
                // Exponential (0% to -50% range like most RC radios)
                // _exponential is set by a slider in joystickConfigAdvanced.qml
                // Calculate new RPY with exponential applied
                roll =      -_exponential*powf(roll, 3) + (1+_exponential)*roll;
                pitch =     -_exponential*powf(pitch,3) + (1+_exponential)*pitch;
                yaw =       -_exponential*powf(yaw,  3) + (1+_exponential)*yaw;
            }

            // Adjust throttle to 0:1 range
            if (_throttleMode == ThrottleModeCenterZero && _activeVehicle->supportsThrottleModeCenterZero()) {
                if (!_activeVehicle->supportsNegativeThrust() || !_negativeThrust) {
                    throttle = std::max(0.0f, throttle);
                }
            } else {
                throttle = (throttle + 1.0f) / 2.0f;
            }
            qCDebug(JoystickValuesLog) << "name:roll:pitch:yaw:throttle:gimbalPitch:gimbalYaw" << name() << roll << -pitch << yaw << throttle << gimbalPitch << gimbalYaw;
            // NOTE: The buttonPressedBits going to MANUAL_CONTROL are currently used by ArduSub (and it only handles 16 bits)
            // Set up button bitmap
            quint64 buttonPressedBits = 0;  // Buttons pressed for manualControl signal
            for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
                quint64 buttonBit = static_cast<quint64>(1LL << buttonIndex);
                if (_rgButtonValues[buttonIndex] != BUTTON_UP) {
                    // Mark the button as pressed as long as its pressed
                    buttonPressedBits |= buttonBit;
                }
            }
            uint16_t shortButtons = static_cast<uint16_t>(buttonPressedBits & 0xFFFF);
            emit manualControl(roll, -pitch, yaw, throttle, shortButtons, _activeVehicle->joystickMode());
            if(_activeVehicle && _axisCount > 4 && _gimbalEnabled) {
                //-- TODO: There is nothing consuming this as there are no messages to handle gimbal
                //   the way MANUAL_CONTROL handles the other channels.
                emit manualControlGimbal(gimbalPitch, gimbalYaw);
            }
        }
    }
}

void Joystick::startPolling(Vehicle* vehicle)
{
    if (vehicle) {
        // If a vehicle is connected, disconnect it
        if (_activeVehicle) {
            UAS* uas = _activeVehicle->uas();
            disconnect(this, &Joystick::manualControl, uas, &UAS::setExternalControlSetpoint);
            disconnect(this, &Joystick::setArmed,           _activeVehicle, &Vehicle::setArmed);
            disconnect(this, &Joystick::setVtolInFwdFlight, _activeVehicle, &Vehicle::setVtolInFwdFlight);
            disconnect(this, &Joystick::setFlightMode,      _activeVehicle, &Vehicle::setFlightMode);
            // Commenting out for the time being as we already have a gimbal control source in qml that is conflicting with this
            // TODO: Implement a proper gimbal manager with support for mutliple sources based on priorities and redirect
            // the events to that
            //disconnect(this, &Joystick::gimbalPitchStep,    _activeVehicle, &Vehicle::gimbalPitchStep);
            //disconnect(this, &Joystick::gimbalYawStep,      _activeVehicle, &Vehicle::gimbalYawStep);
            //disconnect(this, &Joystick::centerGimbal,       _activeVehicle, &Vehicle::centerGimbal);
            //disconnect(this, &Joystick::gimbalControlValue, _activeVehicle, &Vehicle::gimbalControlValue);
            disconnect(this, &Joystick::manualControlGimbal, this,          &Joystick::handleManualControlGimbal);
        }
        // Always set up the new vehicle
        _activeVehicle = vehicle;
        // If joystick is not calibrated, disable it
        if ( !_calibrated ) {
            vehicle->setJoystickEnabled(false);
        }
        // Update qml in case of joystick transition
        emit calibratedChanged(_calibrated);
        // Build action list
        _buildActionList(vehicle);
        // Only connect the new vehicle if it wants joystick data
        if (vehicle->joystickEnabled()) {
            _pollingStartedForCalibration = false;
            UAS* uas = _activeVehicle->uas();
            connect(this, &Joystick::manualControl, uas, &UAS::setExternalControlSetpoint);
            connect(this, &Joystick::setArmed,           _activeVehicle, &Vehicle::setArmed);
            connect(this, &Joystick::setVtolInFwdFlight, _activeVehicle, &Vehicle::setVtolInFwdFlight);
            connect(this, &Joystick::setFlightMode,      _activeVehicle, &Vehicle::setFlightMode);
            /*connect(this, &Joystick::gimbalPitchStep,    _activeVehicle, &Vehicle::gimbalPitchStep);
            connect(this, &Joystick::gimbalYawStep,      _activeVehicle, &Vehicle::gimbalYawStep);
            connect(this, &Joystick::centerGimbal,       _activeVehicle, &Vehicle::centerGimbal);
            connect(this, &Joystick::gimbalControlValue, _activeVehicle, &Vehicle::gimbalControlValue);*/
            connect(this, &Joystick::manualControlGimbal, this,          &Joystick::handleManualControlGimbal);
            // FIXME: ****
            //connect(this, &Joystick::buttonActionTriggered, uas, &UAS::triggerAction);
        }
    }
    if (!isRunning()) {
        _exitThread = false;
        start();
    }
}

void Joystick::stopPolling(void)
{
    if (isRunning()) {
        if (_activeVehicle && _activeVehicle->joystickEnabled()) {
            UAS* uas = _activeVehicle->uas();
            // Neutral attitude controls
            // emit manualControl(0, 0, 0, 0.5, 0, _activeVehicle->joystickMode());
            disconnect(this, &Joystick::manualControl, uas, &UAS::setExternalControlSetpoint);
            disconnect(this, &Joystick::setArmed,           _activeVehicle, &Vehicle::setArmed);
            disconnect(this, &Joystick::setVtolInFwdFlight, _activeVehicle, &Vehicle::setVtolInFwdFlight);
            disconnect(this, &Joystick::setFlightMode,      _activeVehicle, &Vehicle::setFlightMode);
            /*disconnect(this, &Joystick::gimbalPitchStep,    _activeVehicle, &Vehicle::gimbalPitchStep);
            disconnect(this, &Joystick::gimbalYawStep,      _activeVehicle, &Vehicle::gimbalYawStep);
            disconnect(this, &Joystick::centerGimbal,       _activeVehicle, &Vehicle::centerGimbal);
            disconnect(this, &Joystick::gimbalControlValue, _activeVehicle, &Vehicle::gimbalControlValue);*/
            disconnect(this, &Joystick::manualControlGimbal, this,          &Joystick::handleManualControlGimbal);
        }
        // FIXME: ****
        //disconnect(this, &Joystick::buttonActionTriggered,  uas, &UAS::triggerAction);
        _exitThread = true;
    }
}

void Joystick::setCalibration(int axis, Calibration_t& calibration)
{
    if (!_validAxis(axis)) {
        return;
    }
    _calibrated = true;
    _rgCalibration[axis] = calibration;
    _saveSettings();
    emit calibratedChanged(_calibrated);
}

Joystick::Calibration_t Joystick::getCalibration(int axis)
{
    if (!_validAxis(axis)) {
        return Calibration_t();
    }
    return _rgCalibration[axis];
}

void Joystick::setFunctionAxis(AxisFunction_t function, int axis)
{
    if (!_validAxis(axis)) {
        return;
    }
    _calibrated = true;
    _rgFunctionAxis[function] = axis;
    _saveSettings();
    emit calibratedChanged(_calibrated);
}

int Joystick::getFunctionAxis(AxisFunction_t function)
{
    if (static_cast<int>(function) < 0 || function >= maxFunction) {
        qCWarning(JoystickLog) << "Invalid function" << function;
    }
    return _rgFunctionAxis[function];
}

bool Joystick::getButtonRepeat(int button)
{
    if (!_validButton(button) || !_buttonActionArray[button]) {
        return false;
    }
    return _buttonActionArray[button]->repeat;
}

void Joystick::setButtonRepeat(int button, bool repeat)
{
    if (!_validButton(button) || !_buttonActionArray[button]) {
        return;
    }
    _buttonActionArray[button]->repeat = repeat;
    if(repeat){
        // If this was a combo remove combo also on the combo button 
        if(_buttonActionArray[button]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO){
            setButtonCombo(button, _buttonActionArray[button]->buttonCombo, "", false);
            removeButtonComboTable(button, true);
        } else {
            _buttonActionArray[button]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE;
            removeButtonComboTable(button, false);
        }
        qCWarning(JoystickLog) << "set Button REPEAT!!!";
    }

    //-- Save to settings
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
    settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(button), _buttonActionArray[button]->secondaryActionType);
}

void Joystick::setButtonDouble(int button, bool doublePress)
{
    if (!_validButton(button) || !_buttonActionArray[button]) {
        qCWarning(JoystickLog) << "set Button DOUBLE failed... INDEX ISSUE" << button;
        return;
    }

    if(doublePress){
        // Disable repeat mode
        _buttonActionArray[button]->repeat = false;
        // If this was a combo remove combo also on the combo button 
        if(_buttonActionArray[button]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO){
            removeButtonComboTable(button, true);
        }
        _buttonActionArray[button]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_DOUBLE;
        qCDebug(JoystickLog) << "set Button DOUBLE!!!";
    } else {
        if(_buttonActionArray[button]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_DOUBLE) {
            _buttonActionArray[button]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE;
            qCDebug(JoystickLog) << "unset Button DOUBLE!!!";
        }
    }

    emit buttonRepeatChanged(button, _buttonActionArray[button]->repeat);

    //-- Save to settings
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(button), _buttonActionArray[button]->secondaryActionType);
    settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
}

void Joystick::setButtonLong(int button, bool longPress)
{
    if (!_validButton(button) || !_buttonActionArray[button]) {
        qCWarning(JoystickLog) << "set Button LONG failed... INDEX ISSUE" << button;
        return;
    }

    if(longPress){
        // Disable repeat mode
        _buttonActionArray[button]->repeat = false;
        // If this was a combo remove combo also on the combo button 
        if(_buttonActionArray[button]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO){
            removeButtonComboTable(button, true);
        }
        _buttonActionArray[button]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_LONG;
        qCDebug(JoystickLog) << "set Button LONG!!!";
    } else {
        if(_buttonActionArray[button]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_LONG) {
            _buttonActionArray[button]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE;
            qCDebug(JoystickLog) << "unset Button LONG!!!";
        }
    }

    emit buttonRepeatChanged(button, _buttonActionArray[button]->repeat);

    //-- Save to settings
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(button), _buttonActionArray[button]->secondaryActionType);
    settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
}

void Joystick::setButtonCombo(int button, int comboButton, const QString& action, bool comboPress)
{
    if (!_validButton(button) || !_validButton(comboButton)) {
        qCWarning(JoystickLog) << "set Button COMBO failed... INDEX ISSUE" << button;
        return;
    }

    if(button == comboButton) {
        qCDebug(JoystickLog) << "two buttons in combo cannot be the same";
        return;
    }

    if(comboPress){
        setButtonAction(button, action, false);
        setButtonAction(comboButton, action, false);
        _buttonActionArray[button]->repeat = false;
        _buttonActionArray[comboButton]->repeat = false;
        _buttonActionArray[button]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO;
        _buttonActionArray[comboButton]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO;
        _buttonActionArray[button]->buttonCombo = comboButton;
        _buttonActionArray[comboButton]->buttonCombo = button;
        qCDebug(JoystickLog) << "set Button COMBO!!!";
    } else {
        if (!_buttonActionArray[button] || !_buttonActionArray[comboButton]) {
            return;
        }
        if(_buttonActionArray[button]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO) {
            _buttonActionArray[button]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE;
            _buttonActionArray[comboButton]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE;
            qCDebug(JoystickLog) << "unset Button COMBO!!!";        }
    }

    emit buttonRepeatChanged(button, _buttonActionArray[button]->repeat);
    emit buttonRepeatChanged(comboButton, _buttonActionArray[comboButton]->repeat);

    //-- Save to settings
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(button), _buttonActionArray[button]->secondaryActionType);
    settings.setValue(QString(_buttonComboKey).arg(button), _buttonActionArray[button]->buttonCombo);
    settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(comboButton), _buttonActionArray[comboButton]->secondaryActionType);
    settings.setValue(QString(_buttonComboKey).arg(comboButton), _buttonActionArray[comboButton]->buttonCombo);
    settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
    settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);

}

void Joystick::setButtonAction(int button, const QString& action, bool primaryAction)
{
    if (!_validButton(button) || action.isEmpty()) {
        qCWarning(JoystickLog) << "set Button Action failed... INDEX ISSUE" << button;
        return;
    }
    qCInfo(JoystickLog) << "set Button Action:" << button << action;
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);
    // If the action we want to set is "No Action"
    if(action == _buttonActionNone) {
        if(_buttonActionArray[button]) {
            bool noActionLeft = (primaryAction & _buttonActionArray[button]->secondary_action.isEmpty()) |
                                (!primaryAction & _buttonActionArray[button]->action.isEmpty());
            // Both primary and secondary actions are disabled for this button so we can delete this button object
            if(noActionLeft){
                _buttonActionArray[button]->deleteLater();
                _buttonActionArray[button] = nullptr;
                //-- Clear from settings
                if(primaryAction){
                    settings.remove(QString(_buttonActionNameKey).arg(button));
                    settings.remove(QString(_buttonActionRepeatKey).arg(button));
                } else {
                    settings.remove(QString(_buttonSecondaryActionNameKey).arg(button));
                    settings.remove(QString(_buttonSecondaryActionTypeKey).arg(button));
                    settings.remove(QString(_buttonComboKey).arg(button));
                }
            // Update the action for this button to "No Action". One of the two actions will still be set after this call.
            } else {
                if(primaryAction){
                    _buttonActionArray[button]->action = action;
                    // Disable also repeat when primary action is unset
                    if(action == _buttonActionNone){
                        _buttonActionArray[button]->repeat = false;
                    }
                    settings.remove(QString(_buttonActionNameKey).arg(button));
                    settings.remove(QString(_buttonActionRepeatKey).arg(button));
                } else {
                    _buttonActionArray[button]->secondary_action = action;
                    // Disable also secondary action type when secondary action is unset
                    if(action == _buttonActionNone){
                        _buttonActionArray[button]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE;
                    }
                    settings.remove(QString(_buttonSecondaryActionNameKey).arg(button));
                    settings.remove(QString(_buttonSecondaryActionTypeKey).arg(button));
                    settings.remove(QString(_buttonComboKey).arg(button));
                }
            }
        }
    // If the action we want to set is different from "No Action"
    } else {
        // If no action was assigned to this button then we need to create a new object and fill it up with the primary or secondary action
        // passed to this method
        if(!_buttonActionArray[button]) {
            if(primaryAction){
                _buttonActionArray[button] = new AssignedButtonAction(this, action);
            } else {
                _buttonActionArray[button] = new AssignedButtonAction(this, _buttonActionNone);
                if(_buttonActionArray[button]){
                    _buttonActionArray[button]->secondary_action = action;
                }
            }
        // If an action was assigned to this button then we can just update this object
        } else {
            if(primaryAction){
                _buttonActionArray[button]->action = action;
                //-- Make sure repeat is off if this action doesn't support repeats
                int idx = _findAssignableButtonAction(action);
                if(idx >= 0) {
                    AssignableButtonAction* p = qobject_cast<AssignableButtonAction*>(_assignableButtonActions[idx]);
                    if(!p->canRepeat()) {
                        _buttonActionArray[button]->repeat = false;
                    }
                }
            } else {
                _buttonActionArray[button]->secondary_action = action;
            }
        }
        //-- Save to settings
        if(primaryAction){
            settings.setValue(QString(_buttonActionNameKey).arg(button), _buttonActionArray[button]->action);
        } else {
            settings.setValue(QString(_buttonSecondaryActionNameKey).arg(button), _buttonActionArray[button]->secondary_action);
            settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(button), _buttonActionArray[button]->secondaryActionType);
        }
        settings.setValue(QString(_buttonActionRepeatKey).arg(button), _buttonActionArray[button]->repeat);
    }
    emit buttonActionsChanged();
}

QString Joystick::getButtonAction(int button, bool primary)
{
    if (_validButton(button)) {
        if(_buttonActionArray[button]) {
            if(primary){
                return _buttonActionArray[button]->action;
            } else {
                return _buttonActionArray[button]->secondary_action;
            }
        }
    }
    return QString(_buttonActionNone);
}

void Joystick::newCustomAction()
{
    _customActions.append(new JoystickCustomAction(this, _totalButtonCount, _buttonOptions));
    emit customActionsChanged();

    qCDebug(JoystickLog) << "New custom action. List size"  << _customActions.size();
}

void Joystick::removeCustomAction(int customIndex)
{
    if(customIndex >= _customActions.size()){
        // Something is extremely wrong if we are here!!! UI and _customActions are out of sync.
        qCWarning(JoystickLog) << "remove Custom Action: index is higher than the number of custom actions";
        return;
    }
    qCDebug(JoystickLog) << "remove Custom Action" << customIndex << _customActions.size();

    JoystickCustomAction* actionToRemove = qobject_cast<JoystickCustomAction*>(_customActions[customIndex]);

    //-- Save to settings
    QSettings settings;
    settings.beginGroup(_settingsGroup);
    settings.beginGroup(_name);

    if(actionToRemove->button1() != 0){
        if(_buttonActionArray[actionToRemove->button1() - 1]){
            if(_buttonActionArray[actionToRemove->button1() - 1]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO){
                if(!_buttonActionArray[actionToRemove->button2() - 1]) return;
                _buttonActionArray[actionToRemove->button2() - 1]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE;
                settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(actionToRemove->button2() - 1), _buttonActionArray[actionToRemove->button2() - 1]->secondaryActionType);
            }
            _buttonActionArray[actionToRemove->button1() - 1]->secondaryActionType = AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE;
            settings.setValue(QString(_buttonSecondaryActionTypeKey).arg(actionToRemove->button1() - 1), _buttonActionArray[actionToRemove->button1() - 1]->secondaryActionType);
        }
    }

    if(actionToRemove->button1() != 0){
        addButtonGlobalCustomActionsList(actionToRemove->button1() - 1);
        addButtonLocalCustomActionsLists(actionToRemove->button1() - 1, customIndex);
    }

    if(actionToRemove->button2() != 0){
        addButtonGlobalCustomActionsList(actionToRemove->button2() - 1);
        addButtonLocalCustomActionsLists(actionToRemove->button2() - 1, customIndex);
    }

    _customActions.removeAt(customIndex);
    emit customActionsChanged();
}

void Joystick::createAvailableButtonsList()
{
    QStringList list;
    list << "Undefined";
    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        // Check if buttonIndex button is initialized
        if(_buttonActionArray[buttonIndex]) {
            if(_buttonActionArray[buttonIndex]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE || _buttonActionArray[buttonIndex]->secondary_action == _buttonActionNone) {
                list << QString::number(buttonIndex);
            }
        } else {
            list << QString::number(buttonIndex);
        }
    }

    _buttonOptions = list;
}

void Joystick::createCustomActionsTable()
{
    _customActions.clear();
    // Look for custom actions by looping through all the buttons available
    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        // Check if buttonIndex button is initialized
        if(_buttonActionArray[buttonIndex]) {
            // Check if it is a custom action type and if the secondary action has been assigned to it
            if(_buttonActionArray[buttonIndex]->secondaryActionType != AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE && _buttonActionArray[buttonIndex]->secondary_action != _buttonActionNone) {
                // If it is a custom action and it was already inserted once then we skip it
                if(_buttonActionArray[buttonIndex]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO && buttonIndex > _buttonActionArray[buttonIndex]->buttonCombo){
                    continue;
                }
                // Add custom action to table
                JoystickCustomAction* actionToAdd = new JoystickCustomAction(this, _totalButtonCount, _buttonOptions);
                actionToAdd->setButton1(buttonIndex + 1);
                actionToAdd->setCurrentButton1(QString::number(buttonIndex));
                actionToAdd->addButtonToOptionsList(buttonIndex);
                actionToAdd->setAction(_buttonActionArray[buttonIndex]->secondary_action);
                actionToAdd->setSecondaryActionType(_buttonActionArray[buttonIndex]->secondaryActionType);
                if(_buttonActionArray[buttonIndex]->secondaryActionType == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO) {
                    actionToAdd->setButton2(_buttonActionArray[buttonIndex]->buttonCombo + 1);
                    actionToAdd->setCurrentButton2(QString::number(_buttonActionArray[buttonIndex]->buttonCombo));
                    actionToAdd->addButtonToOptionsList(_buttonActionArray[buttonIndex]->buttonCombo);
                }
                actionToAdd->updateCustomActionIndex();
                _customActions.append(actionToAdd);
            }
        }
    }

    emit customActionsChanged();
}

void Joystick::updateCustomActionsIndexes(int customIndex, const QString& button1, const QString& button2)
{
    JoystickCustomAction* customActionFromList = qobject_cast<JoystickCustomAction*>(_customActions[customIndex]);

    customActionFromList->setCurrentButton1(button1);
    customActionFromList->setCurrentButton2(button2);

    if(button1 == "Undefined"){
        customActionFromList->updateCustomActionIndex();
    } else {
        // If the selected button is different from the stored one
        if(button1.toInt() != customActionFromList->button1()-1){
            removeButtonGlobalCustomActionsList(button1.toInt());
            removeButtonLocalCustomActionsLists(button1.toInt(), customIndex);
            if(customActionFromList->button1() != 0){
                addButtonGlobalCustomActionsList(customActionFromList->button1() - 1);
                addButtonLocalCustomActionsLists(customActionFromList->button1() - 1, customIndex);
            }
        }
    }

    if(button2 == "Undefined"){
        customActionFromList->updateCustomActionIndex();
    } else {
        if(button2.toInt() != customActionFromList->button2()-1){
            removeButtonGlobalCustomActionsList(button2.toInt());
            removeButtonLocalCustomActionsLists(button2.toInt(), customIndex);
            if(customActionFromList->button2() != 0){
                addButtonGlobalCustomActionsList(customActionFromList->button2() - 1);
                addButtonLocalCustomActionsLists(customActionFromList->button2() - 1, customIndex);
            }
        }
    }
    emit customActionsChanged();
}

void Joystick::updateCustomActions(int customIndex, const QString& button, const QString& buttonCombo, const QString& action, int secondaryActionType)
{
    qCInfo(JoystickLog) << "update custom button actions table";
    if(customIndex >= _customActions.size()){
        // Something is extremely wrong if we are here!!! UI and _customActions are out of sync.
        qCInfo(JoystickLog) << "update Custom Action: custom action index is higher than the number of custom actions";
        return;
    }

    int button1 = 0;
    int button2 = 0;

    qCDebug(JoystickLog) << "updateCustomActionTable: customIndex : button1 : button2 : action : secondaryActionType" << customIndex << button << buttonCombo << action << secondaryActionType;
    updateCustomActionsIndexes(customIndex, button, buttonCombo);

    if(button != "Undefined"){
        button1 = button.toInt() + 1;
    }
    if(buttonCombo != "Undefined"){
        button2 = buttonCombo.toInt() + 1;
    }

    JoystickCustomAction* customActionFromList = qobject_cast<JoystickCustomAction*>(_customActions[customIndex]);

        switch(secondaryActionType){
            case AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE:
                if(customActionFromList->secondaryActionType() == AssignedButtonAction::SECONDARY_BUTTON_PRESS_DOUBLE){
                    setButtonDouble(customActionFromList->button1() - 1, false);
                } else if(customActionFromList->secondaryActionType() == AssignedButtonAction::SECONDARY_BUTTON_PRESS_LONG){
                    setButtonLong(customActionFromList->button1() - 1, false);
                } else if(customActionFromList->secondaryActionType() == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO) {
                    setButtonCombo(customActionFromList->button1() - 1, customActionFromList->button2() - 1, customActionFromList->action(), false);
                }
                break;
            case AssignedButtonAction::SECONDARY_BUTTON_PRESS_DOUBLE:
                // Check that all necessary fields are valid
                if((button1 != 0) && (action != _buttonActionNone)){
                    // If the button1 changes then remove the double action from the previous button1
                    if(button1 != customActionFromList->button1() && customActionFromList->button1() != 0){
                        setButtonDouble(customActionFromList->button1() - 1, false);
                    // If the secondary action type changed then check if it was a combo and if yes disable it
                    } else if(customActionFromList->secondaryActionType() == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO){
                        setButtonCombo(customActionFromList->button1() - 1, customActionFromList->button2() - 1, customActionFromList->action(), false);
                    }
                    setButtonAction(button1 - 1, action, false);
                    setButtonDouble(button1 - 1, true);
                // If one of the new fields is not valid then check if it used to be valid and if yes disable it
                } else if(customActionFromList->button1() != 0 && customActionFromList->action() != _buttonActionNone){
                    setButtonDouble(customActionFromList->button1() - 1, false);
                }
                break;
            case AssignedButtonAction::SECONDARY_BUTTON_PRESS_LONG:
                // Check that all necessary fields are valid
                if((button1 != 0) && (action != _buttonActionNone)){
                    // If the button1 changes then remove the double action from the previous button1
                    if(button1 != customActionFromList->button1() && customActionFromList->button1() != 0){
                        setButtonLong(customActionFromList->button1() - 1, false);
                    // If the secondary action type changed then check if it was a combo and if yes disable it
                    } else if(customActionFromList->secondaryActionType() == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO){
                        setButtonCombo(customActionFromList->button1() - 1, customActionFromList->button2() - 1, customActionFromList->action(), false);
                    }
                    setButtonAction(button1 - 1, action, false);
                    setButtonLong(button1 - 1, true);
                // If one of the new fields is not valid then check if it used to be valid and if yes disable it
                } else if(customActionFromList->button1() != 0 && customActionFromList->action() != _buttonActionNone){
                    setButtonLong(customActionFromList->button1() - 1, false);
                }
                break;
            case AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO:
                // Check that all necessary fields are valid
                if((button1 != 0) && (button2 != 0) && (action != _buttonActionNone)){
                    if((customActionFromList->button1() != 0) && (customActionFromList->button2() != 0)){
                        // If the button1 changes then remove the double action from the previous button1
                        if((button1 != customActionFromList->button1()) || (button2 != customActionFromList->button2())){
                            setButtonCombo(customActionFromList->button1() - 1, customActionFromList->button2() - 1, customActionFromList->action(), false);
                        // If the secondary action type changed then check if it was a combo and if yes disable it
                        } else if(customActionFromList->secondaryActionType() == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO){
                            setButtonCombo(customActionFromList->button1() - 1, customActionFromList->button2() - 1, customActionFromList->action(), false);
                        }
                    }
                    setButtonCombo(button1 - 1, button2 - 1, action, true);
                // If one of the new fields is not valid then check if it used to be valid and if yes disable it
                } else if(customActionFromList->button1() != 0 && customActionFromList->button2() != 0 && customActionFromList->action() != _buttonActionNone){
                    // Unset double
                    setButtonCombo(customActionFromList->button1() - 1, customActionFromList->button2() - 1, customActionFromList->action(), false);
                }
                break;
            default:
                break;
        }

    // Setting them every time removes a lot of code in checking what changed
    customActionFromList->setButton1(button1);
    customActionFromList->setButton2(button2);
    customActionFromList->setAction(action);
    customActionFromList->setSecondaryActionType(secondaryActionType);

    bool areCustomActionsAssigned = true;
    if(customActionFromList->secondaryActionType() == AssignedButtonAction::SECONDARY_BUTTON_PRESS_COMBO){
        areCustomActionsAssigned = button1 > 0;
    }

    bool isCustomActionActive = ((customActionFromList->secondaryActionType() != AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE)
                                && (action != _buttonActionNone)
                                && (button1 > 0)
                                && areCustomActionsAssigned);

    emit buttonCustomActionChanged(customIndex, isCustomActionActive);
}

void Joystick::removeButtonComboTable(int button, bool comboAction)
{
    for(int i = 0; i < _customActions.size(); i++){
        JoystickCustomAction* comboActionFromList = qobject_cast<JoystickCustomAction*>(_customActions[i]);

        if(comboAction){
            if(comboActionFromList->button1() == button + 1 || comboActionFromList->button2() == button + 1){
                if((comboActionFromList->button1() != 0) && (comboActionFromList->button2() != 0) && (comboActionFromList->action() != _buttonActionNone)) {
                    // unset combo
                    setButtonCombo(comboActionFromList->button1() - 1, comboActionFromList->button2() - 1, comboActionFromList->action(), false);
                    emit buttonCustomActionChanged(i, false);
                }
            }
        } else {
            if(comboActionFromList->button1() == button + 1){
                comboActionFromList->setSecondaryActionType(AssignedButtonAction::SECONDARY_BUTTON_PRESS_NONE);
                emit buttonCustomActionChanged(i, false);
            }
        }
    }
}

QStringList Joystick::buttonActions()
{
    QStringList list;
    for (int button = 0; button < _totalButtonCount; button++) {
        list << getButtonAction(button, true);
    }
    return list;
}

QStringList Joystick::buttonSecondaryActions()
{
    QStringList list;
    for (int button = 0; button < _totalButtonCount; button++) {
        list << getButtonAction(button, false);
    }
    return list;
}

QStringList Joystick::buttonNumberingList()
{
    QStringList list;
    list << "Undefined";
    for (int button = 0; button < _totalButtonCount; button++) {
        list << QString::number(button);
    }
    return list;
}

QStringList Joystick::buttonSecondaryActionTypeList()
{
    QStringList list;
    list << "NONE" << "DOUBLE" << "LONG" << "COMBO";
    
    return list;
}

int Joystick::throttleMode()
{
    return _throttleMode;
}

void Joystick::setThrottleMode(int mode)
{
    if (mode < 0 || mode >= ThrottleModeMax) {
        qCWarning(JoystickLog) << "Invalid throttle mode" << mode;
        return;
    }
    _throttleMode = static_cast<ThrottleMode_t>(mode);
    if (_throttleMode == ThrottleModeDownZero) {
        setAccumulator(false);
    }
    _saveSettings();
    emit throttleModeChanged(_throttleMode);
}

bool Joystick::negativeThrust()
{
    return _negativeThrust;
}

void Joystick::setNegativeThrust(bool allowNegative)
{
    if (_negativeThrust == allowNegative) {
        return;
    }
    _negativeThrust = allowNegative;
    _saveSettings();
    emit negativeThrustChanged(_negativeThrust);
}

float Joystick::exponential()
{
    return _exponential;
}

void Joystick::setExponential(float expo)
{
    _exponential = expo;
    _saveSettings();
    emit exponentialChanged(_exponential);
}

bool Joystick::accumulator()
{
    return _accumulator;
}

void Joystick::setAccumulator(bool accu)
{
    _accumulator = accu;
    _saveSettings();
    emit accumulatorChanged(_accumulator);
}

bool Joystick::deadband()
{
    return _deadband;
}

void Joystick::setDeadband(bool deadband)
{
    _deadband = deadband;
    _saveSettings();
}

bool Joystick::circleCorrection()
{
    return _circleCorrection;
}

void Joystick::setCircleCorrection(bool circleCorrection)
{
    _circleCorrection = circleCorrection;
    _saveSettings();
    emit circleCorrectionChanged(_circleCorrection);
}

QList<QObject*> Joystick::customActions()
{
    return _customActions;
}

void Joystick::setGimbalEnabled(bool set)
{
    _gimbalEnabled = set;
    _saveSettings();
    emit gimbalEnabledChanged();
}

void Joystick::setAxisFrequency(float val)
{
    //-- Arbitrary limits
    if(val < 0.25f) val = 0.25f;
    if(val > 50.0f) val = 50.0f;
    _axisFrequency = val;
    _saveSettings();
    emit axisFrequencyChanged();
}

void Joystick::setButtonFrequency(float val)
{
    //-- Arbitrary limits
    if(val < 0.25f) val = 0.25f;
    if(val > 50.0f) val = 50.0f;
    _buttonFrequency = val;
    _saveSettings();
    emit buttonFrequencyChanged();
}

void Joystick::setCalibrationMode(bool calibrating)
{
    _calibrationMode = calibrating;
    if (calibrating && !isRunning()) {
        _pollingStartedForCalibration = true;
        startPolling(_multiVehicleManager->activeVehicle());
    }
    else if (_pollingStartedForCalibration) {
        stopPolling();
    }
}


void Joystick::_executeButtonAction(const QString& action, bool buttonDown)
{
    if (!_activeVehicle || !_activeVehicle->joystickEnabled() || action == _buttonActionNone) {
        return;
    }

    qCDebug(JoystickLog) << "execute Button Action" << action << buttonDown;

    if (action == _buttonActionArm) {
        if (buttonDown) emit setArmed(true);
    } else if (action == _buttonActionDisarm) {
        if (buttonDown) emit setArmed(false);
    } else if (action == _buttonActionToggleArm) {
        if (buttonDown) emit setArmed(!_activeVehicle->armed());
    } else if (action == _buttonActionVTOLFixedWing) {
        if (buttonDown) emit setVtolInFwdFlight(true);
    } else if (action == _buttonActionVTOLMultiRotor) {
        if (buttonDown) emit setVtolInFwdFlight(false);
    } else if (_activeVehicle->flightModes().contains(action) || _activeVehicle->extraJoystickFlightModes().contains(action)) {
        if (buttonDown) emit setFlightMode(action);
    } else if(action == _buttonActionContinuousZoomIn || action == _buttonActionContinuousZoomOut) {
        if (buttonDown) {
            emit startContinuousZoom(action == _buttonActionContinuousZoomIn ? 1 : -1);
        } else {
            emit stopContinuousZoom();
        }
    } else if(action == _buttonActionStepZoomIn || action == _buttonActionStepZoomOut) {
        if (buttonDown) emit stepZoom(action == _buttonActionStepZoomIn ? 1 : -1);
    } else if(action == _buttonActionNextStream || action == _buttonActionPreviousStream) {
        if (buttonDown) emit stepStream(action == _buttonActionNextStream ? 1 : -1);
    } else if(action == _buttonActionNextCamera || action == _buttonActionPreviousCamera) {
        if (buttonDown) emit stepCamera(action == _buttonActionNextCamera ? 1 : -1);
    } else if(action == _buttonActionTriggerCamera) {
        if (buttonDown) emit triggerCamera();
    } else if(action == _buttonActionToggleCameraMode) {
        if (buttonDown) emit toggleCameraMode();
    } else if(action == _buttonActionStartVideoRecord) {
        if (buttonDown) emit startVideoRecord();
    } else if(action == _buttonActionStopVideoRecord) {
        if (buttonDown) emit stopVideoRecord();
    } else if(action == _buttonActionToggleVideoRecord) {
        if (buttonDown) emit toggleVideoRecord();
    } else if(action == _buttonActionGimbalUp) {
        if (buttonDown) _pitchStep(1);
    } else if(action == _buttonActionGimbalDown) {
        if (buttonDown) _pitchStep(-1);
    } else if(action == _buttonActionGimbalLeft) {
        if (buttonDown) _yawStep(-1);
    } else if(action == _buttonActionGimbalRight) {
        if (buttonDown) _yawStep(1);
    } else if(action == _buttonActionGimbalCenter) {
        if (buttonDown) {
            _localPitch = 0.0;
            _localYaw   = 0.0;
            emit centerGimbal();
        }
    } else if(action == _buttonActionGimbalPitchUp || action == _buttonActionGimbalPitchDown) {
        if (buttonDown) {
            emit startContinuousGimbalPitch(action == _buttonActionGimbalPitchUp ? 1 : -1);
        } else {
            emit stopContinuousGimbalPitch();
        }
    } else if(action == _buttonActionToggleThermal) {
        if(buttonDown) {
            emit toggleThermal();
        }
    } else if(action == _buttonActionThermalOn) {
        if(buttonDown) {
            emit switchThermalOn();
        }
    } else if(action == _buttonActionThermalOff) {
        if(buttonDown) {
            emit switchThermalOff();
        }
    } else if(action == _buttonActionThermalNextPalette) {
        if(buttonDown) {
            emit thermalNextPalette();
        }
    } else {
        qCDebug(JoystickLog) << "_buttonAction unknown action:" << action;
    }
}

void Joystick::_pitchStep(int direction)
{
//    _localPitch += static_cast<double>(direction);
//    //-- Arbitrary range
//    if(_localPitch < -90.0) _localPitch = -90.0;
//    if(_localPitch >  35.0) _localPitch =  35.0;
//    emit gimbalControlValue(_localPitch, _localYaw);
    emit gimbalPitchStep(direction);
}

void Joystick::_yawStep(int direction)
{
//    _localYaw += static_cast<double>(direction);
//    if(_localYaw < -180.0) _localYaw = -180.0;
//    if(_localYaw >  180.0) _localYaw =  180.0;
//    emit gimbalControlValue(_localPitch, _localYaw);
    emit gimbalYawStep(direction);
}

bool Joystick::_validAxis(int axis)
{
    if(axis >= 0 && axis < _axisCount) {
        return true;
    }
    qCWarning(JoystickLog) << "Invalid axis index" << axis;
    return false;
}

bool Joystick::_validButton(int button)
{
    if(button >= 0 && button < _totalButtonCount)
        return true;
    qCWarning(JoystickLog) << "Invalid button index" << button;
    return false;
}

int Joystick::_findAssignableButtonAction(const QString& action)
{
    for(int i = 0; i < _assignableButtonActions.count(); i++) {
        AssignableButtonAction* p = qobject_cast<AssignableButtonAction*>(_assignableButtonActions[i]);
        if(p->action() == action)
            return i;
    }
    return -1;
}

void Joystick::_buildActionList(Vehicle* activeVehicle)
{
    if(_assignableButtonActions.count())
        _assignableButtonActions.clearAndDeleteContents();
    _availableActionTitles.clear();
    //-- Available Actions
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionNone));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionArm));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionDisarm));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionToggleArm));
    if (activeVehicle) {
        QStringList list = activeVehicle->flightModes();
        foreach(auto mode, list) {
            _assignableButtonActions.append(new AssignableButtonAction(this, mode));
        }
        list = activeVehicle->extraJoystickFlightModes();
        foreach(auto mode, list) {
            _assignableButtonActions.append(new AssignableButtonAction(this, mode));
        }
    }
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionVTOLFixedWing));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionVTOLMultiRotor));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionContinuousZoomIn, true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionContinuousZoomOut, true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionStepZoomIn,  true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionStepZoomOut, true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionNextStream));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionPreviousStream));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionNextCamera));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionPreviousCamera));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionTriggerCamera));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionToggleCameraMode));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionStartVideoRecord));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionStopVideoRecord));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionToggleVideoRecord));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionGimbalDown,    true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionGimbalUp,      true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionGimbalLeft,    true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionGimbalRight,   true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionGimbalCenter));

    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionGimbalPitchUp, true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionGimbalPitchDown, true));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionToggleThermal));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionThermalOn));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionThermalOff));
    _assignableButtonActions.append(new AssignableButtonAction(this, _buttonActionThermalNextPalette));

    //-- Leave "No Action" out
    for(int i = 1; i < _assignableButtonActions.count(); i++) {
        AssignableButtonAction* p = qobject_cast<AssignableButtonAction*>(_assignableButtonActions[i]);
        _availableActionTitles << p->action();
    }
    //-- Sort list
    _availableActionTitles.sort(Qt::CaseInsensitive);
    //-- Append "No Action" to top of list
    _availableActionTitles.insert(0,_buttonActionNone);
    emit assignableActionsChanged();
}

AssignableButtonAction* Joystick::getAssignableAction(const QString& action)
{
    for(int i = 0; i < _assignableButtonActions.count(); i++) {
        AssignableButtonAction* p = qobject_cast<AssignableButtonAction*>(_assignableButtonActions[i]);
        if(p->action() == action)
            return p;
    }
    return nullptr;
}
