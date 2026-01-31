#include "APMSensorCalibrationStateMachine.h"
#include "AccelCalibrationMachine.h"
#include "CompassCalibrationMachine.h"
#include "APMSensorsComponentController.h"
#include "MAVLinkProtocol.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QTimer>

QGC_LOGGING_CATEGORY(APMSensorCalibrationStateMachineLog, "APMSensorCalibration.StateMachine")

APMSensorCalibrationStateMachine::APMSensorCalibrationStateMachine(APMSensorsComponentController* controller, QObject* parent)
    : SensorCalibrationStateMachineBase("APMSensorCalibration", controller, parent)
    , _apmController(controller)
{
    // Create sub-machines
    _accelCalMachine = new AccelCalibrationMachine(controller, this);
    _compassCalMachine = new CompassCalibrationMachine(controller, this);

    // Connect sub-machine signals
    connect(_accelCalMachine, &AccelCalibrationMachine::calibrationComplete, this, [this](bool success) {
        qCDebug(APMSensorCalibrationStateMachineLog) << "Accel calibration complete, success:" << success;
        _stopCalibration(success);
    });

    connect(_compassCalMachine, &CompassCalibrationMachine::calibrationComplete, this, [this](bool success) {
        qCDebug(APMSensorCalibrationStateMachineLog) << "Compass calibration complete, success:" << success;
        _stopCalibration(success, true); // showLog for compass cal
    });

    _buildStateMachine();
    _setupTransitions();
}

void APMSensorCalibrationStateMachine::_buildStateMachine()
{
    // Build common base states (idle, simpleCal, cancelling)
    buildBaseStates();

    // Create APM-specific states
    _accelCalState = new QGCState("AccelCalibration", this);
    _compassCalState = new QGCState("CompassCalibration", this);
    _compassMotState = new QGCState("CompassMotCalibration", this);

    // Configure APM-specific state entry callbacks
    connect(_accelCalState, &QAbstractState::entered, this, [this]() {
        qCDebug(APMSensorCalibrationStateMachineLog) << "Entered AccelCalibration state";
        _accelCalMachine->startCalibration();
    });

    connect(_compassCalState, &QAbstractState::entered, this, [this]() {
        qCDebug(APMSensorCalibrationStateMachineLog) << "Entered CompassCalibration state";
        _compassCalMachine->startCalibration();
    });

    connect(_compassMotState, &QAbstractState::entered, this, [this]() {
        qCDebug(APMSensorCalibrationStateMachineLog) << "Entered CompassMotCalibration state";
        appendStatusLog(tr("Raise the throttle slowly to between 50% ~ 75% (the props will spin!) for 5 ~ 10 seconds."));
        appendStatusLog(tr("Quickly bring the throttle back down to zero"));
        appendStatusLog(tr("Press the Next button to complete the calibration"));
    });

    connect(_cancellingState, &QAbstractState::entered, this, [this]() {
        qCDebug(APMSensorCalibrationStateMachineLog) << "Entered Cancelling state";
        _apmController->setWaitingForCancel(true);
    });

    connect(_cancellingState, &QAbstractState::exited, this, [this]() {
        _apmController->setWaitingForCancel(false);
    });
}

void APMSensorCalibrationStateMachine::_setupTransitions()
{
    // Idle -> various calibrations
    _idleState->addTransition(new MachineEventTransition("start_accel", _accelCalState));
    _idleState->addTransition(new MachineEventTransition("start_compass", _compassCalState));
    _idleState->addTransition(new MachineEventTransition("start_simple", _simpleCalState));
    _idleState->addTransition(new MachineEventTransition("start_compassmot", _compassMotState));

    // All states back to Idle on complete/cancel
    _accelCalState->addTransition(new MachineEventTransition("complete", _idleState));
    _compassCalState->addTransition(new MachineEventTransition("complete", _idleState));
    _simpleCalState->addTransition(new MachineEventTransition("complete", _idleState));
    _compassMotState->addTransition(new MachineEventTransition("complete", _idleState));
    _cancellingState->addTransition(new MachineEventTransition("complete", _idleState));

    // Cancel transitions
    _accelCalState->addTransition(new MachineEventTransition("cancel", _cancellingState));
    _simpleCalState->addTransition(new MachineEventTransition("cancel", _cancellingState));
    _compassMotState->addTransition(new MachineEventTransition("cancel", _cancellingState));
    // Compass has its own cancel handling
    _compassCalState->addTransition(new MachineEventTransition("cancel", _idleState));
}

