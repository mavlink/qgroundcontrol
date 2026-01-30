#include "Joystick.h"
#include "MavlinkAction.h"
#include "MavlinkActionManager.h"
#include "MavlinkActionsSettings.h"
#include "FirmwarePlugin.h"
#include "GimbalController.h"
#include "QGCCorePlugin.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "JoystickManager.h"
#include "MultiVehicleManager.h"

#include <QtCore/QCoreApplication>
#include <algorithm>
#include <QtCore/QSettings>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(JoystickLog, "Joystick.Joystick")
QGC_LOGGING_CATEGORY(JoystickVerboseLog, "Joystick.Joystick:verbose")

namespace
{
static constexpr const char* kButtonActionArrayGroup = "JoystickButtonActionSettingsArray";
static constexpr const char* kButtonActionNameKey = "actionName";
static constexpr const char* kButtonRepeatKey = "repeat";
static constexpr const char* kAxisSettingsArrayGroup = "JoystickAxisSettingsArray";
static constexpr const char* kAxisFunctionKey = "function";
static constexpr const char* kAxisMinKey = "min";
static constexpr const char* kAxisMaxKey = "max";
static constexpr const char* kAxisCenterKey = "center";
static constexpr const char* kAxisDeadbandKey = "deadband";
static constexpr const char* kAxisReversedKey = "reversed";
}

/*===========================================================================*/

AssignedButtonAction::AssignedButtonAction(const QString &actionName_, bool repeat_)
    : actionName(actionName_)
    , repeat(repeat_)
{
    qCDebug(JoystickLog) << this;
}

AvailableButtonAction::AvailableButtonAction(const QString &actionName_, bool canRepeat_, QObject *parent)
    : QObject(parent)
    , _actionName(actionName_)
    , _repeat(canRepeat_)
{
    qCDebug(JoystickLog) << this;
}

/*===========================================================================*/

