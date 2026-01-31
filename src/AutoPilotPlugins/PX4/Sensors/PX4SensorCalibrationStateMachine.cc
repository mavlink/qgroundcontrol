#include "PX4SensorCalibrationStateMachine.h"
#include "PX4OrientationCalibrationMachine.h"
#include "SensorsComponentController.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(PX4SensorCalibrationStateMachineLog, "PX4SensorCalibration.StateMachine")

PX4SensorCalibrationStateMachine::PX4SensorCalibrationStateMachine(SensorsComponentController* controller, QObject* parent)
    : SensorCalibrationStateMachineBase("PX4SensorCalibration", controller, parent)
    , _px4Controller(controller)
{
    // Create sub-machine
    _orientationCalMachine = new PX4OrientationCalibrationMachine(controller, this);

    // Connect sub-machine signals
    connect(_orientationCalMachine, &PX4OrientationCalibrationMachine::calibrationComplete, this, [this](bool success) {
        qCDebug(PX4SensorCalibrationStateMachineLog) << "Orientation calibration complete, success:" << success;
        _stopCalibration(success);
    });

    _buildStateMachine();
    _setupTransitions();
}

void PX4SensorCalibrationStateMachine::_buildStateMachine()
{
    // Build common base states (idle, simpleCal, cancelling)
    buildBaseStates();

    // Create PX4-specific states
    _orientationCalState = new QGCState("OrientationCalibration", this);

    // Configure state entry callbacks
    connect(_idleState, &QAbstractState::entered, this, [this]() {
        _unknownFirmwareVersion = false;
    });

    connect(_orientationCalState, &QAbstractState::entered, this, [this]() {
        qCDebug(PX4SensorCalibrationStateMachineLog) << "Entered OrientationCalibration state";
    });

    connect(_cancellingState, &QAbstractState::entered, this, [this]() {
        qCDebug(PX4SensorCalibrationStateMachineLog) << "Entered Cancelling state";
    });
}

void PX4SensorCalibrationStateMachine::_setupTransitions()
{
    // Idle -> various calibrations
    _idleState->addTransition(new MachineEventTransition("start_orientation", _orientationCalState));
    _idleState->addTransition(new MachineEventTransition("start_simple", _simpleCalState));

    // All states back to Idle on complete
    _orientationCalState->addTransition(new MachineEventTransition("complete", _idleState));
    _simpleCalState->addTransition(new MachineEventTransition("complete", _idleState));
    _cancellingState->addTransition(new MachineEventTransition("complete", _idleState));

    // Cancel transitions
    _orientationCalState->addTransition(new MachineEventTransition("cancel", _cancellingState));
    _simpleCalState->addTransition(new MachineEventTransition("cancel", _cancellingState));
}

void PX4SensorCalibrationStateMachine::calibrateCompass()
{
    qCDebug(PX4SensorCalibrationStateMachineLog) << "Starting compass calibration";

    _calType = QGCMAVLink::CalibrationMag;
    _startLogCalibration();
    _px4Controller->_vehicle->startCalibration(_calType);

    emit calibrationStarted(_calType);
}

void PX4SensorCalibrationStateMachine::calibrateGyro()
{
    qCDebug(PX4SensorCalibrationStateMachineLog) << "Starting gyro calibration";

    _calType = QGCMAVLink::CalibrationGyro;
    _startLogCalibration();
    _px4Controller->_vehicle->startCalibration(_calType);

    emit calibrationStarted(_calType);
}

void PX4SensorCalibrationStateMachine::calibrateAccel()
{
    qCDebug(PX4SensorCalibrationStateMachineLog) << "Starting accel calibration";

    _calType = QGCMAVLink::CalibrationAccel;
    _startLogCalibration();
    _px4Controller->_vehicle->startCalibration(_calType);

    emit calibrationStarted(_calType);
}

void PX4SensorCalibrationStateMachine::calibrateLevel()
{
    qCDebug(PX4SensorCalibrationStateMachineLog) << "Starting level calibration";

    _calType = QGCMAVLink::CalibrationLevel;
    _startLogCalibration();
    _px4Controller->_vehicle->startCalibration(_calType);

    if (!isRunning()) {
        start();
    }
    postEvent("start_simple");

    emit calibrationStarted(_calType);
}

void PX4SensorCalibrationStateMachine::calibrateAirspeed()
{
    qCDebug(PX4SensorCalibrationStateMachineLog) << "Starting airspeed calibration";

    _calType = QGCMAVLink::CalibrationPX4Airspeed;
    _startLogCalibration();
    _px4Controller->_vehicle->startCalibration(_calType);

    if (!isRunning()) {
        start();
    }
    postEvent("start_simple");

    emit calibrationStarted(_calType);
}

void PX4SensorCalibrationStateMachine::cancelCalibration()
{
    qCDebug(PX4SensorCalibrationStateMachineLog) << "Cancelling calibration";

    _px4Controller->_cancelButton->setEnabled(false);

    if (_orientationCalMachine->isCalibrating()) {
        _orientationCalMachine->cancelCalibration();
    }

    postEvent("cancel");
    _px4Controller->_vehicle->stopCalibration(true /* showError */);
}

