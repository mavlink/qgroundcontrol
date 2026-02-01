#include "Joystick.h"
#include "MavlinkAction.h"
#include "MavlinkActionManager.h"
#include "MavlinkActionsSettings.h"
#include "FirmwarePlugin.h"
#include "GimbalController.h"
#include "QGCCorePlugin.h"
#include <QtCore/QLoggingCategory>
#include "QmlObjectListModel.h"
#include "SettingsManager.h"
#include "Vehicle.h"
#include "JoystickManager.h"
#include "MultiVehicleManager.h"

#include <QtCore/QCoreApplication>
#include <algorithm>
#include <QtCore/QSettings>
#include <QtCore/QThread>

Q_STATIC_LOGGING_CATEGORY(JoystickLog, "Joystick.Joystick")
Q_STATIC_LOGGING_CATEGORY(JoystickVerboseLog, "Joystick.Joystick:verbose")

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
static constexpr const char* kLegacySettingsGroup = "Joysticks";
static constexpr const char* kLegacyCalibratedKey = "Calibrated4";
static constexpr const char* kLegacyCircleCorrectionKey = "Circle_Correction";
static constexpr const char* kLegacyDeadbandKey = "Deadband";
static constexpr const char* kLegacyNegativeThrustKey = "NegativeThrust";
static constexpr const char* kLegacyAccumulatorKey = "Accumulator";
static constexpr const char* kLegacyAxisFrequencyKey = "AxisFrequency";
static constexpr const char* kLegacyButtonFrequencyKey = "ButtonFrequency";
static constexpr const char* kLegacyThrottleModeKey = "ThrottleMode";
static constexpr const char* kLegacyManualControlExtensionsKey = "ManualControlExtensionsEnabled";
static constexpr const char* kLegacyExponentialKey = "Exponential";
static constexpr const char* kLegacyAxisMinTemplate = "Axis%1Min";
static constexpr const char* kLegacyAxisMaxTemplate = "Axis%1Max";
static constexpr const char* kLegacyAxisCenterTemplate = "Axis%1Trim";
static constexpr const char* kLegacyAxisDeadbandTemplate = "Axis%1Deadbnd";
static constexpr const char* kLegacyAxisReversedTemplate = "Axis%1Rev";
static constexpr const char* kLegacyButtonActionNameTemplate = "ButtonActionName%1";
static constexpr const char* kLegacyButtonActionRepeatTemplate = "ButtonActionRepeat%1";
static constexpr const char* kLegacyMigrationMarker = "_legacyMigrated";
static constexpr const char* kLegacyTxModeKeys[] = {
    "TXMode_MultiRotor",
    "TXMode_FixedWing",
    "TXMode_VTOL",
    "TXMode_Rover",
    "TXMode_Submarine"
};
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
    , _joystickSettings(name, _axisCount, _totalButtonCount)
{
    qCDebug(JoystickLog)
        << name
        << "axisCount" << axisCount
        << "buttonCount" << buttonCount
        << "hatCount" << hatCount
        << this;

    _migrateLegacySettings();
    _loadFromSettingsIntoCalibrationData();

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
        ensureFactThread(_joystickSettings.enableManualControlExtensions());
        ensureFactThread(_joystickSettings.transmitterMode());
        ensureFactThread(_joystickSettings.exponentialPct());
    }

    _buildActionList(MultiVehicleManager::instance()->activeVehicle());
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

    _availableButtonActions->clearAndDeleteContents();
    qDeleteAll(_assignedButtonActions);

    qCDebug(JoystickLog) << this;
}