Joystick::Joystick(const QString &name, int axisCount, int buttonCount, int hatCount, QObject *parent)
    : QThread(parent)
    , _name(name)
    , _axisCount(axisCount)
    , _buttonCount(buttonCount)
    , _hatCount(hatCount)
    , _hatButtonCount(4 * hatCount)
    , _totalButtonCount(_buttonCount + _hatButtonCount)
    , _rgCalibration(_axisCount)
    , _buttonEventStates(_totalButtonCount)
    , _assignedButtonActions(_totalButtonCount, nullptr)
    , _mavlinkActionManager(new MavlinkActionManager(SettingsManager::instance()->mavlinkActionsSettings()->joystickActionsFile(), this))
    , _availableButtonActions(new QmlObjectListModel(this))
    , _joystickManager(JoystickManager::instance())
    , _joystickSettings(name, _axisCount, _totalButtonCount)
{
    qCDebug(JoystickLog)
        << name
        << "axisCount:" << axisCount
        << "buttonCount:" << buttonCount
        << "hatCount:" << hatCount
        << this;

    if (QCoreApplication *const app = QCoreApplication::instance()) {
        QThread *const guiThread = app->thread();
        if (_joystickSettings.thread() != guiThread) {
            _joystickSettings.moveToThread(guiThread);
        }

        const auto ensureFactThread = [guiThread](Fact *fact) {
            if (fact && fact->thread() != guiThread) {
                fact->moveToThread(guiThread);
            }
        };

        ensureFactThread(_joystickSettings.calibrated());
        ensureFactThread(_joystickSettings.circleCorrection());
        ensureFactThread(_joystickSettings.useDeadband());
        ensureFactThread(_joystickSettings.negativeThrust());
        ensureFactThread(_joystickSettings.throttleSmoothing());
        ensureFactThread(_joystickSettings.axisFrequencyHz());
        ensureFactThread(_joystickSettings.buttonFrequencyHz());
        ensureFactThread(_joystickSettings.throttleModeCenterZero());
        ensureFactThread(_joystickSettings.transmitterMode());
        ensureFactThread(_joystickSettings.exponentialPct());
        ensureFactThread(_joystickSettings.enableManualControlAux1());
        ensureFactThread(_joystickSettings.enableManualControlAux2());
        ensureFactThread(_joystickSettings.enableManualControlAux3());
        ensureFactThread(_joystickSettings.enableManualControlAux4());
        ensureFactThread(_joystickSettings.enableManualControlAux5());
        ensureFactThread(_joystickSettings.enableManualControlAux6());
    }

    // Changes to manual control extension settings require re-calibration
    connect(_joystickSettings.enableManualControlPitchExtension(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableManualControlRollExtension(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableManualControlAux1(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableManualControlAux2(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableManualControlAux3(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableManualControlAux4(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableManualControlAux5(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableManualControlAux6(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });

    _resetFunctionToAxisMap();
    _resetAxisCalibrationData();
    _resetButtonActionData();
    _resetButtonEventStates();

    _buildAvailableButtonsActionList(MultiVehicleManager::instance()->activeVehicle());

    _loadFromSettingsIntoCalibrationData();
}

Joystick::~Joystick()
{
    _exitThread = true;
    if (isRunning()) {
        if (QThread::currentThread() == this) {
            qCWarning(JoystickLog) << "Skipping wait() on joystick thread";
        } else {
            wait();
        }
    }

    _resetButtonActionData();
    _availableButtonActions->clearAndDeleteContents();
}

void Joystick::_resetFunctionToAxisMap()
{
    _axisFunctionToJoystickAxisMap.clear();
    for (int i = 0; i < maxAxisFunction; i++) {
        _axisFunctionToJoystickAxisMap[static_cast<AxisFunction_t>(i)] = kJoystickAxisNotAssigned;
    }
}

void Joystick::_resetAxisCalibrationData()
{
    _resetFunctionToAxisMap();
    for (int axis = 0; axis < _axisCount; axis++) {
        auto& calibration = _rgCalibration[axis];
        calibration.reset();
    }
}

void Joystick::_resetButtonActionData()
{
    for (int i = 0; i < _assignedButtonActions.count(); i++) {
        delete _assignedButtonActions[i];
        _assignedButtonActions[i] = nullptr;
    }
}

void Joystick::_resetButtonEventStates()
{
    for (int i = 0; i < _buttonEventStates.count(); i++) {
        _buttonEventStates[i] = ButtonEventNone;
    }
}

void Joystick::_loadButtonSettings()
{
    const QString actionNameKey = QString::fromLatin1(kButtonActionNameKey);
    const QString repeatKey = QString::fromLatin1(kButtonRepeatKey);

    QSettings buttonSettings;
    buttonSettings.beginGroup(_joystickSettings.settingsGroup());

    if (buttonSettings.childGroups().contains(QString::fromLatin1(kButtonActionArrayGroup))) {
        buttonSettings.beginGroup(QString::fromLatin1(kButtonActionArrayGroup));

        qCDebug(JoystickLog) << "Button Actions:";

        bool foundButton = false;
        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            if (!buttonSettings.childGroups().contains(QString::number(buttonIndex))) {
                continue;
            }

            buttonSettings.beginGroup(QString::number(buttonIndex));
            const QString actionName = buttonSettings.value(actionNameKey).toString();
            const bool repeat = buttonSettings.value(repeatKey, false).toBool();
            buttonSettings.endGroup();

            if (actionName.isEmpty()) {
                qCWarning(JoystickLog)
                    << "    "
                    << "button:" << buttonIndex
                    << "has empty action name, clearing action from data";
                buttonSettings.remove(QString::number(buttonIndex));
                continue;
            }
            if (_findAvailableButtonActionIndex(actionName) == -1) {
                qCWarning(JoystickLog)
                    << "    "
                    << "button:" << buttonIndex
                    << "has unknown action name:" << actionName
                    << ", clearing action from data";
                buttonSettings.remove(QString::number(buttonIndex));
                continue;
            }
            if (actionName == _buttonActionNone) {
                buttonSettings.remove(QString::number(buttonIndex));
                continue;
            }

            AssignedButtonAction *buttonAction = new AssignedButtonAction(actionName, repeat);
            _assignedButtonActions[buttonIndex] = buttonAction;
            _assignedButtonActions[buttonIndex]->buttonElapsedTimer.start();
            foundButton = true;

            qCDebug(JoystickLog)
                << "    "
                << "button:" <<     buttonIndex
                << "actionName:" << actionName
                << "repeat:" <<     repeat;
        }

        buttonSettings.endGroup();

        if (!foundButton) {
            buttonSettings.remove(QString::fromLatin1(kButtonActionArrayGroup));
        }
    }
}

void Joystick::_foundInvalidAxisSettingsCleanup()
{
    _clearAxisSettings();
    _resetAxisCalibrationData();
    _joystickSettings.calibrated()->setRawValue(false);
}

void Joystick::_loadAxisSettings(bool joystickCalibrated, int transmitterMode)
{
    QSettings axisSettings;
    axisSettings.beginGroup(_joystickSettings.settingsGroup());

    // If the joystick isn't calibrated, we skip loading calibration data.
    // Also there shouldn't be stored axis information, so clear that if any
    if (!joystickCalibrated) {
        _clearAxisSettings();
        _resetAxisCalibrationData();
        return;
    }

    const QString functionKey = QString::fromLatin1(kAxisFunctionKey);
    const QString minKey = QString::fromLatin1(kAxisMinKey);
    const QString maxKey = QString::fromLatin1(kAxisMaxKey);
    const QString centerKey = QString::fromLatin1(kAxisCenterKey);
    const QString deadbandKey = QString::fromLatin1(kAxisDeadbandKey);
    const QString reversedKey = QString::fromLatin1(kAxisReversedKey);

    qCDebug(JoystickLog) << "Axis Settings:";

    axisSettings.beginGroup(QString::fromLatin1(kAxisSettingsArrayGroup));
    for (int axis = 0; axis < _axisCount; axis++) {
        if (!axisSettings.childGroups().contains(QString::number(axis))) {
            qCDebug(JoystickLog)
                << "    "
                << "axis:" << axis
                << "no settings found, skipping";
            continue;
        }

        axisSettings.beginGroup(QString::number(axis));

        auto& axisCalibration = _rgCalibration[axis];

        const int axisFunction = axisSettings.value(functionKey, maxAxisFunction).toInt();
        if (axisFunction < 0 || axisFunction > maxAxisFunction) {
            qCWarning(JoystickLog) << "Invalid function" << axisFunction << "for axis" << axis;
            axisSettings.endGroup();
            continue;
        }
        _setJoystickAxisForAxisFunction(static_cast<AxisFunction_t>(axisFunction), axis);

        axisCalibration.center = axisSettings.value(centerKey, axisCalibration.center).toInt();
        axisCalibration.min = axisSettings.value(minKey, axisCalibration.min).toInt();
        axisCalibration.max = axisSettings.value(maxKey, axisCalibration.max).toInt();
        axisCalibration.deadband = axisSettings.value(deadbandKey, axisCalibration.deadband).toInt();
        axisCalibration.reversed = axisSettings.value(reversedKey, axisCalibration.reversed).toBool();

        qCDebug(JoystickLog)
            << "    "
            << "axis:" <<           axis
            << "min:" <<            axisCalibration.min
            << "max:" <<            axisCalibration.max
            << "center:" <<         axisCalibration.center
            << "reversed:" <<       axisCalibration.reversed
            << "deadband:" <<       axisCalibration.deadband
            << "axisFunction:" <<   axisFunctionToString(static_cast<AxisFunction_t>(axisFunction));

        axisSettings.endGroup();
    }
    axisSettings.endGroup();

    // All axis information is loaded, verify that we have all the required control mappings
    bool rollFunctionNotAssigned = _axisFunctionToJoystickAxisMap[rollFunction] == kJoystickAxisNotAssigned;
    if (rollFunctionNotAssigned) {
        qCWarning(JoystickLog) << "Roll axis function not assigned";
    }
    bool pitchFunctionNotAssigned = _axisFunctionToJoystickAxisMap[pitchFunction] == kJoystickAxisNotAssigned;
    if (pitchFunctionNotAssigned) {
        qCWarning(JoystickLog) << "Pitch axis function not assigned";
    }
    bool yawFunctionNotAssigned = _axisFunctionToJoystickAxisMap[yawFunction] == kJoystickAxisNotAssigned;
    if (yawFunctionNotAssigned) {
        qCWarning(JoystickLog) << "Yaw axis function not assigned";
    }
    bool throttleFunctionNotAssigned = _axisFunctionToJoystickAxisMap[throttleFunction] == kJoystickAxisNotAssigned;
    if (throttleFunctionNotAssigned) {
        qCWarning(JoystickLog) << "Throttle axis function not assigned";
    }
    bool pitchExtensionFunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableManualControlPitchExtension()->rawValue().toBool()) {
        pitchExtensionFunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(pitchExtensionFunction) == kJoystickAxisNotAssigned;
        if (pitchExtensionFunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing pitch extension axis function mapping!";
        }
    }
    bool rollExtensionFunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableManualControlRollExtension()->rawValue().toBool()) {
        rollExtensionFunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(rollExtensionFunction) == kJoystickAxisNotAssigned;
        if (rollExtensionFunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing roll extension axis function mapping!";
        }
    }
    bool aux1ExtensionFunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableManualControlAux1()->rawValue().toBool()) {
        aux1ExtensionFunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(aux1ExtensionFunction) == kJoystickAxisNotAssigned;
        if (aux1ExtensionFunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing aux1 extension axis function mapping!";
        }
    }
    bool aux2ExtensionFunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableManualControlAux2()->rawValue().toBool()) {
        aux2ExtensionFunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(aux2ExtensionFunction) == kJoystickAxisNotAssigned;
        if (aux2ExtensionFunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing aux2 extension axis function mapping!";
        }
    }
    bool aux3ExtensionFunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableManualControlAux3()->rawValue().toBool()) {
        aux3ExtensionFunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(aux3ExtensionFunction) == kJoystickAxisNotAssigned;
        if (aux3ExtensionFunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing aux3 extension axis function mapping!";
        }
    }
    bool aux4ExtensionFunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableManualControlAux4()->rawValue().toBool()) {
        aux4ExtensionFunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(aux4ExtensionFunction) == kJoystickAxisNotAssigned;
        if (aux4ExtensionFunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing aux4 extension axis function mapping!";
        }
    }
    bool aux5ExtensionFunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableManualControlAux5()->rawValue().toBool()) {
        aux5ExtensionFunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(aux5ExtensionFunction) == kJoystickAxisNotAssigned;
        if (aux5ExtensionFunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing aux5 extension axis function mapping!";
        }
    }
    bool aux6ExtensionFunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableManualControlAux6()->rawValue().toBool()) {
        aux6ExtensionFunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(aux6ExtensionFunction) == kJoystickAxisNotAssigned;
        if (aux6ExtensionFunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing aux6 extension axis function mapping!";
        }
    }
    if (rollFunctionNotAssigned || pitchFunctionNotAssigned || yawFunctionNotAssigned || throttleFunctionNotAssigned ||
            pitchExtensionFunctionRequiredButNotAssigned || rollExtensionFunctionRequiredButNotAssigned ||
            aux1ExtensionFunctionRequiredButNotAssigned || aux2ExtensionFunctionRequiredButNotAssigned || aux3ExtensionFunctionRequiredButNotAssigned ||
            aux4ExtensionFunctionRequiredButNotAssigned || aux5ExtensionFunctionRequiredButNotAssigned || aux6ExtensionFunctionRequiredButNotAssigned) {
        qCWarning(JoystickLog) << "Missing control axis function(s), resetting all axis settings, marking joystick as uncalibrated and disabled";
        _resetAxisCalibrationData();
        _clearAxisSettings();
        _joystickSettings.calibrated()->setRawValue(false);
        _joystickManager->setActiveJoystickEnabledForActiveVehicle(false);
        return;
    }

   // FunctionAxis mappings are always stored in TX mode 2
    // Remap to stored TX mode in settings for UI display
    _remapFunctionsInFunctionMapToNewTransmittedMode(2, transmitterMode);
}

