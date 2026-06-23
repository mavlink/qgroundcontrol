#include "Joystick.h"
#include "Fact.h"
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
#include "VehicleSupports.h"
#include "JoystickManager.h"
#include "MultiVehicleManager.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSet>
#include <algorithm>
#include <cmath>
#include <QtCore/QSettings>
#include <QtCore/QThread>

QGC_LOGGING_CATEGORY(JoystickLog, "Joystick.Joystick")
QGC_LOGGING_CATEGORY(JoystickVerboseLog, "Joystick.Joystick:verbose")

static QDebug operator<<(QDebug debug, Joystick::ButtonEvent_t event)
{
    switch (event) {
    case Joystick::ButtonEventDownTransition: return debug << "Down";
    case Joystick::ButtonEventUpTransition:   return debug << "Up";
    case Joystick::ButtonEventRepeat:         return debug << "Repeat";
    case Joystick::ButtonEventNone:           return debug << "None";
    }
    return debug << static_cast<int>(event);
}

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
    qCDebug(JoystickVerboseLog) << this;
}

AvailableButtonAction::AvailableButtonAction(const QString &actionName_, std::function<void()> onDown_, std::function<void()> onUp_, std::function<void()> onRepeat_, QObject *parent)
    : QObject(parent)
    , _actionName(actionName_)
    , _onDown(std::move(onDown_))
    , _onRepeat(std::move(onRepeat_))
    , _onUp(std::move(onUp_))
{
    qCDebug(JoystickVerboseLog) << this;
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
        ensureFactThread(_joystickSettings.enableManualControlPitchExtension());
        ensureFactThread(_joystickSettings.enableManualControlRollExtension());
        ensureFactThread(_joystickSettings.enableAdditionalAxis1());
        ensureFactThread(_joystickSettings.enableAdditionalAxis2());
        ensureFactThread(_joystickSettings.enableAdditionalAxis3());
        ensureFactThread(_joystickSettings.enableAdditionalAxis4());
        ensureFactThread(_joystickSettings.enableAdditionalAxis5());
        ensureFactThread(_joystickSettings.enableAdditionalAxis6());
        ensureFactThread(_joystickSettings.additionalAxesFunction());
    }

    // Changes to manual control extension settings require re-calibration
    connect(_joystickSettings.enableManualControlPitchExtension(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableManualControlRollExtension(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableAdditionalAxis1(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableAdditionalAxis2(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableAdditionalAxis3(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableAdditionalAxis4(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableAdditionalAxis5(), &Fact::rawValueChanged, this, [this]() {
        _joystickSettings.calibrated()->setRawValue(false);
    });
    connect(_joystickSettings.enableAdditionalAxis6(), &Fact::rawValueChanged, this, [this]() {
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
    _exitPollingThread = true;
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
        if (axisFunction == maxAxisFunction) {
            // Older code would save unassigned axes with maxAxisFunction, we now skip loading those since that is not a valid function assignment
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
    bool additionalAxis1FunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableAdditionalAxis1()->rawValue().toBool()) {
        additionalAxis1FunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(additionalAxis1Function) == kJoystickAxisNotAssigned;
        if (additionalAxis1FunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing additional axis 1 function mapping!";
        }
    }
    bool additionalAxis2FunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableAdditionalAxis2()->rawValue().toBool()) {
        additionalAxis2FunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(additionalAxis2Function) == kJoystickAxisNotAssigned;
        if (additionalAxis2FunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing additional axis 2 function mapping!";
        }
    }
    bool additionalAxis3FunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableAdditionalAxis3()->rawValue().toBool()) {
        additionalAxis3FunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(additionalAxis3Function) == kJoystickAxisNotAssigned;
        if (additionalAxis3FunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing additional axis 3 function mapping!";
        }
    }
    bool additionalAxis4FunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableAdditionalAxis4()->rawValue().toBool()) {
        additionalAxis4FunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(additionalAxis4Function) == kJoystickAxisNotAssigned;
        if (additionalAxis4FunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing additional axis 4 function mapping!";
        }
    }
    bool additionalAxis5FunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableAdditionalAxis5()->rawValue().toBool()) {
        additionalAxis5FunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(additionalAxis5Function) == kJoystickAxisNotAssigned;
        if (additionalAxis5FunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing additional axis 5 function mapping!";
        }
    }
    bool additionalAxis6FunctionRequiredButNotAssigned = false;
    if (_joystickSettings.enableAdditionalAxis6()->rawValue().toBool()) {
        additionalAxis6FunctionRequiredButNotAssigned = _getJoystickAxisForAxisFunction(additionalAxis6Function) == kJoystickAxisNotAssigned;
        if (additionalAxis6FunctionRequiredButNotAssigned) {
            qCWarning(JoystickLog) << "Internal Error: Missing additional axis 6 function mapping!";
        }
    }
    if (rollFunctionNotAssigned || pitchFunctionNotAssigned || yawFunctionNotAssigned || throttleFunctionNotAssigned ||
            pitchExtensionFunctionRequiredButNotAssigned || rollExtensionFunctionRequiredButNotAssigned ||
            additionalAxis1FunctionRequiredButNotAssigned || additionalAxis2FunctionRequiredButNotAssigned || additionalAxis3FunctionRequiredButNotAssigned ||
            additionalAxis4FunctionRequiredButNotAssigned || additionalAxis5FunctionRequiredButNotAssigned || additionalAxis6FunctionRequiredButNotAssigned) {
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
    qCDebug(JoystickLog) << "    additionalAxesFunction:" <<            (_joystickSettings.additionalAxesFunction()->rawValue().toUInt() == 1 ? "RC_CHANNELS_OVERRIDE" : "MANUAL_CONTROL");
    qCDebug(JoystickLog) << "    enableAdditionalAxis1:" <<             _joystickSettings.enableAdditionalAxis1()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis2:" <<             _joystickSettings.enableAdditionalAxis2()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis3:" <<             _joystickSettings.enableAdditionalAxis3()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis4:" <<             _joystickSettings.enableAdditionalAxis4()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis5:" <<             _joystickSettings.enableAdditionalAxis5()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis6:" <<             _joystickSettings.enableAdditionalAxis6()->rawValue().toBool();

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
        AxisFunction_t function = _getAxisFunctionForJoystickAxis(axis);
        if (function == maxAxisFunction) {
            // No function assigned to axis, nothing to save
            continue;
        }
        AxisCalibration_t *const calibration = &_rgCalibration[axis];
        axisSettings.beginGroup(QString::number(axis));

        axisSettings.setValue(centerKey, calibration->center);
        axisSettings.setValue(minKey, calibration->min);
        axisSettings.setValue(maxKey, calibration->max);
        axisSettings.setValue(deadbandKey, calibration->deadband);
        axisSettings.setValue(reversedKey, calibration->reversed);
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
    qCDebug(JoystickLog) << "    calibrated:" <<                        calibrated;
    qCDebug(JoystickLog) << "    throttleSmoothing:" <<                 _joystickSettings.throttleSmoothing()->rawValue().toBool();
    qCDebug(JoystickLog) << "    axisFrequencyHz:" <<                   _joystickSettings.axisFrequencyHz()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    buttonFrequencyHz:" <<                 _joystickSettings.buttonFrequencyHz()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    throttleModeCenterZero:" <<            _joystickSettings.throttleModeCenterZero()->rawValue().toBool();
    qCDebug(JoystickLog) << "    negativeThrust:" <<                    _joystickSettings.negativeThrust()->rawValue().toBool();
    qCDebug(JoystickLog) << "    circleCorrection:" <<                  _joystickSettings.circleCorrection()->rawValue().toBool();
    qCDebug(JoystickLog) << "    exponentialPct:" <<                    _joystickSettings.exponentialPct()->rawValue().toDouble();
    qCDebug(JoystickLog) << "    useDeadband:" <<                       _joystickSettings.useDeadband()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlPitchExtension:" << _joystickSettings.enableManualControlPitchExtension()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableManualControlRollExtension:" <<  _joystickSettings.enableManualControlRollExtension()->rawValue().toBool();
    qCDebug(JoystickLog) << "    additionalAxesFunction:" <<            (_joystickSettings.additionalAxesFunction()->rawValue().toUInt() == 1 ? "RC_CHANNELS_OVERRIDE" : "MANUAL_CONTROL");
    qCDebug(JoystickLog) << "    enableAdditionalAxis1:" <<             _joystickSettings.enableAdditionalAxis1()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis2:" <<             _joystickSettings.enableAdditionalAxis2()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis3:" <<             _joystickSettings.enableAdditionalAxis3()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis4:" <<             _joystickSettings.enableAdditionalAxis4()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis5:" <<             _joystickSettings.enableAdditionalAxis5()->rawValue().toBool();
    qCDebug(JoystickLog) << "    enableAdditionalAxis6:" <<             _joystickSettings.enableAdditionalAxis6()->rawValue().toBool();
    qCDebug(JoystickLog) << "    transmitterMode:" <<                   transmitterMode;

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

        while (!_exitPollingThread) {
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

    if ((openFailed || updateFailed) && !_exitPollingThread) {
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
    if (_pollingFlags == PollingNone) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling!";
        return;
    }

    _updateButtonEventStates(_buttonEventStates);

    if (_pollingFlags.testFlag(PollingForConfiguration)) {
        for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
            if (_buttonEventStates[buttonIndex] == ButtonEventDownTransition) {
                qCDebug(JoystickLog) << "Button pressed - button" << buttonIndex;
                emit rawButtonPressedChanged(buttonIndex, true);
            } else if (_buttonEventStates[buttonIndex] == ButtonEventUpTransition) {
                qCDebug(JoystickLog) << "Button released - button" << buttonIndex;
                emit rawButtonPressedChanged(buttonIndex, false);
            }
        }
    } else if (_pollingFlags.testFlag(PollingForVehicle)) {
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
        QSet<QString> executedActions;
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
                        // Post to the GUI thread for safe access to class members.
                        QMetaObject::invokeMethod(this, [this, buttonAction]() {
                            _executeButtonAction(buttonAction, ButtonEventRepeat);
                        }, Qt::QueuedConnection);
                    }
                } else {
                    if (buttonEventState == ButtonEventDownTransition) {
                        // Check for multi-button action
                        QList<int> multiActionButtons = { buttonIndex };
                        bool allActionButtonsPressed = true;
                        for (int multiIndex = 0; multiIndex < _totalButtonCount; multiIndex++) {
                            if (multiIndex == buttonIndex) {
                                continue;
                            }
                            if (_assignedButtonActions[multiIndex] && (_assignedButtonActions[multiIndex]->actionName == buttonAction)) {
                                // We found a multi-button action
                                if (_buttonEventStates[multiIndex] == ButtonEventDownTransition || _buttonEventStates[multiIndex] == ButtonEventRepeat) {
                                    // So far so good
                                    multiActionButtons.append(multiIndex);
                                } else {
                                    // We are missing a press we need, skip this multi-button action only
                                    allActionButtonsPressed = false;
                                    break;
                                }
                            }
                        }

                        if (!allActionButtonsPressed) {
                            continue;
                        }

                        if (multiActionButtons.size() > 1) {
                            qCDebug(JoystickLog) << "Multi-button action - buttons:action" << multiActionButtons << buttonAction;
                        } else {
                            qCDebug(JoystickLog) << "Action triggered - button:Action" << buttonIndex << buttonAction;
                        }
                        // Multiple buttons can share the same action name (multi-button feature).
                        // Guard against executing the action more than once per poll cycle when
                        // several such buttons are all in the same event state simultaneously.
                        if (!executedActions.contains(buttonAction)) {
                            // Post to the GUI thread for safe access to class members.
                            QMetaObject::invokeMethod(this, [this, buttonAction]() {
                                _executeButtonAction(buttonAction, ButtonEventDownTransition);
                            }, Qt::QueuedConnection);
                            executedActions.insert(buttonAction);
                        }
                        continue;
                    }
                }
            } else if (buttonEventState == ButtonEventUpTransition) {
                // Same deduplication guard for release events.
                if (!executedActions.contains(buttonAction)) {
                    // Post to the GUI thread for safe access to class members.
                    QMetaObject::invokeMethod(this, [this, buttonAction]() {
                        _executeButtonAction(buttonAction, ButtonEventUpTransition);
                    }, Qt::QueuedConnection);
                    executedActions.insert(buttonAction);
                }
                continue;
            }
        }
    }
}