void Joystick::_migrateLegacySettings()
{
    const QString newSettingsGroup = _joystickSettings.settingsGroup();

    QSettings newFormatSettings;
    newFormatSettings.beginGroup(newSettingsGroup);
    const bool alreadyMigrated = newFormatSettings.value(QString::fromLatin1(kLegacyMigrationMarker), false).toBool();
    bool hasNewFormatAxisData = false;
    if (!alreadyMigrated) {
        newFormatSettings.beginGroup(QString::fromLatin1(kAxisSettingsArrayGroup));
        hasNewFormatAxisData = !newFormatSettings.childGroups().isEmpty();
        newFormatSettings.endGroup();
    }
    newFormatSettings.endGroup();

    if (alreadyMigrated || hasNewFormatAxisData) {
        if (!alreadyMigrated) {
            QSettings sentinelSettings;
            sentinelSettings.beginGroup(newSettingsGroup);
            sentinelSettings.setValue(QString::fromLatin1(kLegacyMigrationMarker), true);
            sentinelSettings.endGroup();
        }
        return;
    }

    QSettings legacySettings;
    legacySettings.beginGroup(QString::fromLatin1(kLegacySettingsGroup));

    if (!legacySettings.childGroups().contains(_name)) {
        legacySettings.endGroup();
        QSettings sentinelSettings;
        sentinelSettings.beginGroup(newSettingsGroup);
        sentinelSettings.setValue(QString::fromLatin1(kLegacyMigrationMarker), true);
        sentinelSettings.endGroup();
        return;
    }

    int legacyTransmitterMode = -1;
    for (const char* key : kLegacyTxModeKeys) {
        const QString txModeKey = QString::fromLatin1(key);
        if (legacySettings.contains(txModeKey)) {
            legacyTransmitterMode = legacySettings.value(txModeKey).toInt();
            if (legacyTransmitterMode > 0) {
                break;
            }
        }
    }

    legacySettings.beginGroup(_name);

    if (legacySettings.allKeys().isEmpty()) {
        legacySettings.endGroup();
        legacySettings.endGroup();
        QSettings sentinelSettings;
        sentinelSettings.beginGroup(newSettingsGroup);
        sentinelSettings.setValue(QString::fromLatin1(kLegacyMigrationMarker), true);
        sentinelSettings.endGroup();
        return;
    }

    qCInfo(JoystickLog) << "Migrating legacy joystick settings for" << _name;

    auto setBoolIfPresent = [&](Fact* fact, const char* key) {
        if (!fact) {
            return;
        }
        const QString keyString = QString::fromLatin1(key);
        if (legacySettings.contains(keyString)) {
            fact->setRawValue(legacySettings.value(keyString).toBool());
        }
    };

    setBoolIfPresent(_joystickSettings.calibrated(), kLegacyCalibratedKey);
    setBoolIfPresent(_joystickSettings.circleCorrection(), kLegacyCircleCorrectionKey);
    setBoolIfPresent(_joystickSettings.useDeadband(), kLegacyDeadbandKey);
    setBoolIfPresent(_joystickSettings.negativeThrust(), kLegacyNegativeThrustKey);
    setBoolIfPresent(_joystickSettings.throttleSmoothing(), kLegacyAccumulatorKey);
    setBoolIfPresent(_joystickSettings.enableManualControlExtensions(), kLegacyManualControlExtensionsKey);

    const QString axisFrequencyKey = QString::fromLatin1(kLegacyAxisFrequencyKey);
    if (legacySettings.contains(axisFrequencyKey)) {
        _joystickSettings.axisFrequencyHz()->setRawValue(legacySettings.value(axisFrequencyKey).toDouble());
    }

    const QString buttonFrequencyKey = QString::fromLatin1(kLegacyButtonFrequencyKey);
    if (legacySettings.contains(buttonFrequencyKey)) {
        _joystickSettings.buttonFrequencyHz()->setRawValue(legacySettings.value(buttonFrequencyKey).toDouble());
    }

    const QString throttleModeKey = QString::fromLatin1(kLegacyThrottleModeKey);
    if (legacySettings.contains(throttleModeKey)) {
        const int throttleMode = legacySettings.value(throttleModeKey).toInt();
        _joystickSettings.throttleModeCenterZero()->setRawValue(throttleMode == 0);
    }

    const QString exponentialKey = QString::fromLatin1(kLegacyExponentialKey);
    if (legacySettings.contains(exponentialKey)) {
        const float legacyExponent = legacySettings.value(exponentialKey).toFloat();
        double exponentialPercent = 0.0;
        if (legacyExponent < 0.0f) {
            exponentialPercent = std::min(50.0, std::max(0.0, static_cast<double>(-legacyExponent * 100.0f)));
        }
        _joystickSettings.exponentialPct()->setRawValue(exponentialPercent);
    }

    if (legacyTransmitterMode >= 1 && legacyTransmitterMode <= 4) {
        _joystickSettings.transmitterMode()->setRawValue(legacyTransmitterMode);
    }

    QVector<int> axisFunctionForAxis(_axisCount, maxAxisFunction);
    for (int functionIndex = 0; functionIndex < maxAxisFunction; ++functionIndex) {
        const QString functionKey = QString::fromLatin1(_rgFunctionSettingsKey[functionIndex]);
        const int storedAxis = legacySettings.value(functionKey, -1).toInt();
        if (storedAxis >= 0 && storedAxis < _axisCount) {
            axisFunctionForAxis[storedAxis] = functionIndex;
        }
    }

    AxisCalibration_t defaultCalibration;

    QSettings axisSettings;
    axisSettings.beginGroup(newSettingsGroup);
    axisSettings.beginGroup(QString::fromLatin1(kAxisSettingsArrayGroup));

    for (int axisIndex = 0; axisIndex < _axisCount; ++axisIndex) {
        const int minValue = legacySettings.value(QString::fromLatin1(kLegacyAxisMinTemplate).arg(axisIndex), defaultCalibration.min).toInt();
        const int maxValue = legacySettings.value(QString::fromLatin1(kLegacyAxisMaxTemplate).arg(axisIndex), defaultCalibration.max).toInt();
        const int centerValue = legacySettings.value(QString::fromLatin1(kLegacyAxisCenterTemplate).arg(axisIndex), defaultCalibration.center).toInt();
        const int deadbandValue = legacySettings.value(QString::fromLatin1(kLegacyAxisDeadbandTemplate).arg(axisIndex), defaultCalibration.deadband).toInt();
        const bool reversedValue = legacySettings.value(QString::fromLatin1(kLegacyAxisReversedTemplate).arg(axisIndex), defaultCalibration.reversed).toBool();

        axisSettings.beginGroup(QString::number(axisIndex));
        axisSettings.setValue(QString::fromLatin1(kAxisFunctionKey), axisFunctionForAxis[axisIndex]);
        axisSettings.setValue(QString::fromLatin1(kAxisMinKey), minValue);
        axisSettings.setValue(QString::fromLatin1(kAxisMaxKey), maxValue);
        axisSettings.setValue(QString::fromLatin1(kAxisCenterKey), centerValue);
        axisSettings.setValue(QString::fromLatin1(kAxisDeadbandKey), deadbandValue);
        axisSettings.setValue(QString::fromLatin1(kAxisReversedKey), reversedValue);
        axisSettings.endGroup();
    }

    axisSettings.endGroup();
    axisSettings.endGroup();

    QSettings buttonSettings;
    buttonSettings.beginGroup(newSettingsGroup);
    buttonSettings.beginGroup(QString::fromLatin1(kButtonActionArrayGroup));

    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; ++buttonIndex) {
        const QString actionKey = QString::fromLatin1(kLegacyButtonActionNameTemplate).arg(buttonIndex);
        if (!legacySettings.contains(actionKey)) {
            continue;
        }

        const QString actionName = legacySettings.value(actionKey).toString();
        if (actionName.isEmpty() || actionName == QString::fromLatin1(_buttonActionNone)) {
            continue;
        }

        const QString repeatKey = QString::fromLatin1(kLegacyButtonActionRepeatTemplate).arg(buttonIndex);
        const bool repeatValue = legacySettings.value(repeatKey, false).toBool();

        buttonSettings.beginGroup(QString::number(buttonIndex));
        buttonSettings.setValue(QString::fromLatin1(kButtonActionNameKey), actionName);
        buttonSettings.setValue(QString::fromLatin1(kButtonRepeatKey), repeatValue);
        buttonSettings.endGroup();
    }

    buttonSettings.endGroup();
    buttonSettings.endGroup();

    legacySettings.endGroup();
    legacySettings.endGroup();

    QSettings sentinelSettings;
    sentinelSettings.beginGroup(newSettingsGroup);
    sentinelSettings.setValue(QString::fromLatin1(kLegacyMigrationMarker), true);
    sentinelSettings.endGroup();
}