void Joystick::_loadFromSettingsIntoCalibrationData()
{
    _resetAxisCalibrationData();
    _resetButtonActionData();

    // Top level joystick settings come through the JoystickSettings SettingsGroup

    bool calibrated = _joystickSettings.calibrated()->rawValue().toBool();
    int transmitterMode = _joystickSettings.transmitterMode()->rawValue().toInt();

    qCDebug(JoystickLog) << name();
    qCDebug(JoystickLog) << "    calibrated:" <<                        _joystickSettings.calibrated()->rawValue().toBool();
    qCDebug(JoystickLog) << "    throttleSmoothing:" <<                 _joystickSettings.throttleSmoothing()->rawValue().toBool();
    qCDebug(JoystickLog) << "    axisFrequencyHz:" <<                   _joystickSettings.axisFrequencyHz()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    buttonFrequencyHz:" <<                 _joystickSettings.buttonFrequencyHz()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    throttleModeCenterZero:" <<            _joystickSettings.throttleModeCenterZero()->rawValue().toBool();
    qCDebug(JoystickLog) << "    negativeThrust:" <<                    _joystickSettings.negativeThrust()->rawValue().toBool();
    qCDebug(JoystickLog) << "    circleCorrection:" <<                  _joystickSettings.circleCorrection()->rawValue().toBool();
    qCDebug(JoystickLog) << "    exponentialPct:" <<                    _joystickSettings.exponentialPct()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    useDeadband:" <<                       _joystickSettings.useDeadband()->rawValue().toBool();
    qCDebug(JoystickLog) << "    transmitterMode:" <<                   transmitterMode;
    qCDebug(JoystickLog) << "    enableManualControlPitchExtension:" << _joystickSettings.enableManualControlPitchExtension()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlRollExtension:" <<  _joystickSettings.enableManualControlRollExtension()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux1:" <<           _joystickSettings.enableManualControlAux1()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux2:" <<           _joystickSettings.enableManualControlAux2()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux3:" <<           _joystickSettings.enableManualControlAux3()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux4:" <<           _joystickSettings.enableManualControlAux4()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux5:" <<           _joystickSettings.enableManualControlAux5()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux6:" <<           _joystickSettings.enableManualControlAux6()->rawValue().toBool();

    _loadAxisSettings(calibrated, transmitterMode);
    _loadButtonSettings();
}

void Joystick::_saveAxisSettings(int transmitterMode)
{
    qCDebug(JoystickLog) << name();

    // FunctionAxis mappings are always stored in TX mode 2
    // Temporarily remap from calibration TX mode to mode 2 for storage
    _remapFunctionsInFunctionMapToNewTransmittedMode(transmitterMode, 2);

    QSettings axisSettings;
    axisSettings.beginGroup(_joystickSettings.settingsGroup());
    axisSettings.beginGroup(QString::fromLatin1(kAxisSettingsArrayGroup));

    const QString functionKey = QString::fromLatin1(kAxisFunctionKey);
    const QString minKey = QString::fromLatin1(kAxisMinKey);
    const QString maxKey = QString::fromLatin1(kAxisMaxKey);
    const QString centerKey = QString::fromLatin1(kAxisCenterKey);
    const QString deadbandKey = QString::fromLatin1(kAxisDeadbandKey);
    const QString reversedKey = QString::fromLatin1(kAxisReversedKey);

    for (int axis = 0; axis < _axisCount; axis++) {
        AxisCalibration_t *const calibration = &_rgCalibration[axis];
        axisSettings.beginGroup(QString::number(axis));

        axisSettings.setValue(centerKey, calibration->center);
        axisSettings.setValue(minKey, calibration->min);
        axisSettings.setValue(maxKey, calibration->max);
        axisSettings.setValue(deadbandKey, calibration->deadband);
        axisSettings.setValue(reversedKey, calibration->reversed);
        AxisFunction_t function = _getAxisFunctionForJoystickAxis(axis);
        axisSettings.setValue(functionKey, static_cast<int>(function));

        qCDebug(JoystickLog)
            << "    "
            << "axis:" <<           axis
            << "min:" <<            calibration->min
            << "max:" <<            calibration->max
            << "center:" <<         calibration->center
            << "reversed:" <<       calibration->reversed
            << "deadband:" <<       calibration->deadband
            << "axisFunction:" <<   axisFunctionToString(function);

        axisSettings.endGroup();
    }

    axisSettings.endGroup();
    axisSettings.endGroup();

    // Map back to current TX mode
    _remapFunctionsInFunctionMapToNewTransmittedMode(2, transmitterMode);
}

void Joystick::_saveButtonSettings()
{
    qCDebug(JoystickLog) << name();

    const QString actionNameKey = QString::fromLatin1(kButtonActionNameKey);
    const QString repeatKey = QString::fromLatin1(kButtonRepeatKey);

    QSettings buttonSettings;
    buttonSettings.beginGroup(_joystickSettings.settingsGroup());
    buttonSettings.beginGroup(QString::fromLatin1(kButtonActionArrayGroup));

    bool anyButtonsSaved = false;
    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        AssignedButtonAction *buttonAction = _assignedButtonActions[buttonIndex];

        if (buttonAction) {
            buttonSettings.beginGroup(QString::number(buttonIndex));
            buttonSettings.setValue(actionNameKey, buttonAction->actionName);
            buttonSettings.setValue(repeatKey, buttonAction->repeat);
            buttonSettings.endGroup();

            anyButtonsSaved = true;

            qCDebug(JoystickLog)
                << "    "
                << "button:" <<     buttonIndex
                << "actionName:" << buttonAction->actionName
                << "repeat:" <<     buttonAction->repeat;
        } else {
            // No action assigned, remove any existing settings
            buttonSettings.remove(QString::number(buttonIndex));
        }
    }

    buttonSettings.endGroup();
    buttonSettings.endGroup();

    if (!anyButtonsSaved) {
        qCDebug(JoystickLog) << "    No button actions to save, cleared button action settings.";
        _clearButtonSettings();
    }
}

void Joystick::_clearAxisSettings()
{
    QSettings axisSettings;
    axisSettings.beginGroup(_joystickSettings.settingsGroup());
    axisSettings.remove(QString::fromLatin1(kAxisSettingsArrayGroup));
    axisSettings.endGroup();
}

void Joystick::_clearButtonSettings()
{
    QSettings buttonSettings;
    buttonSettings.beginGroup(_joystickSettings.settingsGroup());
    buttonSettings.remove(QString::fromLatin1(kButtonActionArrayGroup));
    buttonSettings.endGroup();
}

void Joystick::_saveFromCalibrationDataIntoSettings()
{
    bool calibrated = _joystickSettings.calibrated()->rawValue().toBool();
    int transmitterMode = _joystickSettings.transmitterMode()->rawValue().toInt();

    qCDebug(JoystickLog) << name();
    qCDebug(JoystickLog) << "    calibrated:" << calibrated;
    qCDebug(JoystickLog) << "    throttleSmoothing:" << _joystickSettings.throttleSmoothing()->rawValue().toBool();
    qCDebug(JoystickLog) << "    axisFrequencyHz:" << _joystickSettings.axisFrequencyHz()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    buttonFrequencyHz:" << _joystickSettings.buttonFrequencyHz()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    throttleModeCenterZero:" << _joystickSettings.throttleModeCenterZero()->rawValue().toBool();
    qCDebug(JoystickLog) << "    negativeThrust:" << _joystickSettings.negativeThrust()->rawValue().toBool();
    qCDebug(JoystickLog) << "    circleCorrection:" << _joystickSettings.circleCorrection()->rawValue().toBool();
    qCDebug(JoystickLog) << "    exponentialPct:" << _joystickSettings.exponentialPct()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    useDeadband:" << _joystickSettings.useDeadband()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux1:" << _joystickSettings.enableManualControlAux1()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux2:" << _joystickSettings.enableManualControlAux2()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux3:" << _joystickSettings.enableManualControlAux3()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux4:" << _joystickSettings.enableManualControlAux4()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux5:" << _joystickSettings.enableManualControlAux5()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlAux6:" << _joystickSettings.enableManualControlAux6()->rawValue().toBool();
    qCDebug(JoystickLog) << "    transmitterMode:" << transmitterMode;

    _clearAxisSettings();
    if (calibrated) {
        _saveAxisSettings(transmitterMode);
    }

    _clearButtonSettings();
    _saveButtonSettings();
}

void Joystick::_remapFunctionsInFunctionMapToNewTransmittedMode(int fromMode, int toMode)
{
    static constexpr const int modeCount = 4;
    static constexpr const int attitudeControlCount = 4;
    static constexpr const AxisFunction_t mapping[modeCount][attitudeControlCount] = {
        { yawFunction,  pitchFunction,      rollFunction,   throttleFunction },
        { yawFunction,  throttleFunction,   rollFunction,   pitchFunction },
        { rollFunction, pitchFunction,      yawFunction,    throttleFunction },
        { rollFunction, throttleFunction,   yawFunction,    pitchFunction }
    };

    // First make a direct copy so the extension stick function are set correctly
    AxisFunctionMap_t tempMap = _axisFunctionToJoystickAxisMap;

    for (int i=0; i<attitudeControlCount; i++) {
        auto fromAxisFunction = mapping[fromMode - 1][i];
        auto toAxisFunction = mapping[toMode - 1][i];

        tempMap[toAxisFunction] = _axisFunctionToJoystickAxisMap[fromAxisFunction];
    }

    for (int i = 0; i < maxAxisFunction; i++) {
        auto axisFunction = static_cast<AxisFunction_t>(i);
        _axisFunctionToJoystickAxisMap[axisFunction] = tempMap[axisFunction];
    }
}