void APMSensorCalibrationStateMachine::calibrateCompass()
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Starting compass calibration";

    _calType = QGCMAVLink::CalibrationMag;

    // Connect for support check - we send a cancel command to check if onboard cal is supported
    connect(_apmController->_vehicle, &Vehicle::mavCommandResult, _apmController, &APMSensorsComponentController::_mavCommandResult);
    _apmController->_vehicle->sendMavCommand(_apmController->_vehicle->defaultComponentId(), MAV_CMD_DO_CANCEL_MAG_CAL, false /* showError */);
}

void APMSensorCalibrationStateMachine::calibrateAccel(bool simple)
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Starting accel calibration, simple:" << simple;

    if (simple) {
        _calType = QGCMAVLink::CalibrationAPMAccelSimple;
        _startLogCalibration();
        _apmController->_vehicle->startCalibration(_calType);

        if (!isRunning()) {
            start();
        }
        postEvent("start_simple");
    } else {
        _calType = QGCMAVLink::CalibrationAccel;
        _apmController->_vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
        _startVisualCalibration();
        _apmController->_vehicle->startCalibration(_calType);

        if (!isRunning()) {
            start();
        }
        postEvent("start_accel");
    }

    emit calibrationStarted(_calType);
}

void APMSensorCalibrationStateMachine::calibrateCompassNorth(float lat, float lon, int mask)
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Starting compass north calibration";

    _calType = QGCMAVLink::CalibrationMag;
    _startLogCalibration();

    connect(_apmController->_vehicle, &Vehicle::mavCommandResult, _apmController, &APMSensorsComponentController::_mavCommandResult);
    _apmController->_vehicle->sendMavCommand(
        _apmController->_vehicle->defaultComponentId(),
        MAV_CMD_FIXED_MAG_CAL_YAW,
        true /* showError */,
        0 /* north */,
        mask,
        lat,
        lon
    );

    emit calibrationStarted(_calType);
}

void APMSensorCalibrationStateMachine::calibrateGyro()
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Starting gyro calibration";

    _calType = QGCMAVLink::CalibrationGyro;
    _apmController->_vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startLogCalibration();
    appendStatusLog(tr("Requesting gyro calibration..."));
    _apmController->_vehicle->startCalibration(_calType);

    if (!isRunning()) {
        start();
    }
    postEvent("start_simple");

    emit calibrationStarted(_calType);
}

void APMSensorCalibrationStateMachine::calibrateMotorInterference()
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Starting motor interference calibration";

    _calType = QGCMAVLink::CalibrationAPMCompassMot;
    _apmController->_vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startLogCalibration();
    _apmController->_vehicle->startCalibration(_calType);

    if (!isRunning()) {
        start();
    }
    postEvent("start_compassmot");

    emit calibrationStarted(_calType);
}

void APMSensorCalibrationStateMachine::levelHorizon()
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Starting level horizon calibration";

    _calType = QGCMAVLink::CalibrationLevel;
    _apmController->_vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startLogCalibration();
    appendStatusLog(tr("Hold the vehicle in its level flight position."));
    _apmController->_vehicle->startCalibration(_calType);

    if (!isRunning()) {
        start();
    }
    postEvent("start_simple");

    emit calibrationStarted(_calType);
}

void APMSensorCalibrationStateMachine::calibratePressure()
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Starting pressure calibration";

    _calType = QGCMAVLink::CalibrationAPMPressureAirspeed;
    _apmController->_vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startLogCalibration();
    appendStatusLog(tr("Requesting pressure calibration..."));
    _apmController->_vehicle->startCalibration(_calType);

    if (!isRunning()) {
        start();
    }
    postEvent("start_simple");

    emit calibrationStarted(_calType);
}

void APMSensorCalibrationStateMachine::cancelCalibration()
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Cancelling calibration";

    _apmController->_cancelButton->setEnabled(false);

    if (_calType == QGCMAVLink::CalibrationMag) {
        _apmController->_vehicle->sendMavCommand(_apmController->_vehicle->defaultComponentId(), MAV_CMD_DO_CANCEL_MAG_CAL, true /* showError */);
        _compassCalMachine->cancelCalibration();
        _stopCalibration(false);
    } else if (_calType == QGCMAVLink::CalibrationAccel) {
        _accelCalMachine->cancelCalibration();
        postEvent("cancel");
        _apmController->_vehicle->stopCalibration(true /* showError */);
    } else {
        postEvent("cancel");
        _apmController->_vehicle->stopCalibration(true /* showError */);
    }
}