void Joystick::_loadFromSettingsIntoCalibrationData()
{
    bool calibrated = _joystickSettings.calibrated()->rawValue().toBool();
    int transmitterMode = _joystickSettings.transmitterMode()->rawValue().toInt();

    qCDebug(JoystickLog) << name();
    qCDebug(JoystickLog)
        << "    "
        << "calibrated" << calibrated
        << "throttleSmoothing" << _joystickSettings.throttleSmoothing()->rawValue().toBool()
        << "axisFrequencyHz" << _joystickSettings.axisFrequencyHz()->rawValue().toDouble()
        << "buttonFrequencyHz" << _joystickSettings.buttonFrequencyHz()->rawValue().toDouble()
        << "throttleModeCenterZero" << _joystickSettings.throttleModeCenterZero()->rawValue().toBool()
        << "negativeThrust" << _joystickSettings.negativeThrust()->rawValue().toBool()
        << "circleCorrection" << _joystickSettings.circleCorrection()->rawValue().toBool()
        << "exponentialPct" << _joystickSettings.exponentialPct()->rawValue().toDouble()
        << "enableManualControlExtensions" << _joystickSettings.enableManualControlExtensions()->rawValue().toBool()
        << "useDeadband" << _joystickSettings.useDeadband()->rawValue().toBool()
        << "transmitterMode" << transmitterMode;

    QSettings axisSettings;
    axisSettings.beginGroup(_joystickSettings.settingsGroup());
    axisSettings.beginGroup(QString::fromLatin1(kAxisSettingsArrayGroup));

    const QString functionKey = QString::fromLatin1(kAxisFunctionKey);
    const QString minKey = QString::fromLatin1(kAxisMinKey);
    const QString maxKey = QString::fromLatin1(kAxisMaxKey);
    const QString centerKey = QString::fromLatin1(kAxisCenterKey);
    const QString deadbandKey = QString::fromLatin1(kAxisDeadbandKey);
    const QString reversedKey = QString::fromLatin1(kAxisReversedKey);

    for (int function = 0; function < maxAxisFunction; function++) {
        _rgFunctionAxis[function] = -1;
    }

    for (int axis = 0; axis < _axisCount; axis++) {
        AxisCalibration_t *const calibration = &_rgCalibration[axis];
        axisSettings.beginGroup(QString::number(axis));

        const int function = axisSettings.value(functionKey, maxAxisFunction).toInt();
        if (function < 0 || function > maxAxisFunction) {
            qCWarning(JoystickLog) << "Invalid function" << function << "for axis" << axis;
            axisSettings.endGroup();
            continue;
        }

        if (calibrated && function == maxAxisFunction) {
            qCWarning(JoystickLog) << "Joystick is marked calibrated but axis" << axis << "has no function assigned";
        }

        calibration->center = axisSettings.value(centerKey, calibration->center).toInt();
        calibration->min = axisSettings.value(minKey, calibration->min).toInt();
        calibration->max = axisSettings.value(maxKey, calibration->max).toInt();
        calibration->deadband = axisSettings.value(deadbandKey, calibration->deadband).toInt();
        calibration->reversed = axisSettings.value(reversedKey, calibration->reversed).toBool();

        if (function >= 0 && function < maxAxisFunction) {
            _rgFunctionAxis[function] = axis;
        }

        qDebug(JoystickLog)
            << "    "
            << "axis" << axis
            << "min" << calibration->min
            << "max" << calibration->max
            << "center" << calibration->center
            << "reversed" << calibration->reversed
            << "deadband" << calibration->deadband
            << "function" << axisFunctionToString(static_cast<AxisFunction_t>(function));

        axisSettings.endGroup();
    }

    axisSettings.endGroup();
    axisSettings.endGroup();

    // FunctionAxis mappings are always stored in TX mode 2
    // Remap to stored TX mode in settings for UI display
    _remapAxes (2, transmitterMode, _rgFunctionAxis);

    // Rebuild assigned button actions
    for (int i = 0; i < _assignedButtonActions.count(); i++) {
        delete _assignedButtonActions[i];
        _assignedButtonActions[i] = nullptr;
    }

    QSettings buttonSettings;
    buttonSettings.beginGroup(_joystickSettings.settingsGroup());
    buttonSettings.beginGroup(QString::fromLatin1(kButtonActionArrayGroup));
    const QString actionNameKey = QString::fromLatin1(kButtonActionNameKey);
    const QString repeatKey = QString::fromLatin1(kButtonRepeatKey);

    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        buttonSettings.beginGroup(QString::number(buttonIndex));
        const QString actionName = buttonSettings.value(actionNameKey).toString();
        const bool repeat = buttonSettings.value(repeatKey, false).toBool();
        buttonSettings.endGroup();

        if (actionName.isEmpty() || (actionName == _buttonActionNone)) {
            continue;
        }

        AssignedButtonAction *buttonAction = new AssignedButtonAction(actionName, repeat);
        _assignedButtonActions[buttonIndex] = buttonAction;
        _assignedButtonActions[buttonIndex]->buttonElapsedTimer.start();

        qCDebug(JoystickLog)
            << "    "
            << "button" << buttonIndex
            << "actionName" << actionName
            << "repeat" << repeat;
    }

    buttonSettings.endGroup();
    buttonSettings.endGroup();
}