void Joystick::run()
{
    bool openFailed = false;
    bool updateFailed = false;

    if (!_open()) {
        qCWarning(JoystickLog) << "Failed to open joystick:" << _name;
        openFailed = true;
    }

    if (!openFailed) {
        _axisElapsedTimer.start();

        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            if (_assignedButtonActions[buttonIndex]) {
                _assignedButtonActions[buttonIndex]->buttonElapsedTimer.start();
            }
        }

        while (!_exitThread) {
            if (!_update()) {
                qCWarning(JoystickLog) << "Joystick disconnected or update failed:" << _name;
                updateFailed = true;
                break;
            }

            _handleButtons();

            if (axisCount() != 0) {
                _handleAxis();
            }

            const double axisFrequencyHz = _joystickSettings.axisFrequencyHz()->rawValue().toDouble();
            const double buttonFrequencyHz = _joystickSettings.buttonFrequencyHz()->rawValue().toDouble();

            const int sleep = qMin(static_cast<int>(1000.0 / axisFrequencyHz), static_cast<int>(1000.0 / buttonFrequencyHz)) / 2;
            QThread::msleep(sleep);
        }

        _close();
    }

    if ((openFailed || updateFailed) && !_exitThread) {
        qCDebug(JoystickLog) << "Triggering joystick rescan after failure";
        QMetaObject::invokeMethod(JoystickManager::instance(), "_checkForAddedOrRemovedJoysticks", Qt::QueuedConnection);
    }
}

void Joystick::_updateButtonEventState(int buttonIndex, const bool buttonPressed, ButtonEvent_t &buttonEventState)
{
    if (buttonPressed) {
        if (buttonEventState == ButtonEventNone) {
            qCDebug(JoystickLog) << "Button" << buttonIndex << "down transition";
            buttonEventState = ButtonEventDownTransition;
        } else if (buttonEventState == ButtonEventDownTransition) {
            qCDebug(JoystickLog) << "Button" << buttonIndex << "repeat";
            buttonEventState = ButtonEventRepeat;
        }
    } else {
        if (buttonEventState == ButtonEventDownTransition || buttonEventState == ButtonEventRepeat) {
            qCDebug(JoystickLog) << "Button" << buttonIndex << "up transition";
            buttonEventState = ButtonEventUpTransition;
        } else if (buttonEventState == ButtonEventUpTransition) {
            qCDebug(JoystickLog) << "Button" << buttonIndex << "none";
            buttonEventState = ButtonEventNone;
        }
    }
}

void Joystick::_updateButtonEventStates(QVector<ButtonEvent_t> &buttonEventStates)
{
    if (buttonEventStates.size() < _totalButtonCount) {
        qCWarning(JoystickLog) << "Internal Error: buttonEventStates size incorrect!";
        return;
    }

    // Update standard buttons - index 0 to buttonCount-1
    for (int buttonIndex = 0; buttonIndex < _buttonCount; buttonIndex++) {
        const bool buttonIsPressed = _getButton(buttonIndex);
        _updateButtonEventState(buttonIndex, buttonIsPressed, buttonEventStates[buttonIndex]);
    }

    // Update hat buttons - indexes buttonCount to totalButtonCount-1
    const int numHatButtons = 4;
    int nextIndex = _buttonCount;
    for (int hatIndex = 0; hatIndex < _hatCount; hatIndex++) {
        for (int hatButtonIndex = 0; hatButtonIndex < numHatButtons; hatButtonIndex++) {
            const bool buttonIsPressed = _getHat(hatIndex, hatButtonIndex);
            const int currentIndex = nextIndex;
            _updateButtonEventState(currentIndex, buttonIsPressed, buttonEventStates[currentIndex]);
            ++nextIndex;
        }
    }
}

void Joystick::_handleButtons()
{
    if (_currentPollingType == NotPolling) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling!";
        return;
    }

    _updateButtonEventStates(_buttonEventStates);

    if (_currentPollingType == PollingForConfiguration) {
        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            if (_buttonEventStates[buttonIndex] == ButtonEventDownTransition) {
                qCDebug(JoystickLog) << "Button pressed - button" << buttonIndex;
                emit rawButtonPressedChanged(buttonIndex, true);
            } else if (_buttonEventStates[buttonIndex] == ButtonEventUpTransition) {
                qCDebug(JoystickLog) << "Button released - button" << buttonIndex;
                emit rawButtonPressedChanged(buttonIndex, false);
            }
        }
    } else if (_currentPollingType == PollingForVehicle) {
        Vehicle *const vehicle = _pollingVehicle;
        if (!vehicle) {
            qCWarning(JoystickLog) << "Internal Error: No vehicle for joystick!";
            return;
        }
        if (!_joystickManager->activeJoystickEnabledForActiveVehicle()) {
            qCWarning(JoystickLog) << "Internal Error: Joystick not enabled for vehicle!";
            return;
        }
        if (!_joystickSettings.calibrated()->rawValue().toBool()) {
            return;
        }

        //-- Process button press/release
        const int buttonDelay = static_cast<int>(1000.0 / _joystickSettings.buttonFrequencyHz()->rawValue().toDouble());
        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            if (!_assignedButtonActions[buttonIndex]) {
                continue;
            }
            auto assignedAction = _assignedButtonActions[buttonIndex];

            const QString buttonAction = assignedAction->actionName;
            if (buttonAction.isEmpty() || (buttonAction == _buttonActionNone)) {
                continue;
            }

            auto buttonEventState = _buttonEventStates[buttonIndex];
            if (buttonEventState == ButtonEventDownTransition || buttonEventState == ButtonEventRepeat) {
                if (assignedAction->repeat) {
                    if (assignedAction->buttonElapsedTimer.elapsed() > buttonDelay) {
                        assignedAction->buttonElapsedTimer.start();
                        qCDebug(JoystickLog) << "Repeat - button:action" << buttonIndex << buttonAction;
                        _executeButtonAction(buttonAction, ButtonEventRepeat);
                    }
                } else {
                    if (buttonEventState == ButtonEventDownTransition) {
                        // Check for multi-button action
                        QList<int> multiActionButtons = { buttonIndex };
                        for (int multiIndex = 0; multiIndex < _totalButtonCount; multiIndex++) {
                            if (_assignedButtonActions[multiIndex] && (_assignedButtonActions[multiIndex]->actionName == buttonAction)) {
                                // We found a multi-button action
                                if (_buttonEventStates[multiIndex] == ButtonEventDownTransition || _buttonEventStates[multiIndex] == ButtonEventRepeat) {
                                    // So far so good
                                    multiActionButtons.append(multiIndex);
                                    continue;
                                } else {
                                    // We are missing a press we need
                                    return;
                                }
                            }
                        }

                        if (multiActionButtons.size() > 1) {
                            qCDebug(JoystickLog) << "Multi-button action - buttons:action" << multiActionButtons << buttonAction;
                        } else {
                            qCDebug(JoystickLog) << "Action triggered - button:Action" << buttonIndex << buttonAction;
                        }
                        _executeButtonAction(buttonAction, ButtonEventDownTransition);
                        return;
                    }
                }
            } else if (buttonEventState == ButtonEventUpTransition) {
                qCDebug(JoystickLog) << "Button up - button:action" << buttonIndex << buttonAction;
                _executeButtonAction(buttonAction, ButtonEventUpTransition);
                return;
            }
        }
    }
}

float Joystick::_adjustRange(int value, const AxisCalibration_t &calibration, bool withDeadbands)
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
        if (valueNormalized > calibration.deadband) {
            axisPercent = (valueNormalized - calibration.deadband) / (axisLength - calibration.deadband);
        } else if (valueNormalized<-calibration.deadband) {
            axisPercent = (valueNormalized + calibration.deadband) / (axisLength - calibration.deadband);
        } else {
            axisPercent = 0.f;
        }
    } else {
        axisPercent = valueNormalized / axisLength;
    }

    float correctedValue = axisBasis * axisPercent;
    if (calibration.reversed) {
        correctedValue *= -1.0f;
    }

    return std::max(-1.0f, std::min(correctedValue, 1.0f));
}