float Joystick::_adjustRange(int value, const AxisCalibration_t &calibration, bool withDeadbands)
{
    float valueNormalized;
    float axisLength;
    float axisBasis;

    if (calibration.center == calibration.min) {
        axisBasis = 1.0f;
        valueNormalized = static_cast<float>(std::max(0, value - calibration.center));
        axisLength = calibration.max - calibration.center;
    } else if (calibration.center == calibration.max) {
        axisBasis = -1.0f;
        valueNormalized = static_cast<float>(std::max(0, calibration.center - value));
        axisLength = calibration.center - calibration.min;
    } else if (value > calibration.center) {
        axisBasis = 1.0f;
        valueNormalized = value - calibration.center;
        axisLength =  calibration.max - calibration.center;
    } else {
        axisBasis = -1.0f;
        valueNormalized = calibration.center - value;
        axisLength =  calibration.center - calibration.min;
    }

    if (axisLength <= 0.0f) {
        return 0.0f;
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

uint16_t Joystick::_adjustRangeToRcOverridePwm(int value, const AxisCalibration_t &calibration, bool withDeadbands)
{
    const float normalizedValue = _adjustRange(value, calibration, withDeadbands);
    const bool oneSidedAxis = (calibration.center == calibration.min) || (calibration.center == calibration.max);

    float pwmValue;
    if (oneSidedAxis) {
        pwmValue = 1000.0f + (std::clamp(normalizedValue, 0.0f, 1.0f) * 1000.0f);
    } else {
        pwmValue = 1500.0f + (std::clamp(normalizedValue, -1.0f, 1.0f) * 500.0f);
    }

    return static_cast<uint16_t>(std::lround(std::clamp(pwmValue, 1000.0f, 2000.0f)));
}

void Joystick::_handleAxis()
{
    const int axisDelay = static_cast<int>(1000.0 / _joystickSettings.axisFrequencyHz()->rawValue().toDouble());
    if (_axisElapsedTimer.elapsed() <= axisDelay) {
        return;
    }

    _axisElapsedTimer.start();

    if (_pollingFlags == PollingNone) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling!";
        return;
    }

    if (_pollingFlags.testFlag(PollingForConfiguration)) {
        // Signal the axis values to Joystick Config/Cal. Axes in Joystick terminology are the same as Channels in
        // RemoteControlCalibrationController terminology.
        QVector<int> channelValues(_axisCount);
        for (int axisIndex = 0; axisIndex < _axisCount; axisIndex++) {
            channelValues[axisIndex] = _getAxisValue(axisIndex);
        }
        emit rawChannelValuesChanged(channelValues);
    } else if (_pollingFlags.testFlag(PollingForVehicle)) {
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
        bool additionalAxesFunctionIsManualControl = _joystickSettings.additionalAxesFunction()->rawValue().toUInt() == 0;
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
        std::array<uint16_t, 6> auxRcOverridePwm{};
        std::array<bool, 6> auxRcOverrideEnabled{};

        float auxManualControl1 = qQNaN();
        if (_joystickSettings.enableAdditionalAxis1()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(additionalAxis1Function) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing additional axis 1 function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(additionalAxis1Function);
            if (additionalAxesFunctionIsManualControl) {
                auxManualControl1 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
            } else {
                auxRcOverridePwm[0] = _adjustRangeToRcOverridePwm(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
                auxRcOverrideEnabled[0] = true;
            }
        }
        float auxManualControl2 = qQNaN();
        if (_joystickSettings.enableAdditionalAxis2()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(additionalAxis2Function) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing additional axis 2 function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(additionalAxis2Function);
            if (additionalAxesFunctionIsManualControl) {
                auxManualControl2 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
            } else {
                auxRcOverridePwm[1] = _adjustRangeToRcOverridePwm(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
                auxRcOverrideEnabled[1] = true;
            }
        }
        float auxManualControl3 = qQNaN();
        if (_joystickSettings.enableAdditionalAxis3()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(additionalAxis3Function) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing additional axis 3 function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(additionalAxis3Function);
            if (additionalAxesFunctionIsManualControl) {
                auxManualControl3 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
            } else {
                auxRcOverridePwm[2] = _adjustRangeToRcOverridePwm(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
                auxRcOverrideEnabled[2] = true;
            }
        }
        float auxManualControl4 = qQNaN();
        if (_joystickSettings.enableAdditionalAxis4()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(additionalAxis4Function) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing additional axis 4 function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(additionalAxis4Function);
            if (additionalAxesFunctionIsManualControl) {
                auxManualControl4 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
            } else {
                auxRcOverridePwm[3] = _adjustRangeToRcOverridePwm(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
                auxRcOverrideEnabled[3] = true;
            }
        }
        float auxManualControl5 = qQNaN();
        if (_joystickSettings.enableAdditionalAxis5()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(additionalAxis5Function) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing additional axis 5 function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(additionalAxis5Function);
            if (additionalAxesFunctionIsManualControl) {
                auxManualControl5 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
            } else {
                auxRcOverridePwm[4] = _adjustRangeToRcOverridePwm(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
                auxRcOverrideEnabled[4] = true;
            }
        }
        float auxManualControl6 = qQNaN();
        if (_joystickSettings.enableAdditionalAxis6()->rawValue().toBool()) {
            if (_getJoystickAxisForAxisFunction(additionalAxis6Function) == kJoystickAxisNotAssigned) {
                qCWarning(JoystickLog) << "Internal Error: Missing additional axis 6 function mapping!";
                return;
            }
            axisIndex = _getJoystickAxisForAxisFunction(additionalAxis6Function);
            if (additionalAxesFunctionIsManualControl) {
                auxManualControl6 = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
            } else {
                auxRcOverridePwm[5] = _adjustRangeToRcOverridePwm(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
                auxRcOverrideEnabled[5] = true;
            }
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
        if (throttleModeCenterZero && vehicle->supports()->throttleModeCenterZero()) {
            if (!vehicle->supports()->negativeThrust() || !negativeThrust) {
                throttle = std::max(0.0f, throttle);
            }
        } else {
            throttle = (throttle + 1.0f) / 2.0f;
        }

        if (additionalAxesFunctionIsManualControl) {
            qCDebug(JoystickVerboseLog)
                << name()
                << "roll:" << roll
                << "pitch:" << -pitch
                << "yaw:" << yaw
                << "throttle:" << throttle
                << "pitchExtension:" << pitchExtension
                << "rollExtension:" << rollExtension
                << "additionalAxesFunction: MANUAL_CONTROL"
                << "aux1:" << auxManualControl1
                << "aux2:" << auxManualControl2
                << "aux3:" << auxManualControl3
                << "aux4:" << auxManualControl4
                << "aux5:" << auxManualControl5
                << "aux6:" << auxManualControl6;
        } else {
            qCDebug(JoystickVerboseLog)
                << name()
                << "roll:" << roll
                << "pitch:" << -pitch
                << "yaw:" << yaw
                << "throttle:" << throttle
                << "pitchExtension:" << pitchExtension
                << "rollExtension:" << rollExtension
                << "additionalAxesFunction: RC_CHANNELS_OVERRIDE"
                << "rcOverridePwm[0]:" << auxRcOverridePwm[0] << "enabled:" << auxRcOverrideEnabled[0]
                << "rcOverridePwm[1]:" << auxRcOverridePwm[1] << "enabled:" << auxRcOverrideEnabled[1]
                << "rcOverridePwm[2]:" << auxRcOverridePwm[2] << "enabled:" << auxRcOverrideEnabled[2]
                << "rcOverridePwm[3]:" << auxRcOverridePwm[3] << "enabled:" << auxRcOverrideEnabled[3]
                << "rcOverridePwm[4]:" << auxRcOverridePwm[4] << "enabled:" << auxRcOverrideEnabled[4]
                << "rcOverridePwm[5]:" << auxRcOverridePwm[5] << "enabled:" << auxRcOverrideEnabled[5];
        }

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


        vehicle->sendJoystickDataThreadSafe(roll, pitch, yaw, throttle, lowButtons, highButtons, pitchExtension, rollExtension, auxManualControl1, auxManualControl2, auxManualControl3, auxManualControl4, auxManualControl5, auxManualControl6);
        vehicle->sendJoystickAuxRcOverrideThreadSafe(auxRcOverridePwm, auxRcOverrideEnabled, !additionalAxesFunctionIsManualControl);
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
    qCDebug(JoystickLog) << "Starting joystick polling for vehicle. Vehicle id:" << vehicle.id() << "Current flags:" << _pollingFlagsToString(_pollingFlags);

    if (_pollingFlags.testFlag(PollingForVehicle)) {
        qCWarning(JoystickLog) << "Internal Error: Joystick already polling for vehicle!";
        return;
    }

    _pollingVehicle = &vehicle;
    qCDebug(JoystickLog) << "Started joystick polling for vehicle. Vehicle id:" << _pollingVehicle->id();

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

    _pollingFlags |= PollingForVehicle;
    _startPollingThread();
}

void Joystick::_startPollingForConfiguration()
{
    qCDebug(JoystickLog) << "Starting joystick polling for configuration. Current flags:" << _pollingFlagsToString(_pollingFlags);

    if (_pollingFlags.testFlag(PollingForConfiguration)) {
        qCWarning(JoystickLog) << "Internal Error: Joystick already polling for configuration!";
        return;
    }

    _pollingFlags |= PollingForConfiguration;
    _startPollingThread();
}

void Joystick::_stopPollingForConfiguration()
{
    qCDebug(JoystickLog) << "Stopping joystick polling for configuration. Current flags:" << _pollingFlagsToString(_pollingFlags);

    if (!_pollingFlags.testFlag(PollingForConfiguration)) {
        qCWarning(JoystickLog) << "Internal Error: Joystick not polling for configuration!";
        return;
    }

    if (!isRunning()) {
        qCWarning(JoystickLog) << "Internal Error: Joystick polling thread not running!";
    }

    const PollingFlags remainingFlags = _pollingFlags & ~PollingFlags(PollingForConfiguration);

    // Stop the thread BEFORE updating _pollingFlags. If we cleared the flags first and the
    // thread was still running, _handleButtons()/_handleAxis() would see PollingNone and
    // fire a spurious "Internal Error: Joystick not polling!" warning.
    if (remainingFlags == PollingNone) {
        _stopPollingThread();
    }

    _pollingFlags = remainingFlags;
    qCDebug(JoystickLog) << "Remaining flags:" << _pollingFlagsToString(_pollingFlags);

    if (remainingFlags.testFlag(PollingForVehicle) && !isRunning()) {
        qCWarning(JoystickLog) << "Internal Error: Joystick polling not running! Forcing start of polling thread. Continuing polling for vehicle.";
        _startPollingThread();
    }
}

void Joystick::_stopAllPollingForVehicle()
{
    qCDebug(JoystickLog) << "Stopping all joystick polling for vehicle. Current flags:" << _pollingFlagsToString(_pollingFlags);

    if (_pollingVehicle) {
        _pollingVehicle->sendJoystickAuxRcOverrideThreadSafe({}, {}, false);
        (void) disconnect(this, nullptr, _pollingVehicle, nullptr);
        (void) disconnect(_pollingVehicle, &Vehicle::flightModesChanged, this, &Joystick::_flightModesChanged);
        if (GimbalController *const gimbal = _pollingVehicle->gimbalController()) {
            (void) disconnect(this, nullptr, gimbal, nullptr);
        }
        if (!isRunning()) {
            qCWarning(JoystickLog) << "Joystick polling thread not running even though _pollingVehicle was set.";
        }
    }

    const PollingFlags remainingFlags = _pollingFlags & ~PollingFlags(PollingForVehicle);

    // Stop the thread BEFORE updating _pollingFlags. If we cleared the flags first and the
    // thread was still running, _handleButtons()/_handleAxis() would see PollingNone and
    // fire a spurious "Internal Error: Joystick not polling!" warning.
    if (remainingFlags == PollingNone) {
        _stopPollingThread();
    }

    // Clear _pollingVehicle AFTER updating _pollingFlags so the polling thread never sees
    // PollingForVehicle set with a null _pollingVehicle, which would fire spurious
    // "Internal Error: No vehicle for joystick!" warnings.
    _pollingVehicle = nullptr;
    _pollingFlags = remainingFlags;
    qCDebug(JoystickLog) << "Remaining flags:" << _pollingFlagsToString(_pollingFlags);

    if (remainingFlags.testFlag(PollingForConfiguration) && !isRunning()) {
        qCWarning(JoystickLog) << "Joystick polling thread not running but configuration polling flag set. Forcing start.";
        _startPollingThread();
    }
}

void Joystick::_startPollingThread()
{
    if (isRunning()) {
        qCDebug(JoystickLog) << "Polling thread already running. Flags:" << _pollingFlagsToString(_pollingFlags);
    } else {
        qCDebug(JoystickLog) << "Starting polling thread. Flags:" << _pollingFlagsToString(_pollingFlags);
        _exitPollingThread = false;
        start();
    }
}

void Joystick::_stopPollingThread()
{
    if (isRunning()) {
        qCDebug(JoystickLog) << "Stopping polling thread. Flags:" << _pollingFlagsToString(_pollingFlags);
        _exitPollingThread = true;
        if (QThread::currentThread() == this) {
            qCWarning(JoystickLog) << "Skipping wait() on joystick thread to avoid deadlock";
        } else {
            wait();
        }
        // _exitPollingThread is reset to false in _startPollingThread() before the next start()
    } else {
        qCDebug(JoystickLog) << "Polling thread already stopped. Flags:" << _pollingFlagsToString(_pollingFlags);
    }
}

QString Joystick::_pollingFlagsToString(PollingFlags flags) const
{
    if (flags == PollingNone) {
        return QStringLiteral("None");
    }
    QStringList parts;
    if (flags.testFlag(PollingForVehicle)) {
        parts << QStringLiteral("PollingForVehicle");
    }
    if (flags.testFlag(PollingForConfiguration)) {
        parts << QStringLiteral("PollingForConfiguration");
    }
    return parts.join(QStringLiteral("|"));
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
        case additionalAxis1Function:
            return RemoteControlCalibrationController::stickFunctionAdditionalAxis1;
        case additionalAxis2Function:
            return RemoteControlCalibrationController::stickFunctionAdditionalAxis2;
        case additionalAxis3Function:
            return RemoteControlCalibrationController::stickFunctionAdditionalAxis3;
        case additionalAxis4Function:
            return RemoteControlCalibrationController::stickFunctionAdditionalAxis4;
        case additionalAxis5Function:
            return RemoteControlCalibrationController::stickFunctionAdditionalAxis5;
        case additionalAxis6Function:
            return RemoteControlCalibrationController::stickFunctionAdditionalAxis6;
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
        case RemoteControlCalibrationController::stickFunctionAdditionalAxis1:
            return additionalAxis1Function;
        case RemoteControlCalibrationController::stickFunctionAdditionalAxis2:
            return additionalAxis2Function;
        case RemoteControlCalibrationController::stickFunctionAdditionalAxis3:
            return additionalAxis3Function;
        case RemoteControlCalibrationController::stickFunctionAdditionalAxis4:
            return additionalAxis4Function;
        case RemoteControlCalibrationController::stickFunctionAdditionalAxis5:
            return additionalAxis5Function;
        case RemoteControlCalibrationController::stickFunctionAdditionalAxis6:
            return additionalAxis6Function;
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

    const int idx = _findAvailableButtonActionIndex(action);
    if (idx >= 0) {
        const AvailableButtonAction *const availAction = qobject_cast<const AvailableButtonAction*>(_availableButtonActions->get(idx));
        std::function<void()> handler;
        switch (buttonEvent) {
        case ButtonEventDownTransition:
            handler = availAction->onDown();
            break;
        case ButtonEventRepeat:
            handler = availAction->onRepeat();
            break;
        case ButtonEventUpTransition:
            handler = availAction->onUp();
            break;
        default:
            return;
        }
        if (handler) {
            qCDebug(JoystickLog) << "Button Action:" << action << buttonEvent;
            handler();
            return;
        }
        // No lambda for this event — only DownTransition falls through to flight mode / MAVLink handling
        if (buttonEvent != ButtonEventDownTransition) {
            return;
        }
    }

    if (buttonEvent != ButtonEventDownTransition) {
        return;
    }

    // Flight mode check
    if (vehicle->flightModes().contains(action)) {
        qCDebug(JoystickLog) << "Button Action: Switching flight mode to" << action;
        emit setFlightMode(action);
        return;
    }

    // MAVLink actions
    emit unknownAction(action);
    for (int i = 0; i < _mavlinkActionManager->actions()->count(); i++) {
        MavlinkAction *const mavlinkAction = _mavlinkActionManager->actions()->value<MavlinkAction*>(i);
        if (action == mavlinkAction->label()) {
            qCDebug(JoystickLog) << "Button Action: Sending MAVLink action" << action;
            mavlinkAction->sendTo(vehicle);
            return;
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

    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNone, nullptr));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionArm,
        [this]() { emit setArmed(true); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionDisarm,
        [this]() { emit setArmed(false); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionToggleArm,
        [this]() { emit setArmed(!_pollingVehicle->armed()); }));
    if (vehicle) {
        const QStringList list = vehicle->flightModes();
        for (const QString &mode : list) {
            _availableButtonActions->append(new AvailableButtonAction(mode, nullptr));
        }
    }

    _availableButtonActions->append(new AvailableButtonAction(_buttonActionVTOLFixedWing,
        [this]() { emit setVtolInFwdFlight(true); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionVTOLMultiRotor,
        [this]() { emit setVtolInFwdFlight(false); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionContinuousZoomIn,
        [this]() { emit startContinuousZoom(1); },
        [this]() { emit stopContinuousZoom(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionContinuousZoomOut,
        [this]() { emit startContinuousZoom(-1); },
        [this]() { emit stopContinuousZoom(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStepZoomIn,
        [this]() { emit stepZoom(1); },
        nullptr,
        [this]() { emit stepZoom(1); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStepZoomOut,
        [this]() { emit stepZoom(-1); },
        nullptr,
        [this]() { emit stepZoom(-1); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionContinuousFocusIn,
        [this]() { emit startContinuousFocus(1); },
        [this]() { emit stopContinuousFocus(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionContinuousFocusOut,
        [this]() { emit startContinuousFocus(-1); },
        [this]() { emit stopContinuousFocus(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStepFocusIn,
        [this]() { emit stepFocus(1); },
        nullptr,
        [this]() { emit stepFocus(1); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStepFocusOut,
        [this]() { emit stepFocus(-1); },
        nullptr,
        [this]() { emit stepFocus(-1); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNextStream,
        [this]() { emit stepStream(1); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionPreviousStream,
        [this]() { emit stepStream(-1); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionNextCamera,
        [this]() { emit stepCamera(1); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionPreviousCamera,
        [this]() { emit stepCamera(-1); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionTriggerCamera,
        [this]() { emit triggerCamera(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStartVideoRecord,
        [this]() { emit startVideoRecord(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionStopVideoRecord,
        [this]() { emit stopVideoRecord(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionToggleVideoRecord,
        [this]() { emit toggleVideoRecord(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalDown,
        [this]() { emit gimbalPitchStart(-1); },
        [this]() { emit gimbalPitchStop(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalUp,
        [this]() { emit gimbalPitchStart(1); },
        [this]() { emit gimbalPitchStop(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalLeft,
        [this]() { emit gimbalYawStart(-1); },
        [this]() { emit gimbalYawStop(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalRight,
        [this]() { emit gimbalYawStart(1); },
        [this]() { emit gimbalYawStop(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalCenter,
        [this]() { emit centerGimbal(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalYawLock,
        [this]() { emit gimbalYawLock(true); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGimbalYawFollow,
        [this]() { emit gimbalYawLock(false); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionEmergencyStop,
        [this]() { emit emergencyStop(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperGrab,
        [this]() { emit gripperAction(GRIPPER_ACTION_GRAB); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperRelease,
        [this]() { emit gripperAction(GRIPPER_ACTION_RELEASE); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionGripperHold,
        [this]() { emit gripperAction(GRIPPER_ACTION_HOLD); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionLandingGearDeploy,
        [this]() { emit landingGearDeploy(); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionLandingGearRetract,
        [this]() { emit landingGearRetract(); }));
#ifndef QGC_NO_ARDUPILOT_DIALECT
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionMotorInterlockEnable,
        [this]() { emit motorInterlock(true); }));
    _availableButtonActions->append(new AvailableButtonAction(_buttonActionMotorInterlockDisable,
        [this]() { emit motorInterlock(false); }));
#endif

    const auto customActions = QGCCorePlugin::instance()->joystickActions();
    for (const auto &action : customActions) {
        // onDown is nullptr — dispatch falls through to unknownAction (DownTransition only).
        // A non-null onRepeat makes canRepeat() return true, preserving the UI toggle,
        // but custom plugin repeat dispatch is not supported (unknownAction carries no event type).
        std::function<void()> repeatFn = action.canRepeat ? std::function<void()>([]{}) : nullptr;
        _availableButtonActions->append(new AvailableButtonAction(action.name, nullptr, nullptr, repeatFn));
    }

    for (int i = 0; i < _mavlinkActionManager->actions()->count(); i++) {
        const MavlinkAction *const mavlinkAction = _mavlinkActionManager->actions()->value<const MavlinkAction*>(i);
        _availableButtonActions->append(new AvailableButtonAction(mavlinkAction->label(), nullptr));
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
    case additionalAxis1Function:
        return QStringLiteral("Additional Axis 1");
    case additionalAxis2Function:
        return QStringLiteral("Additional Axis 2");
    case additionalAxis3Function:
        return QStringLiteral("Additional Axis 3");
    case additionalAxis4Function:
        return QStringLiteral("Additional Axis 4");
    case additionalAxis5Function:
        return QStringLiteral("Additional Axis 5");
    case additionalAxis6Function:
        return QStringLiteral("Additional Axis 6");
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
    _exitPollingThread = true;
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