void Joystick::_saveFromCalibrationDataIntoSettings()
{
    bool calibrated = _joystickSettings.calibrated()->rawValue().toBool();
    int transmitterMode = _joystickSettings.transmitterMode()->rawValue().toInt();

    qCDebug(JoystickLog) << name();
    qCDebug(JoystickLog)
        << "    "
        << "calibrated" << calibrated
        << "throttleSmoothing" << _joystickSettings.throttleSmoothing()->rawValue().toBool()
        << "axisFrequencyHz" << _joystickSettings.axisFrequencyHz()->rawValue().toDouble()
        << "buttonFrequencyHz" << _joystickSettings.buttonFrequencyHz()->rawValue().toDouble()
        << "throttleModeCenterZero" << _joystickSettings.throttleModeCenterZero()->rawValue().toBool()
        << "negativeThrust" << _joystickSettings.negativeThrust()->rawValue().toBool()
        << "circleCorrection" << _joystickSettings.circleCorrection()->rawValue().toBool()
        << "exponentialPct" << _joystickSettings.exponentialPct()->rawValue().toDouble()
        << "enableManualControlExtensions" << _joystickSettings.enableManualControlExtensions()->rawValue().toBool()
        << "useDeadband" << _joystickSettings.useDeadband()->rawValue().toBool()
        << "transmitterMode" << transmitterMode;


    // FunctionAxis mappings are always stored in TX mode 2
    // Temporarily remap from calibration TX mode to mode 2 for storage
    _remapAxes (transmitterMode, 2, _rgFunctionAxis);

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
        AxisFunction_t function = _getFunctionForAxis(axis);
        axisSettings.setValue(functionKey, static_cast<int>(function));

        qDebug(JoystickLog)
            << "    "
            << "axis" << axis
            << "min" << calibration->min
            << "max" << calibration->max
            << "center" << calibration->center
            << "reversed" << calibration->reversed
            << "deadband" << calibration->deadband
            << "function" << function;

        axisSettings.endGroup();
    }

    axisSettings.endGroup();
    axisSettings.endGroup();

    // Map back to current TX mode
    _remapAxes (2, transmitterMode, _rgFunctionAxis);

    // Save button actions
    QSettings buttonSettings;
    buttonSettings.beginGroup(_joystickSettings.settingsGroup());
    buttonSettings.beginGroup(QString::fromLatin1(kButtonActionArrayGroup));
    const QString actionNameKey = QString::fromLatin1(kButtonActionNameKey);
    const QString repeatKey = QString::fromLatin1(kButtonRepeatKey);

    for (int buttonIndex = 0; buttonIndex < _totalButtonCount; buttonIndex++) {
        buttonSettings.beginGroup(QString::number(buttonIndex));

        AssignedButtonAction *buttonAction = _assignedButtonActions[buttonIndex];

        if (buttonAction) {
            buttonSettings.setValue(actionNameKey, buttonAction->actionName);
            buttonSettings.setValue(repeatKey, buttonAction->repeat);
            qCDebug(JoystickLog)
                << "    "
                << "button" << buttonIndex
                << "actionName" << buttonAction->actionName
                << "repeat" << buttonAction->repeat;
        } else {
            buttonSettings.setValue(actionNameKey, _buttonActionNone);
            buttonSettings.setValue(repeatKey, false);
        }

        buttonSettings.endGroup();
    }

    buttonSettings.endGroup();
    buttonSettings.endGroup();
}