void Joystick::_handleAxis()
{
    const int axisDelay = static_cast<int>(1000.0 / _joystickSettings.axisFrequencyHz()->rawValue().toDouble());
    if (_axisElapsedTimer.elapsed() <= axisDelay) {
        return;
    }

    _axisElapsedTimer.start();

    if (_currentPollingType == NotPolling) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling!";
        return;
    }

    if (_currentPollingType == PollingForConfiguration) {
        // Signal the axis values to Joystick Config/Cal. Axes in Joystick terminology are the same as Channels in
        // RemoteControlCalibrationController terminology.
        QVector<int> channelValues(_axisCount);
        for (int axisIndex = 0; axisIndex < _axisCount; axisIndex++) {
            channelValues[axisIndex] = _getAxisValue(axisIndex);
        }
        emit rawChannelValuesChanged(channelValues);
    } else if (_currentPollingType == PollingForVehicle) {
        Vehicle *const vehicle = _pollingVehicle;
        if (!vehicle) {
            qCWarning(JoystickLog) << "Internal Error: No vehicle for joystick!";
            return;
        }
        if (!_joystickManager->activeJoystickEnabledForActiveVehicle()) {
            qCWarning(JoystickLog) << "Internal Error: Joystick not enabled for vehicle!";
            return;
        }
        if (!_joystickSettings.calibrated()->rawValue().toBool()) {
            return;
        }

        bool useDeadband = _joystickSettings.useDeadband()->rawValue().toBool();
        bool throttleModeCenterZero = _joystickSettings.throttleModeCenterZero()->rawValue().toBool();
        bool negativeThrust = _joystickSettings.negativeThrust()->rawValue().toBool();
        bool circleCorrection = _joystickSettings.circleCorrection()->rawValue().toBool();
        bool throttleSmoothing = _joystickSettings.throttleSmoothing()->rawValue().toBool();
        double exponentialPercent = _joystickSettings.exponentialPct()->rawValue().toDouble();

        if (_getJoystickAxisForAxisFunction(rollFunction) == kJoystickAxisNotAssigned ||
            _getJoystickAxisForAxisFunction(pitchFunction) == kJoystickAxisNotAssigned ||
            _getJoystickAxisForAxisFunction(yawFunction) == kJoystickAxisNotAssigned ||
            _getJoystickAxisForAxisFunction(throttleFunction) == kJoystickAxisNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing attitude control axis function mapping!";
            return;
        }
        int axisIndex = _getJoystickAxisForAxisFunction(rollFunction);
        float roll = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        axisIndex = _getJoystickAxisForAxisFunction(pitchFunction);
        float pitch = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        axisIndex = _getJoystickAxisForAxisFunction(yawFunction);
        float yaw = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex],useDeadband);
        axisIndex = _getJoystickAxisForAxisFunction(throttleFunction);
        float throttle = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], throttleModeCenterZero ? useDeadband : false);

        float pitchExtension = qQNaN();
        if (_joystickSettings.enableManualControlPitchExtension()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(pitchExtensionFunction) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing pitch extension axis function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(pitchExtensionFunction);
            pitchExtension = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        }
        float rollExtension = qQNaN();
        if (_joystickSettings.enableManualControlRollExtension()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(rollExtensionFunction) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing roll extension axis function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(rollExtensionFunction);
            rollExtension = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        }
        float aux1 = qQNaN();
        if (_joystickSettings.enableManualControlAux1()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(aux1ExtensionFunction) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing aux1 extension axis function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(aux1ExtensionFunction);
            aux1 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        }
        float aux2 = qQNaN();
        if (_joystickSettings.enableManualControlAux2()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(aux2ExtensionFunction) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing aux2 extension axis function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(aux2ExtensionFunction);
            aux2 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        }
        float aux3 = qQNaN();
        if (_joystickSettings.enableManualControlAux3()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(aux3ExtensionFunction) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing aux3 extension axis function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(aux3ExtensionFunction);
            aux3 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        }
        float aux4 = qQNaN();
        if (_joystickSettings.enableManualControlAux4()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(aux4ExtensionFunction) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing aux4 extension axis function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(aux4ExtensionFunction);
            aux4 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        }
        float aux5 = qQNaN();
        if (_joystickSettings.enableManualControlAux5()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(aux5ExtensionFunction) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing aux5 extension axis function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(aux5ExtensionFunction);
            aux5 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        }
        float aux6 = qQNaN();
        if (_joystickSettings.enableManualControlAux6()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(aux6ExtensionFunction) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing aux6 extension axis function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(aux6ExtensionFunction);
            aux6 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        }

        if (throttleSmoothing) {
            static float throttleSmoothingValue = 0.f;
            throttleSmoothingValue += (throttle * (40 / 1000.f)); // for throttle to change from min to max it will take 1000ms (40ms is a loop time)
            throttleSmoothingValue = std::max(static_cast<float>(-1.f), std::min(throttleSmoothingValue, static_cast<float>(1.f)));
            throttle = throttleSmoothingValue;
        }

        if (circleCorrection) {
            const float roll_limited = std::max(static_cast<float>(-M_PI_4), std::min(roll, static_cast<float>(M_PI_4)));
            const float pitch_limited = std::max(static_cast<float>(-M_PI_4), std::min(pitch, static_cast<float>(M_PI_4)));
            const float yaw_limited = std::max(static_cast<float>(-M_PI_4), std::min(yaw, static_cast<float>(M_PI_4)));
            const float throttle_limited = std::max(static_cast<float>(-M_PI_4), std::min(throttle, static_cast<float>(M_PI_4)));

            // Map from unit circle to linear range and limit
            roll = std::max(-1.0f, std::min(tanf(asinf(roll_limited)), 1.0f));
            pitch = std::max(-1.0f, std::min(tanf(asinf(pitch_limited)), 1.0f));
            yaw = std::max(-1.0f, std::min(tanf(asinf(yaw_limited)), 1.0f));
            throttle = std::max(-1.0f, std::min(tanf(asinf(throttle_limited)), 1.0f));
        }

        if ( exponentialPercent > 0.0 ) {
            const float exponential = -static_cast<float>(exponentialPercent / 100.0);
            roll =  -exponential * powf(roll, 3) + ((1 + exponential) * roll);
            pitch = -exponential * powf(pitch,3) + ((1 + exponential) * pitch);
            yaw =   -exponential * powf(yaw,  3) + ((1 + exponential) * yaw);
        }

        // Adjust throttle to 0:1 range
        if (throttleModeCenterZero && vehicle->supportsThrottleModeCenterZero()) {
            if (!vehicle->supportsNegativeThrust() || !negativeThrust) {
                throttle = std::max(0.0f, throttle);
            }
        } else {
            throttle = (throttle + 1.0f) / 2.0f;
        }

        qCDebug(JoystickVerboseLog)
            << name()
            << "roll:" << roll
            << "pitch:" << -pitch
            << "yaw:" << yaw
            << "throttle:" << throttle
            << "pitchExtension:" << pitchExtension
            << "rollExtension:" << rollExtension
            << "aux1:" << aux1
            << "aux2:" << aux2
            << "aux3:" << aux3
            << "aux4:" << aux4
            << "aux5:" << aux5
            << "aux6:" << aux6;

        // NOTE: The buttonPressedBits going to MANUAL_CONTROL are currently used by ArduSub (and it only handles 16 bits)
        // Set up button bitmap
        quint64 buttonPressedBits = 0;  ///< Buttons pressed for manualControl signal
        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            const quint64 buttonBit = static_cast<quint64>(1LL << buttonIndex);
            if (_buttonEventStates[buttonIndex] == ButtonEventDownTransition || _buttonEventStates[buttonIndex] == ButtonEventRepeat) {
                buttonPressedBits |= buttonBit;
            }
        }

        emit axisValues(roll, pitch, yaw, throttle);

        const uint16_t lowButtons = static_cast<uint16_t>(buttonPressedBits & 0xFFFF);
        const uint16_t highButtons = static_cast<uint16_t>((buttonPressedBits >> 16) & 0xFFFF);


        vehicle->sendJoystickDataThreadSafe(roll, pitch, yaw, throttle, lowButtons, highButtons, pitchExtension, rollExtension, aux1, aux2, aux3, aux4, aux5, aux6);
    }
}

void Joystick::_startPollingForActiveVehicle()
{
    Vehicle *activeVehicle = MultiVehicleManager::instance()->activeVehicle();
    if (!activeVehicle) {
        qCWarning(JoystickLog) << "Internal Error: No active vehicle to poll for";
        return;
    }
    _startPollingForVehicle(*activeVehicle);
}