void APMSensorCalibrationStateMachine::nextClicked()
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Next button clicked";

    SharedLinkInterfacePtr sharedLink = _apmController->_vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        mavlink_message_t msg{};

        (void) mavlink_msg_command_ack_pack_chan(
            MAVLinkProtocol::instance()->getSystemId(),
            MAVLinkProtocol::getComponentId(),
            sharedLink->mavlinkChannel(),
            &msg,
            0,    // command
            1,    // result
            0,    // progress
            0,    // result_param2
            0,    // target_system
            0     // target_component
        );

        (void) _apmController->_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);

        if (_calType == QGCMAVLink::CalibrationAPMCompassMot) {
            _stopCalibration(true);
        }
    }

    // Forward to sub-machines
    if (_calType == QGCMAVLink::CalibrationAccel) {
        _accelCalMachine->nextButtonPressed();
    }
}

void APMSensorCalibrationStateMachine::handleMavlinkMessage(LinkInterface* link, const mavlink_message_t& message)
{
    Q_UNUSED(link);

    if (message.sysid != _apmController->_vehicle->id()) {
        return;
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_COMMAND_ACK:
        _handleCommandAck(message);
        break;
    case MAVLINK_MSG_ID_MAG_CAL_PROGRESS:
        _handleMagCalProgress(message);
        break;
    case MAVLINK_MSG_ID_MAG_CAL_REPORT:
        _handleMagCalReport(message);
        break;
    case MAVLINK_MSG_ID_COMMAND_LONG:
        _handleCommandLong(message);
        break;
    }
}

void APMSensorCalibrationStateMachine::handleMavCommandResult(int vehicleId, int component, int command, int result, int failureCode)
{
    Q_UNUSED(component);
    Q_UNUSED(failureCode);

    if (_apmController->_vehicle->id() != vehicleId) {
        return;
    }

    switch (command) {
    case MAV_CMD_DO_CANCEL_MAG_CAL:
        disconnect(_apmController->_vehicle, &Vehicle::mavCommandResult, _apmController, &APMSensorsComponentController::_mavCommandResult);
        if (result == MAV_RESULT_ACCEPTED) {
            // Onboard mag cal is supported
            qCDebug(APMSensorCalibrationStateMachineLog) << "Onboard mag cal supported";
            _startLogCalibration();
            _compassCalMachine->handleSupportCheckResult(true);

            if (!isRunning()) {
                start();
            }
            postEvent("start_compass");
            emit calibrationStarted(_calType);
        }
        break;
    case MAV_CMD_DO_START_MAG_CAL:
        if (result != MAV_RESULT_ACCEPTED) {
            // Trigger submachine's cleanup which will restore fitness
            _compassCalMachine->cancelCalibration();
        }
        break;
    case MAV_CMD_FIXED_MAG_CAL_YAW:
        if (result == MAV_RESULT_ACCEPTED) {
            appendStatusLog(tr("Successfully completed"));
            _stopCalibration(true, true);
        } else {
            appendStatusLog(tr("Failed"));
            _stopCalibration(false);
        }
        break;
    }
}

void APMSensorCalibrationStateMachine::_startLogCalibration()
{
    hideAllCalAreas();

    connect(_apmController->_vehicle, &Vehicle::textMessageReceived, _apmController, &APMSensorsComponentController::_handleTextMessage);

    emit _apmController->setAllCalButtonsEnabled(false);
    if ((_calType == QGCMAVLink::CalibrationAccel) || (_calType == QGCMAVLink::CalibrationAPMCompassMot)) {
        _apmController->_nextButton->setEnabled(true);
    }
    _apmController->_cancelButton->setEnabled(_calType == QGCMAVLink::CalibrationMag);

    connect(MAVLinkProtocol::instance(), &MAVLinkProtocol::messageReceived, this,
            [this](LinkInterface* link, const mavlink_message_t& msg) {
        handleMavlinkMessage(link, msg);
    });
}

void APMSensorCalibrationStateMachine::_startVisualCalibration()
{
    emit _apmController->setAllCalButtonsEnabled(false);
    _apmController->_cancelButton->setEnabled(true);
    _apmController->_nextButton->setEnabled(false);

    _apmController->_resetInternalState();
    _apmController->setProgress(0);

    _apmController->setOrientationHelpText(tr("Hold still in the current orientation and press Next when ready"));
    setShowOrientationCalArea(true);

    connect(MAVLinkProtocol::instance(), &MAVLinkProtocol::messageReceived, this,
            [this](LinkInterface* link, const mavlink_message_t& msg) {
        handleMavlinkMessage(link, msg);
    });
}