int Joystick::_mapFunctionMode(int mode, int function)
{
    static constexpr const int mapping[][6] = {
        { yawFunction, pitchFunction, rollFunction, throttleFunction, gimbalPitchFunction, gimbalYawFunction },
        { yawFunction, throttleFunction, rollFunction, pitchFunction, gimbalPitchFunction, gimbalYawFunction },
        { rollFunction, pitchFunction, yawFunction, throttleFunction, gimbalPitchFunction, gimbalYawFunction },
        { rollFunction, throttleFunction, yawFunction, pitchFunction, gimbalPitchFunction, gimbalYawFunction }
    };

    if ((mode > 0) && (mode <= 4)) {
        return mapping[mode - 1][function];
    }

    return -1;
}

void Joystick::_remapAxes(int fromMode, int toMode, int (&newMapping)[maxAxisFunction])
{
    int temp[maxAxisFunction];

    for (int function = 0; function < maxAxisFunction; function++) {
        temp[_mapFunctionMode(toMode, function)] = _rgFunctionAxis[_mapFunctionMode(fromMode, function)];
    }

    for (int function = 0; function < maxAxisFunction; function++) {
        newMapping[function] = temp[function];
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
        if (!JoystickManager::instance()->joystickEnabledForVehicle(vehicle)) {
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
        if (!JoystickManager::instance()->joystickEnabledForVehicle(vehicle)) {
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
        bool enableManualControlExtensions = _joystickSettings.enableManualControlExtensions()->rawValue().toBool();
        double exponentialPercent = _joystickSettings.exponentialPct()->rawValue().toDouble();

        int axisIndex = _rgFunctionAxis[rollFunction];
        float roll = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        axisIndex = _rgFunctionAxis[pitchFunction];
        float pitch = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], useDeadband);
        axisIndex = _rgFunctionAxis[yawFunction];
        float yaw = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex],useDeadband);
        axisIndex = _rgFunctionAxis[throttleFunction];
        float throttle = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex], throttleModeCenterZero ? useDeadband : false);

        float gimbalPitch = NAN;
        if (enableManualControlExtensions && _axisCount > 4) {
            // Hardcoded to axis 4
            axisIndex = 4;
            gimbalPitch = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex],useDeadband);
        }

        float gimbalYaw = NAN;
        if (enableManualControlExtensions && _axisCount > 5) {
            // Hardcoded to axis 5
            axisIndex = 5;
            gimbalYaw = _adjustRange(_getAxisValue(axisIndex), _rgCalibration[axisIndex],useDeadband);
        }

        if (throttleSmoothing) {
            static float throttleSmoothing = 0.f;
            throttleSmoothing += (throttle * (40 / 1000.f)); // for throttle to change from min to max it will take 1000ms (40ms is a loop time)
            throttleSmoothing = std::max(static_cast<float>(-1.f), std::min(throttleSmoothing, static_cast<float>(1.f)));
            throttle = throttleSmoothing;
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
            << "name" << name()
            << "roll" << roll
            << "pitch" << -pitch
            << "yaw" << yaw
            << "throttle" << throttle
            << "gimbalPitch" << gimbalPitch
            << "gimbalYaw" << gimbalYaw;

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


        vehicle->sendJoystickDataThreadSafe(roll, pitch, yaw, throttle, lowButtons, highButtons, gimbalPitch, gimbalYaw);
    }
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

    _buildActionList(_pollingVehicle);

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