void Joystick::_startPollingForVehicle(Vehicle &vehicle)
{
    if (_currentPollingType == PollingForVehicle) {
        qCWarning(JoystickLog) << "Internal Error: Joystick already polling for vehicle!";
        return;
    }
    if (_previousPollingType != NotPolling) {
        qCWarning(JoystickLog) << "Internal Error: Joystick previous polling type not None:" << _pollingTypeToString(_previousPollingType);
        return;
    }
    if (_currentPollingType == NotPolling && isRunning()) {
        qCWarning(JoystickLog) << "Internal Error: Joystick polling should not be running!";
        return;
    }

    _currentPollingType = PollingForVehicle;
    _pollingVehicle = &vehicle;

    _buildAvailableButtonsActionList(_pollingVehicle);

    (void) connect(this, &Joystick::setArmed,           _pollingVehicle, &Vehicle::setArmedShowError);
    (void) connect(this, &Joystick::setVtolInFwdFlight, _pollingVehicle, &Vehicle::setVtolInFwdFlight);
    (void) connect(this, &Joystick::setFlightMode,      _pollingVehicle, &Vehicle::setFlightMode);
    (void) connect(this, &Joystick::emergencyStop,      _pollingVehicle, &Vehicle::emergencyStop);
    (void) connect(this, &Joystick::gripperAction,      _pollingVehicle, &Vehicle::sendGripperAction);
    (void) connect(this, &Joystick::landingGearDeploy,  _pollingVehicle, &Vehicle::landingGearDeploy);
    (void) connect(this, &Joystick::landingGearRetract, _pollingVehicle, &Vehicle::landingGearRetract);
    (void) connect(this, &Joystick::motorInterlock,     _pollingVehicle, &Vehicle::motorInterlock);

    (void) connect(_pollingVehicle, &Vehicle::flightModesChanged, this, &Joystick::_flightModesChanged);

    if (GimbalController *const gimbal = _pollingVehicle->gimbalController()) {
        (void) connect(this, &Joystick::gimbalYawLock,      gimbal, &GimbalController::gimbalYawLock);
        (void) connect(this, &Joystick::centerGimbal,       gimbal, &GimbalController::centerGimbal);
        (void) connect(this, &Joystick::gimbalPitchStart,   gimbal, &GimbalController::gimbalPitchStart);
        (void) connect(this, &Joystick::gimbalYawStart,     gimbal, &GimbalController::gimbalYawStart);
        (void) connect(this, &Joystick::gimbalPitchStop,    gimbal, &GimbalController::gimbalPitchStop);
        (void) connect(this, &Joystick::gimbalYawStop,      gimbal, &GimbalController::gimbalYawStop);
    }

    qDebug(JoystickLog) << "Started joystick polling for vehicle" << _pollingVehicle->id();

    start();
}

void Joystick::_startPollingForConfiguration()
{
    if (_currentPollingType == PollingForConfiguration) {
        qCWarning(JoystickLog) << "Internal Error: Joystick already polling for configuration!";
        return;
    }
    if (_previousPollingType != NotPolling) {
        qCWarning(JoystickLog) << "Internal Error: Joystick previous polling type not None:" << _pollingTypeToString(_previousPollingType);
        return;
    }

    qCDebug(JoystickLog) << "Started joystick polling for configuration. Saved previous polling type:" << _pollingTypeToString(_currentPollingType);

    _previousPollingType = _currentPollingType;
    _currentPollingType = PollingForConfiguration;

    if (!isRunning()) {
        start();
    }
}

void Joystick::_stopPollingForConfiguration()
{
    if (_currentPollingType != PollingForConfiguration) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling for configuration!";
        return;
    }
    if (!isRunning()) {
        qCWarning(JoystickLog) << "Internal Error: Joystick polling not running!";
        return;
    }

    qCDebug(JoystickLog) << "Stopped joystick polling for configuration. Restored previous polling type:" << _pollingTypeToString(_previousPollingType);

    _currentPollingType = _previousPollingType;
    _previousPollingType = NotPolling;

    if (_currentPollingType == NotPolling) {
        _exitThread = true;
    }
}

void Joystick::_stopAllPolling()
{
    if (_pollingVehicle) {
        (void) disconnect(this, nullptr, _pollingVehicle, nullptr);
        (void) disconnect(_pollingVehicle, &Vehicle::flightModesChanged, this, &Joystick::_flightModesChanged);
        if (GimbalController *const gimbal = _pollingVehicle->gimbalController()) {
            (void) disconnect(this, nullptr, gimbal, nullptr);
        }
        _pollingVehicle = nullptr;
    }

    qCDebug(JoystickLog) << "Stopped all joystick polling";

    _currentPollingType = NotPolling;
    _previousPollingType = NotPolling;

    if (isRunning()) {
        _exitThread = true;
    }
}

QString Joystick::_pollingTypeToString(PollingType pollingType) const
{
    switch (pollingType) {
    case NotPolling:
        return QStringLiteral("NotPolling");
    case PollingForConfiguration:
        return QStringLiteral("PollingForConfiguration");
    case PollingForVehicle:
        return QStringLiteral("PollingForVehicle");
    default:
        break;
    }

    return QStringLiteral("UnknownPollingType");
}

void Joystick::setAxisCalibration(int axis, const AxisCalibration_t &calibration)
{
    if (!_validAxis(axis)) {
        return;
    }

    _rgCalibration[axis] = calibration;
}

Joystick::AxisCalibration_t Joystick::getAxisCalibration(int axis) const
{
    if (!_validAxis(axis)) {
        return AxisCalibration_t{};
    }

    return _rgCalibration[axis];
}

void Joystick::_setJoystickAxisForAxisFunction(AxisFunction_t axisFunction, int joystickAxis)
{
    if (axisFunction == maxAxisFunction) {
        qCWarning(JoystickLog) << "Internal Error: maxAxisFunction passed to _setJoystickAxisForAxisFunction";
        return;
    }
    if (joystickAxis == kJoystickAxisNotAssigned) {
        qCWarning(JoystickLog) << "Internal Error: kJoystickAxisNotAssigned passed to _setJoystickAxisForAxisFunction";
        return;
    }

    if (!_validAxis(joystickAxis)) {
        return;
    }

    _axisFunctionToJoystickAxisMap[axisFunction] = joystickAxis;
}

int Joystick::_getJoystickAxisForAxisFunction(AxisFunction_t axisFunction) const
{
    if (axisFunction == maxAxisFunction) {
        qCWarning(JoystickLog) << "Internal Error: maxAxisFunction passed to _getJoystickAxisForAxisFunction";
        return kJoystickAxisNotAssigned;
    }

    return _axisFunctionToJoystickAxisMap.value(axisFunction, kJoystickAxisNotAssigned);
}

RemoteControlCalibrationController::StickFunction Joystick::mapAxisFunctionToRCCStickFunction(Joystick::AxisFunction_t axisFunction) const
{
    switch (axisFunction) {
        case rollFunction:
            return RemoteControlCalibrationController::stickFunctionRoll;
        case pitchFunction:
            return RemoteControlCalibrationController::stickFunctionPitch;
        case yawFunction:
            return RemoteControlCalibrationController::stickFunctionYaw;
        case throttleFunction:
            return RemoteControlCalibrationController::stickFunctionThrottle;
        case aux1ExtensionFunction:
            return RemoteControlCalibrationController::stickFunctionAux1Extension;
        case aux2ExtensionFunction:
            return RemoteControlCalibrationController::stickFunctionAux2Extension;
        case aux3ExtensionFunction:
            return RemoteControlCalibrationController::stickFunctionAux3Extension;
        case aux4ExtensionFunction:
            return RemoteControlCalibrationController::stickFunctionAux4Extension;
        case aux5ExtensionFunction:
            return RemoteControlCalibrationController::stickFunctionAux5Extension;
        case aux6ExtensionFunction:
            return RemoteControlCalibrationController::stickFunctionAux6Extension;
        case pitchExtensionFunction:
            return RemoteControlCalibrationController::stickFunctionPitchExtension;
        case rollExtensionFunction:
            return RemoteControlCalibrationController::stickFunctionRollExtension;
        case maxAxisFunction:
            return RemoteControlCalibrationController::stickFunctionMax;
    }
    Q_UNREACHABLE();
}

Joystick::AxisFunction_t Joystick::mapRCCStickFunctionToAxisFunction(RemoteControlCalibrationController::StickFunction stickFunction) const
{
    switch (stickFunction) {
        case RemoteControlCalibrationController::stickFunctionRoll:
            return rollFunction;
        case RemoteControlCalibrationController::stickFunctionPitch:
            return pitchFunction;
        case RemoteControlCalibrationController::stickFunctionYaw:
            return yawFunction;
        case RemoteControlCalibrationController::stickFunctionThrottle:
            return throttleFunction;
        case RemoteControlCalibrationController::stickFunctionAux1Extension:
            return aux1ExtensionFunction;
        case RemoteControlCalibrationController::stickFunctionAux2Extension:
            return aux2ExtensionFunction;
        case RemoteControlCalibrationController::stickFunctionAux3Extension:
            return aux3ExtensionFunction;
        case RemoteControlCalibrationController::stickFunctionAux4Extension:
            return aux4ExtensionFunction;
        case RemoteControlCalibrationController::stickFunctionAux5Extension:
            return aux5ExtensionFunction;
        case RemoteControlCalibrationController::stickFunctionAux6Extension:
            return aux6ExtensionFunction;
        case RemoteControlCalibrationController::stickFunctionPitchExtension:
            return pitchExtensionFunction;
        case RemoteControlCalibrationController::stickFunctionRollExtension:
            return rollExtensionFunction;
        case RemoteControlCalibrationController::stickFunctionMax:
            return maxAxisFunction;
    }
    Q_UNREACHABLE();
}