void APMSensorCalibrationStateMachine::_stopCalibration(bool success, bool showLog)
{
    qCDebug(APMSensorCalibrationStateMachineLog) << "Stopping calibration, success:" << success << "showLog:" << showLog;

    disconnect(MAVLinkProtocol::instance(), &MAVLinkProtocol::messageReceived, nullptr, nullptr);
    _apmController->_vehicle->vehicleLinkManager()->setCommunicationLostEnabled(true);

    disconnect(_apmController->_vehicle, &Vehicle::textMessageReceived, _apmController, &APMSensorsComponentController::_handleTextMessage);

    emit _apmController->setAllCalButtonsEnabled(true);
    _apmController->_nextButton->setEnabled(false);
    _apmController->_cancelButton->setEnabled(false);

    // Note: Compass fitness restoration is handled by CompassCalibrationMachine

    if (success) {
        _apmController->_resetInternalState();
        _apmController->setProgress(1);
        if (_apmController->parameterExists(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_LEARN"))) {
            _apmController->getParameterFact(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_LEARN"))->setRawValue(0);
        }
    } else {
        _apmController->setProgress(0);
    }

    _apmController->_refreshParams();

    if (success) {
        _apmController->setOrientationHelpText(tr("Calibration complete"));
        emit _apmController->resetStatusTextArea();
    } else if (!showLog) {
        emit _apmController->resetStatusTextArea();
        hideAllCalAreas();
    }

    QGCMAVLink::CalibrationType completedType = _calType;
    _calType = QGCMAVLink::CalibrationNone;

    postEvent("complete");
    emit _apmController->calibrationComplete(completedType);
    emit calibrationComplete(completedType, success);
}

void APMSensorCalibrationStateMachine::_handleCommandAck(const mavlink_message_t& message)
{
    if ((_calType == QGCMAVLink::CalibrationLevel) ||
        (_calType == QGCMAVLink::CalibrationGyro) ||
        (_calType == QGCMAVLink::CalibrationAPMPressureAirspeed) ||
        (_calType == QGCMAVLink::CalibrationAPMAccelSimple)) {

        mavlink_command_ack_t commandAck{};
        mavlink_msg_command_ack_decode(&message, &commandAck);

        if (commandAck.command == MAV_CMD_PREFLIGHT_CALIBRATION) {
            switch (commandAck.result) {
            case MAV_RESULT_IN_PROGRESS:
                appendStatusLog(tr("In progress"));
                break;
            case MAV_RESULT_ACCEPTED:
                appendStatusLog(tr("Successfully completed"));
                _stopCalibration(true, true);
                break;
            default:
                appendStatusLog(tr("Failed"));
                _stopCalibration(false);
                break;
            }
        }
    }
}

void APMSensorCalibrationStateMachine::_handleMagCalProgress(const mavlink_message_t& message)
{
    if (_calType != QGCMAVLink::CalibrationMag) {
        return;
    }

    mavlink_mag_cal_progress_t magCalProgress{};
    mavlink_msg_mag_cal_progress_decode(&message, &magCalProgress);

    _compassCalMachine->handleMagCalProgress(magCalProgress.compass_id, magCalProgress.cal_mask, magCalProgress.completion_pct);
}

void APMSensorCalibrationStateMachine::_handleMagCalReport(const mavlink_message_t& message)
{
    if (_calType != QGCMAVLink::CalibrationMag) {
        return;
    }

    mavlink_mag_cal_report_t magCalReport{};
    mavlink_msg_mag_cal_report_decode(&message, &magCalReport);

    _compassCalMachine->handleMagCalReport(magCalReport.compass_id, magCalReport.cal_status, magCalReport.fitness);
}

void APMSensorCalibrationStateMachine::_handleCommandLong(const mavlink_message_t& message)
{
    if (_calType != QGCMAVLink::CalibrationAccel) {
        return;
    }

    mavlink_command_long_t commandLong{};
    mavlink_msg_command_long_decode(&message, &commandLong);

    if (commandLong.command == MAV_CMD_ACCELCAL_VEHICLE_POS) {
        auto position = static_cast<AccelCalibrationMachine::Position>(static_cast<int>(commandLong.param1));
        _accelCalMachine->handlePositionCommand(position);
    }
}