void Joystick::setFunctionAxis(AxisFunction_t function, int axis)
{
    if (!_validAxis(axis)) {
        return;
    }

    _rgFunctionAxis[function] = axis;
}

int Joystick::getFunctionAxis(AxisFunction_t function) const
{
    if ((static_cast<int>(function) < 0) || (function >= maxAxisFunction)) {
        qCWarning(JoystickLog) << "Invalid function" << function;
    }

    return _rgFunctionAxis[function];
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
        case maxAxisFunction:
            return RemoteControlCalibrationController::stickFunctionMax;
        default:
            qCWarning(JoystickLog) << "Invalid AxisFunction_t" << axisFunction;
            return RemoteControlCalibrationController::stickFunctionRoll;
    }
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
        default:
            qCWarning(JoystickLog) << "Invalid StickFunction" << stickFunction;
            return rollFunction;
    }
}

void Joystick::setFunctionForChannel(RemoteControlCalibrationController::StickFunction stickFunction, int channel)
{
    return setFunctionAxis(mapRCCStickFunctionToAxisFunction(stickFunction), channel);
}

int Joystick::getChannelForFunction(RemoteControlCalibrationController::StickFunction stickFunction) const
{
    return getFunctionAxis(mapRCCStickFunctionToAxisFunction(stickFunction));
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
    if (!JoystickManager::instance()->joystickEnabledForVehicle(vehicle)) {
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

void Joystick::_buildActionList(Vehicle *vehicle)
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
    case gimbalPitchFunction:
        return QStringLiteral("Gimbal Pitch");
    case gimbalYawFunction:
        return QStringLiteral("Gimbal Yaw");
    case maxAxisFunction:
        return QStringLiteral("Unassigned");
    default:
        return QStringLiteral("Unknown");
    }
}

void Joystick::startConfiguration()
{
    _startPollingForConfiguration();
}

void Joystick::stopConfiguration()
{
    _stopPollingForConfiguration();
}

Joystick::AxisFunction_t Joystick::_getFunctionForAxis(int axis) const
{
    for (int function = 0; function < maxAxisFunction; function++) {
        if (_rgFunctionAxis[function] == axis) {
            return static_cast<AxisFunction_t>(function);
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