void Joystick::setFunctionForChannel(RemoteControlCalibrationController::StickFunction stickFunction, int channel)
{
    _setJoystickAxisForAxisFunction(mapRCCStickFunctionToAxisFunction(stickFunction), channel);
}

int Joystick::getChannelForFunction(RemoteControlCalibrationController::StickFunction stickFunction) const
{
    return _getJoystickAxisForAxisFunction(mapRCCStickFunctionToAxisFunction(stickFunction));
}

void Joystick::setButtonRepeat(int button, bool repeat)
{
    if (!_validButton(button) || !_assignedButtonActions[button]) {
        return;
    }

    _assignedButtonActions[button]->repeat = repeat;
    _assignedButtonActions[button]->buttonElapsedTimer.start();

    QSettings settings;
    settings.beginGroup(_joystickSettings.settingsGroup());
    settings.beginGroup(QString::fromLatin1(kButtonActionArrayGroup));
    settings.beginGroup(QString::number(button));
    const QString repeatKey = QString::fromLatin1(kButtonRepeatKey);
    settings.setValue(repeatKey, repeat);
    settings.endGroup();
    settings.endGroup();
    settings.endGroup();
}

bool Joystick::getButtonRepeat(int button)
{
    if (!_validButton(button) || !_assignedButtonActions[button]) {
        return false;
    }

    return _assignedButtonActions[button]->repeat;
}

void Joystick::setButtonAction(int button, const QString& actionName)
{
    if (!_validButton(button)) {
        return;
    }

    qCDebug(JoystickLog) << "button:actionName" << button << actionName;

    QSettings settings;
    settings.beginGroup(_joystickSettings.settingsGroup());
    settings.beginGroup(QString::fromLatin1(kButtonActionArrayGroup));
    settings.beginGroup(QString::number(button));
    const QString actionNameKey = QString::fromLatin1(kButtonActionNameKey);
    const QString repeatKey = QString::fromLatin1(kButtonRepeatKey);

    if (actionName.isEmpty() || actionName == _buttonActionNone) {
        if (_assignedButtonActions[button]) {
            delete _assignedButtonActions[button];
            _assignedButtonActions[button] = nullptr;
        }
        settings.setValue(actionNameKey, _buttonActionNone);
        settings.setValue(repeatKey, false);
    } else {
        if (!_assignedButtonActions[button]) {
            _assignedButtonActions[button] = new AssignedButtonAction(actionName, false /* repeat */);
        } else {
            _assignedButtonActions[button]->actionName = actionName;
            //-- Make sure repeat is off if this action doesn't support repeats
            const int idx = _findAvailableButtonActionIndex(actionName);
            if (idx >= 0) {
                const AvailableButtonAction *const buttonAction = qobject_cast<const AvailableButtonAction*>(_availableButtonActions->get(idx));
                if (!buttonAction->canRepeat()) {
                    _assignedButtonActions[button]->repeat = false;
                }
            }
        }

        settings.setValue(actionNameKey, _assignedButtonActions[button]->actionName);
        settings.setValue(repeatKey, _assignedButtonActions[button]->repeat);
    }

    settings.endGroup();
    settings.endGroup();
    settings.endGroup();

    emit buttonActionsChanged();
}

QString Joystick::getButtonAction(int button) const
{
    if (_validButton(button)) {
        if (_assignedButtonActions[button]) {
            return _assignedButtonActions[button]->actionName;
        }
    }

    return QString(_buttonActionNone);
}

QStringList Joystick::buttonActions() const
{
    QStringList list;

    list.reserve(_totalButtonCount);

    for (int button = 0; button < _totalButtonCount; button++) {
        if (_assignedButtonActions[button]) {
            list << _assignedButtonActions[button]->actionName;
        } else {
            list << _buttonActionNone;
        }
    }

    return list;
}

void Joystick::_executeButtonAction(const QString &action, const ButtonEvent_t buttonEvent)
{
    Vehicle *const vehicle = _pollingVehicle;
    if (!vehicle) {
        qCWarning(JoystickLog) << "Internal Error: No vehicle for joystick!";
        return;
    }
    if (!_joystickManager->activeJoystickEnabledForActiveVehicle()) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not enabled for vehicle!";
        return;
    }
    if (action == _buttonActionNone) {
        return;
    }

    struct ActionInfo {
        const QString action;
        const ButtonEvent_t event;
        std::function<void()> func;
    };
    auto actionInfo = std::to_array<ActionInfo>({
        { _buttonActionArm,                     ButtonEventDownTransition,  [this]() { emit setArmed(true); } },
        { _buttonActionDisarm,                  ButtonEventDownTransition,  [this]() { emit setArmed(false); } },
        { _buttonActionToggleArm,               ButtonEventDownTransition,  [this, vehicle]() { emit setArmed(!vehicle->armed()); } },
        { _buttonActionVTOLFixedWing,           ButtonEventDownTransition,  [this]() { emit setVtolInFwdFlight(true); } },
        { _buttonActionVTOLMultiRotor,          ButtonEventDownTransition,  [this]() { emit setVtolInFwdFlight(false); } },
        { _buttonActionTriggerCamera,           ButtonEventDownTransition,  [this]() { emit triggerCamera(); } },
        { _buttonActionContinuousZoomIn,        ButtonEventDownTransition,  [this]() { emit startContinuousZoom(1); } },
        { _buttonActionContinuousZoomIn,        ButtonEventRepeat,          [this]() { emit startContinuousZoom(1); } },
        { _buttonActionContinuousZoomOut,       ButtonEventDownTransition,  [this]() { emit startContinuousZoom(-1); } },
        { _buttonActionContinuousZoomOut,       ButtonEventRepeat,          [this]() { emit startContinuousZoom(-1); } },
        { _buttonActionStepZoomIn,              ButtonEventDownTransition,  [this]() { emit stepZoom(1); } },
        { _buttonActionStepZoomIn,              ButtonEventRepeat,          [this]() { emit stepZoom(1); } },
        { _buttonActionStepZoomOut,             ButtonEventDownTransition,  [this]() { emit stepZoom(-1); } },
        { _buttonActionStepZoomOut,             ButtonEventRepeat,          [this]() { emit stepZoom(-1); } },
        { _buttonActionNextStream,              ButtonEventDownTransition,  [this]() { emit stepStream(1); } },
        { _buttonActionPreviousStream,          ButtonEventDownTransition,  [this]() { emit stepStream(-1); } },
        { _buttonActionNextCamera,              ButtonEventDownTransition,  [this]() { emit stepCamera(1); } },
        { _buttonActionPreviousCamera,          ButtonEventDownTransition,  [this]() { emit stepCamera(-1); } },
        { _buttonActionGimbalUp,                ButtonEventDownTransition,  [this]() { emit gimbalPitchStart(1); } },
        { _buttonActionGimbalUp,                ButtonEventUpTransition,    [this]() { emit gimbalPitchStop(); } },
        { _buttonActionGimbalDown,              ButtonEventDownTransition,  [this]() { emit gimbalPitchStart(-1); } },
        { _buttonActionGimbalDown,              ButtonEventUpTransition,    [this]() { emit gimbalPitchStop(); } },
        { _buttonActionGimbalLeft,              ButtonEventDownTransition,  [this]() { emit gimbalYawStart(-1); } },
        { _buttonActionGimbalLeft,              ButtonEventUpTransition,    [this]() { emit gimbalYawStop(); } },
        { _buttonActionGimbalRight,             ButtonEventDownTransition,  [this]() { emit gimbalYawStart(1); } },
        { _buttonActionGimbalRight,             ButtonEventUpTransition,    [this]() { emit gimbalYawStop(); } },
        { _buttonActionStartVideoRecord,        ButtonEventDownTransition,  [this]() { emit startVideoRecord(); } },
        { _buttonActionStopVideoRecord,         ButtonEventDownTransition,  [this]() { emit stopVideoRecord(); } },
        { _buttonActionToggleVideoRecord,       ButtonEventDownTransition,  [this]() { emit toggleVideoRecord(); } },
        { _buttonActionGimbalCenter,            ButtonEventDownTransition,  [this]() { emit centerGimbal(); } },
        { _buttonActionGimbalYawLock,           ButtonEventDownTransition,  [this]() { emit gimbalYawLock(true); } },
        { _buttonActionGimbalYawFollow,         ButtonEventDownTransition,  [this]() { emit gimbalYawLock(false); } },
        { _buttonActionEmergencyStop,           ButtonEventDownTransition,  [this]() { emit emergencyStop(); } },
        { _buttonActionGripperGrab,             ButtonEventDownTransition,  [this]() { emit gripperAction(QGCMAVLink::GripperActionGrab); } },
        { _buttonActionGripperRelease,          ButtonEventDownTransition,  [this]() { emit gripperAction(QGCMAVLink::GripperActionRelease); } },
        { _buttonActionGripperHold,             ButtonEventDownTransition,  [this]() { emit gripperAction(QGCMAVLink::GripperActionHold); } },
        { _buttonActionLandingGearDeploy,       ButtonEventDownTransition,  [this]() { emit landingGearDeploy(); } },
        { _buttonActionLandingGearRetract,      ButtonEventDownTransition,  [this]() { emit landingGearRetract(); } },
        { _buttonActionMotorInterlockEnable,    ButtonEventDownTransition,  [this]() { emit motorInterlock(true); } },
        { _buttonActionMotorInterlockDisable,   ButtonEventDownTransition,  [this]() { emit motorInterlock(false); } },
    });

    // First check for flight mode match
    if (vehicle->flightModes().contains(action)) {
        if (buttonEvent == ButtonEventDownTransition) {
            emit setFlightMode(action);
            return;
        }
    }

    // Now look for an action match
    auto it = std::find_if(actionInfo.begin(), actionInfo.end(), [&action, &buttonEvent](const ActionInfo &info) {
        return (info.action == action) && (info.event == buttonEvent);
    });
    if (it != actionInfo.end()) {
        it->func();
        return;
    }

    // Finally let mavlink actions have a go
    if (buttonEvent == ButtonEventDownTransition) {
        emit unknownAction(action);
        for (int i = 0; i<_mavlinkActionManager->actions()->count(); i++) {
            MavlinkAction *const mavlinkAction = _mavlinkActionManager->actions()->value<MavlinkAction*>(i);
            if (action == mavlinkAction->label()) {
                mavlinkAction->sendTo(vehicle);
                return;
            }
        }
    }
}