void PX4SensorCalibrationStateMachine::handleTextMessage(const QString& text)
{
    // Handle progress messages specially (they don't have [cal] prefix in the value part)
    if (text.contains("progress <")) {
        _handleProgress(text);
        return;
    }

    appendStatusLog(text);
    qCDebug(PX4SensorCalibrationStateMachineLog) << text;

    if (_unknownFirmwareVersion) {
        // We don't know how to do visual cal with this firmware version
        return;
    }

    // All calibration messages start with [cal]
    QString calPrefix("[cal] ");
    if (!text.startsWith(calPrefix)) {
        return;
    }
    QString calText = text.mid(calPrefix.length());

    // Check for calibration started
    QString calStartPrefix("calibration started: ");
    if (calText.startsWith(calStartPrefix)) {
        _handleCalibrationStarted(calText.mid(calStartPrefix.length()));
        return;
    }

    // Route to orientation calibration machine if active
    if (_orientationCalMachine->isCalibrating()) {
        if (_orientationCalMachine->handleCalibrationMessage(calText)) {
            return;
        }
    }

    // Handle simple calibration completion
    if (calText.startsWith("calibration done:")) {
        _stopCalibration(true);
        return;
    }

    if (calText.startsWith("calibration cancelled")) {
        _stopCalibration(false);
        return;
    }

    if (calText.startsWith("calibration failed")) {
        _stopCalibration(false);
        return;
    }
}

void PX4SensorCalibrationStateMachine::_startLogCalibration()
{
    _unknownFirmwareVersion = false;
    hideAllCalAreas();

    connect(_px4Controller->_vehicle, &Vehicle::textMessageReceived, _px4Controller, &SensorsComponentController::_handleUASTextMessage);

    _px4Controller->_cancelButton->setEnabled(false);
}

void PX4SensorCalibrationStateMachine::_startVisualCalibration()
{
    setButtonsEnabled(false);
    _px4Controller->_cancelButton->setEnabled(true);

    _px4Controller->_resetInternalState();
    _px4Controller->setProgress(0);
}

void PX4SensorCalibrationStateMachine::_stopCalibration(bool success)
{
    qCDebug(PX4SensorCalibrationStateMachineLog) << "Stopping calibration, success:" << success;

    disconnect(_px4Controller->_vehicle, &Vehicle::textMessageReceived, _px4Controller, &SensorsComponentController::_handleUASTextMessage);

    setButtonsEnabled(true);
    _px4Controller->_cancelButton->setEnabled(false);

    if (success) {
        _px4Controller->_resetInternalState();
        _px4Controller->setProgress(1);
    } else {
        _px4Controller->setProgress(0);
    }

    _px4Controller->_refreshParams();

    if (success) {
        _px4Controller->setOrientationHelpText(tr("Calibration complete"));
        if (_calType != QGCMAVLink::CalibrationPX4Airspeed && _calType != QGCMAVLink::CalibrationLevel) {
            emit _px4Controller->resetStatusTextArea();
        }
        if (_calType == QGCMAVLink::CalibrationMag) {
            emit _px4Controller->magCalComplete();
        }
    } else {
        hideAllCalAreas();
        qgcApp()->showAppMessage(tr("Calibration failed. Calibration log will be displayed."));
    }

    QGCMAVLink::CalibrationType completedType = _calType;
    _calType = QGCMAVLink::CalibrationNone;

    postEvent("complete");
    emit calibrationComplete(completedType, success);
}

void PX4SensorCalibrationStateMachine::_handleCalibrationStarted(const QString& text)
{
    // Split version number and cal type
    QStringList parts = text.split(" ");
    if (parts.count() != 2 || parts[0].toInt() != _supportedFirmwareCalVersion) {
        _unknownFirmwareVersion = true;
        QString msg = tr("Unsupported calibration firmware version, using log");
        appendStatusLog(msg);
        qCDebug(PX4SensorCalibrationStateMachineLog) << msg;
        return;
    }

    _startVisualCalibration();

    QString calType = parts[1];
    qCDebug(PX4SensorCalibrationStateMachineLog) << "Calibration started for type:" << calType;

    if (calType == "accel" || calType == "mag" || calType == "gyro") {
        PX4OrientationCalibrationMachine::CalibrationType orientationType;
        int visibleSides = 0x3F;  // All sides visible by default

        if (calType == "accel") {
            orientationType = PX4OrientationCalibrationMachine::CalibrationType::Accel;
            _px4Controller->_accelCalInProgress = true;
        } else if (calType == "mag") {
            orientationType = PX4OrientationCalibrationMachine::CalibrationType::Mag;
            _px4Controller->_magCalInProgress = true;

            // Read the requested calibration directions from the system
            if (_px4Controller->_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "CAL_MAG_SIDES")) {
                visibleSides = _px4Controller->_vehicle->parameterManager()->getParameter(
                    ParameterManager::defaultComponentId, "CAL_MAG_SIDES")->rawValue().toInt();
            }
        } else {
            orientationType = PX4OrientationCalibrationMachine::CalibrationType::Gyro;
            _px4Controller->_gyroCalInProgress = true;
            visibleSides = (1 << 5);  // Only down side for gyro
        }

        setShowOrientationCalArea(true);

        if (!isRunning()) {
            start();
        }
        postEvent("start_orientation");

        _orientationCalMachine->startCalibration(orientationType, visibleSides);

    } else if (calType == "airspeed") {
        _px4Controller->_airspeedCalInProgress = true;
    } else if (calType == "level") {
        _px4Controller->_levelCalInProgress = true;
    }
}

void PX4SensorCalibrationStateMachine::_handleProgress(const QString& text)
{
    // Parse progress value from "progress <N>"
    // The text may have HTML entities that need to be decoded
    QString decodedText = text;
    decodedText.replace("&lt;", "<");
    decodedText.replace("&gt;", ">");

    if (decodedText.contains("progress <")) {
        QString percent = decodedText.split("<").last().split(">").first();
        bool ok;
        int p = percent.toInt(&ok);
        if (ok) {
            _px4Controller->setProgress(static_cast<float>(p) / 100.0f);
        }
    }
}