bool Joystick::_validAxis(int axis) const
{
    if ((axis >= 0) && (axis < _axisCount)) {
        return true;
    }

    qCWarning(JoystickLog) << "Invalid axis index" << axis;
    return false;
}

bool Joystick::_validButton(int button) const
{
    if ((button >= 0) && (button < _totalButtonCount)) {
        return true;
    }

    qCWarning(JoystickLog) << "Invalid button index" << button;
    return false;
}

int Joystick::_findAvailableButtonActionIndex(const QString &action)
{
    for (int i = 0; i < _availableButtonActions->count(); i++) {
        const AvailableButtonAction *const buttonAction = qobject_cast<const AvailableButtonAction*>(_availableButtonActions->get(i));
        if (buttonAction->action() == action) {
            return i;
        }
    }

    return -1;
}

void Joystick::_buildAvailableButtonsActionList(Vehicle *vehicle)
{
    if (_availableButtonActions->count()) {
        _availableButtonActions->clearAndDeleteContents();
    }
    _availableActionTitles.clear();

    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNone, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionArm, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionDisarm, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionToggleArm, false));
    if (vehicle) {
        const QStringList list = vehicle->flightModes();
        for (const QString &mode : list) {
            _availableButtonActions->append(new AvailableButtonAction(mode, false));
        }
    }

    _availableButtonActions->append(new AvailableButtonAction(_buttonActionVTOLFixedWing, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionVTOLMultiRotor, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionContinuousZoomIn, true));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionContinuousZoomOut, true));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStepZoomIn, true));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStepZoomOut, true));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNextStream, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionPreviousStream, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNextCamera, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionPreviousCamera, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionTriggerCamera, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStartVideoRecord, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStopVideoRecord, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionToggleVideoRecord, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalDown, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalUp, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalLeft, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalRight, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalCenter, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalYawLock, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalYawFollow, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionEmergencyStop, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperGrab, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperRelease, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperHold, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionLandingGearDeploy, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionLandingGearRetract, false));
#ifndef QGC_NO_ARDUPILOT_DIALECT
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionMotorInterlockEnable, false));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionMotorInterlockDisable, false));
#endif

    const auto customActions = QGCCorePlugin::instance()->joystickActions();
    for (const auto &action : customActions) {
        _availableButtonActions->append(new AvailableButtonAction(action.name, action.canRepeat));
    }

    for (int i = 0; i < _mavlinkActionManager->actions()->count(); i++) {
        const MavlinkAction *const mavlinkAction = _mavlinkActionManager->actions()->value<const MavlinkAction*>(i);
        _availableButtonActions->append(new AvailableButtonAction(mavlinkAction->label(), false));
    }

    for (int i = 0; i < _availableButtonActions->count(); i++) {
        const AvailableButtonAction *const buttonAction = qobject_cast<const AvailableButtonAction*>(_availableButtonActions->get(i));
        _availableActionTitles << buttonAction->action();
    }

    emit assignableActionsChanged();
}

QString Joystick::axisFunctionToString(AxisFunction_t function)
{
    switch (function) {
    case rollFunction:
        return QStringLiteral("Roll");
    case pitchFunction:
        return QStringLiteral("Pitch");
    case yawFunction:
        return QStringLiteral("Yaw");
    case throttleFunction:
        return QStringLiteral("Throttle");
    case aux1ExtensionFunction:
        return QStringLiteral("Aux 1 Extension");
    case aux2ExtensionFunction:
        return QStringLiteral("Aux 2 Extension");
    case aux3ExtensionFunction:
        return QStringLiteral("Aux 3 Extension");
    case aux4ExtensionFunction:
        return QStringLiteral("Aux 4 Extension");
    case aux5ExtensionFunction:
        return QStringLiteral("Aux 5 Extension");
    case aux6ExtensionFunction:
        return QStringLiteral("Aux 6 Extension");
    case pitchExtensionFunction:
        return QStringLiteral("Pitch Extension");
    case rollExtensionFunction:
        return QStringLiteral("Roll Extension");
    case maxAxisFunction:
        return QStringLiteral("Unassigned");
    }
    Q_UNREACHABLE();
}

void Joystick::startConfiguration()
{
    _startPollingForConfiguration();
}

void Joystick::stopConfiguration()
{
    _stopPollingForConfiguration();
}

Joystick::AxisFunction_t Joystick::_getAxisFunctionForJoystickAxis(int joystickAxis) const
{
    if (!_validAxis(joystickAxis)) {
        return maxAxisFunction;
    }

    for (int i = 0; i < maxAxisFunction; i++) {
        auto axisFunction = static_cast<AxisFunction_t>(i);
        if (_axisFunctionToJoystickAxisMap.value(axisFunction, kJoystickAxisNotAssigned) == joystickAxis) {
            return axisFunction;
        }
    }

    return maxAxisFunction;
}

void Joystick::stop()
{
    _exitThread = true;
    if (isRunning()) {
        if (QThread::currentThread() == this) {
            qCWarning(JoystickLog) << "Skipping wait() on joystick thread";
        } else {
            wait();
        }
    }
}

void Joystick::setLinkedGroupId(const QString &groupId)
{
    if (_linkedGroupId != groupId) {
        _linkedGroupId = groupId;

        // Persist to settings
        QSettings settings;
        settings.beginGroup(QStringLiteral("Joystick"));
        settings.beginGroup(_name);
        if (groupId.isEmpty()) {
            settings.remove(QStringLiteral("LinkedGroupId"));
        } else {
            settings.setValue(QStringLiteral("LinkedGroupId"), groupId);
        }
        settings.endGroup();
        settings.endGroup();

        emit linkedGroupChanged();
    }
}

void Joystick::setLinkedGroupRole(const QString &role)
{
    if (_linkedGroupRole != role) {
        _linkedGroupRole = role;

        // Persist to settings
        QSettings settings;
        settings.beginGroup(QStringLiteral("Joystick"));
        settings.beginGroup(_name);
        if (role.isEmpty()) {
            settings.remove(QStringLiteral("LinkedGroupRole"));
        } else {
            settings.setValue(QStringLiteral("LinkedGroupRole"), role);
        }
        settings.endGroup();
        settings.endGroup();

        emit linkedGroupChanged();
    }
}
