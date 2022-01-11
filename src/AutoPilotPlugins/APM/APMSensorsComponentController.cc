/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "APMSensorsComponentController.h"
#include "QGCMAVLink.h"
#include "UAS.h"
#include "QGCApplication.h"
#include "APMAutoPilotPlugin.h"
#include "ParameterManager.h"

#include <QVariant>
#include <QQmlProperty>

QGC_LOGGING_CATEGORY(APMSensorsComponentControllerLog, "APMSensorsComponentControllerLog")
QGC_LOGGING_CATEGORY(APMSensorsComponentControllerVerboseLog, "APMSensorsComponentControllerVerboseLog")

const char* APMSensorsComponentController::_compassCalFitnessParam = "COMPASS_CAL_FIT";

APMSensorsComponentController::APMSensorsComponentController(void)
    : _sensorsComponent(nullptr)
    , _statusLog(nullptr)
    , _progressBar(nullptr)
    , _nextButton(nullptr)
    , _cancelButton(nullptr)
    , _showOrientationCalArea(false)
    , _calTypeInProgress(CalTypeNone)
    , _orientationCalDownSideDone(false)
    , _orientationCalUpsideDownSideDone(false)
    , _orientationCalLeftSideDone(false)
    , _orientationCalRightSideDone(false)
    , _orientationCalNoseDownSideDone(false)
    , _orientationCalTailDownSideDone(false)
    , _orientationCalDownSideVisible(false)
    , _orientationCalUpsideDownSideVisible(false)
    , _orientationCalLeftSideVisible(false)
    , _orientationCalRightSideVisible(false)
    , _orientationCalNoseDownSideVisible(false)
    , _orientationCalTailDownSideVisible(false)
    , _orientationCalDownSideInProgress(false)
    , _orientationCalUpsideDownSideInProgress(false)
    , _orientationCalLeftSideInProgress(false)
    , _orientationCalRightSideInProgress(false)
    , _orientationCalNoseDownSideInProgress(false)
    , _orientationCalTailDownSideInProgress(false)
    , _orientationCalDownSideRotate(false)
    , _orientationCalUpsideDownSideRotate(false)
    , _orientationCalLeftSideRotate(false)
    , _orientationCalRightSideRotate(false)
    , _orientationCalNoseDownSideRotate(false)
    , _orientationCalTailDownSideRotate(false)
    , _waitingForCancel(false)
    , _restoreCompassCalFitness(false)
{
    _compassCal.setVehicle(_vehicle);
    connect(&_compassCal, &APMCompassCal::vehicleTextMessage, this, &APMSensorsComponentController::_handleUASTextMessage);

    APMAutoPilotPlugin * apmPlugin = qobject_cast<APMAutoPilotPlugin*>(_vehicle->autopilotPlugin());

    // Find the sensors component
    foreach (const QVariant& varVehicleComponent, apmPlugin->vehicleComponents()) {
        _sensorsComponent = qobject_cast<APMSensorsComponent*>(varVehicleComponent.value<VehicleComponent*>());
        if (_sensorsComponent) {
            break;
        }
    }

    if (_sensorsComponent) {
        connect(_sensorsComponent, &VehicleComponent::setupCompleteChanged, this, &APMSensorsComponentController::setupNeededChanged);
    } else {
        qWarning() << "Sensors component is missing";
    }

}

APMSensorsComponentController::~APMSensorsComponentController()
{
    _restorePreviousCompassCalFitness();
}

/// Appends the specified text to the status log area in the ui
void APMSensorsComponentController::_appendStatusLog(const QString& text)
{
    Q_ASSERT(_statusLog);
    
    QVariant returnedValue;
    QVariant varText = text;
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}

void APMSensorsComponentController::_startLogCalibration(void)
{
    _hideAllCalAreas();
    
    connect(_vehicle, &Vehicle::textMessageReceived, this, &APMSensorsComponentController::_handleUASTextMessage);
    
    emit setAllCalButtonsEnabled(false);
    if (_calTypeInProgress == CalTypeAccel || _calTypeInProgress == CalTypeCompassMot) {
        _nextButton->setEnabled(true);
    }
    _cancelButton->setEnabled(_calTypeInProgress == CalTypeOnboardCompass);

    connect(qgcApp()->toolbox()->mavlinkProtocol(), &MAVLinkProtocol::messageReceived, this, &APMSensorsComponentController::_mavlinkMessageReceived);
}

void APMSensorsComponentController::_startVisualCalibration(void)
{
    emit setAllCalButtonsEnabled(false);
    _cancelButton->setEnabled(true);
    _nextButton->setEnabled(false);

    _resetInternalState();
    
    _progressBar->setProperty("value", 0);

    connect(qgcApp()->toolbox()->mavlinkProtocol(), &MAVLinkProtocol::messageReceived, this, &APMSensorsComponentController::_mavlinkMessageReceived);
}

void APMSensorsComponentController::_resetInternalState(void)
{
    _orientationCalDownSideDone = true;
    _orientationCalUpsideDownSideDone = true;
    _orientationCalLeftSideDone = true;
    _orientationCalRightSideDone = true;
    _orientationCalTailDownSideDone = true;
    _orientationCalNoseDownSideDone = true;
    _orientationCalDownSideInProgress = false;
    _orientationCalUpsideDownSideInProgress = false;
    _orientationCalLeftSideInProgress = false;
    _orientationCalRightSideInProgress = false;
    _orientationCalNoseDownSideInProgress = false;
    _orientationCalTailDownSideInProgress = false;
    _orientationCalDownSideRotate = false;
    _orientationCalUpsideDownSideRotate = false;
    _orientationCalLeftSideRotate = false;
    _orientationCalRightSideRotate = false;
    _orientationCalNoseDownSideRotate = false;
    _orientationCalTailDownSideRotate = false;

    emit orientationCalSidesRotateChanged();
    emit orientationCalSidesDoneChanged();
    emit orientationCalSidesInProgressChanged();
}

void APMSensorsComponentController::_stopCalibration(APMSensorsComponentController::StopCalibrationCode code)
{
    disconnect(qgcApp()->toolbox()->mavlinkProtocol(), &MAVLinkProtocol::messageReceived, this, &APMSensorsComponentController::_mavlinkMessageReceived);
    _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(true);

    disconnect(_vehicle, &Vehicle::textMessageReceived, this, &APMSensorsComponentController::_handleUASTextMessage);
    
    emit setAllCalButtonsEnabled(true);
    _nextButton->setEnabled(false);
    _cancelButton->setEnabled(false);

    if (_calTypeInProgress == CalTypeOnboardCompass) {
        _restorePreviousCompassCalFitness();
    }

    if (code == StopCalibrationSuccess) {
        _resetInternalState();
        _progressBar->setProperty("value", 1);
        if (parameterExists(FactSystem::defaultComponentId, QStringLiteral("COMPASS_LEARN"))) {
            getParameterFact(FactSystem::defaultComponentId, QStringLiteral("COMPASS_LEARN"))->setRawValue(0);
        }
    } else {
        _progressBar->setProperty("value", 0);
    }
    
    _waitingForCancel = false;
    emit waitingForCancelChanged();

    _refreshParams();
    
    switch (code) {
    case StopCalibrationSuccess:
        _orientationCalAreaHelpText->setProperty("text", tr("Calibration complete"));
        emit resetStatusTextArea();
        emit calibrationComplete(_calTypeInProgress);
        break;

    case StopCalibrationSuccessShowLog:
        emit calibrationComplete(_calTypeInProgress);
        break;

    case StopCalibrationCancelled:
        emit resetStatusTextArea();
        _hideAllCalAreas();
        break;

    default:
        // Assume failed
        _hideAllCalAreas();
        qgcApp()->showAppMessage(tr("Calibration failed. Calibration log will be displayed."));
        break;
    }
    
    _calTypeInProgress = CalTypeNone;
}

void APMSensorsComponentController::_mavCommandResult(int vehicleId, int component, int command, int result, bool noReponseFromVehicle)
{
    Q_UNUSED(component);
    Q_UNUSED(noReponseFromVehicle);

    if (_vehicle->id() != vehicleId) {
        return;
    }

    if (command == MAV_CMD_DO_CANCEL_MAG_CAL) {
        disconnect(_vehicle, &Vehicle::mavCommandResult, this, &APMSensorsComponentController::_mavCommandResult);
        if (result == MAV_RESULT_ACCEPTED) {
            // Onboard mag cal is supported
            _calTypeInProgress = CalTypeOnboardCompass;
            _rgCompassCalProgress[0] = 0;
            _rgCompassCalProgress[1] = 0;
            _rgCompassCalProgress[2] = 0;
            _rgCompassCalComplete[0] = false;
            _rgCompassCalComplete[1] = false;
            _rgCompassCalComplete[2] = false;

            _startLogCalibration();
            uint8_t compassBits = 0;
            if (getParameterFact(FactSystem::defaultComponentId, QStringLiteral("COMPASS_DEV_ID"))->rawValue().toInt() > 0 &&
                    getParameterFact(FactSystem::defaultComponentId, QStringLiteral("COMPASS_USE"))->rawValue().toBool()) {
                compassBits |= 1 << 0;
                qCDebug(APMSensorsComponentControllerLog) << "Performing onboard compass cal for compass 1";
            } else {
                _rgCompassCalComplete[0] = true;
                _rgCompassCalSucceeded[0] = true;
                _rgCompassCalFitness[0] = 0;
            }
            if (getParameterFact(FactSystem::defaultComponentId, QStringLiteral("COMPASS_DEV_ID2"))->rawValue().toInt() > 0 &&
                    getParameterFact(FactSystem::defaultComponentId, QStringLiteral("COMPASS_USE2"))->rawValue().toBool()) {
                compassBits |= 1 << 1;
                qCDebug(APMSensorsComponentControllerLog) << "Performing onboard compass cal for compass 2";
            } else {
                _rgCompassCalComplete[1] = true;
                _rgCompassCalSucceeded[1] = true;
                _rgCompassCalFitness[1] = 0;
            }
            if (getParameterFact(FactSystem::defaultComponentId, QStringLiteral("COMPASS_DEV_ID3"))->rawValue().toInt() > 0 &&
                    getParameterFact(FactSystem::defaultComponentId, QStringLiteral("COMPASS_USE3"))->rawValue().toBool()) {
                compassBits |= 1 << 2;
                qCDebug(APMSensorsComponentControllerLog) << "Performing onboard compass cal for compass 3";
            } else {
                _rgCompassCalComplete[2] = true;
                _rgCompassCalSucceeded[2] = true;
                _rgCompassCalFitness[2] = 0;
            }

            // We bump up the fitness value so calibration will always succeed
            Fact* compassCalFitness = getParameterFact(FactSystem::defaultComponentId, _compassCalFitnessParam);
            _restoreCompassCalFitness = true;
            _previousCompassCalFitness = compassCalFitness->rawValue().toFloat();
            getParameterFact(FactSystem::defaultComponentId, _compassCalFitnessParam)->setRawValue(100.0);

            _appendStatusLog(tr("Rotate the vehicle randomly around all axes until the progress bar fills all the way to the right ."));
            _vehicle->sendMavCommand(_vehicle->defaultComponentId(),
                                     MAV_CMD_DO_START_MAG_CAL,
                                     true,          // showError
                                     compassBits,   // which compass(es) to calibrate
                                     0,             // no retry on failure
                                     1,             // save values after complete
                                     0,             // no delayed start
                                     0);            // no auto-reboot

        } else {
            // Onboard mag cal is not supported
            _compassCal.startCalibration();
        }
    } else if (command == MAV_CMD_DO_START_MAG_CAL && result != MAV_RESULT_ACCEPTED) {
        _restorePreviousCompassCalFitness();
    } else if (command == MAV_CMD_FIXED_MAG_CAL_YAW) {
        if (result == MAV_RESULT_ACCEPTED) {
            _appendStatusLog(tr("Successfully completed"));
            _stopCalibration(StopCalibrationSuccessShowLog);
        } else {
            _appendStatusLog(tr("Failed"));
            _stopCalibration(StopCalibrationFailed);
        }
    }
}

void APMSensorsComponentController::calibrateCompass(void)
{
    // First we need to determine if the vehicle support onboard compass cal. There isn't an easy way to
    // do this. A hack is to send the mag cancel command and see if it is accepted.
    connect(_vehicle, &Vehicle::mavCommandResult, this, &APMSensorsComponentController::_mavCommandResult);
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_DO_CANCEL_MAG_CAL, false /* showError */);

    // Now we wait for the result to come back
}

void APMSensorsComponentController::calibrateCompassNorth(float lat, float lon, int mask)
{
    _startLogCalibration();
    connect(_vehicle, &Vehicle::mavCommandResult, this, &APMSensorsComponentController::_mavCommandResult);
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_FIXED_MAG_CAL_YAW, true /* showError */, 0 /* north*/, mask, lat, lon);
}

void APMSensorsComponentController::calibrateAccel(bool doSimpleAccelCal)
{

    _calTypeInProgress = CalTypeAccel;
    if (doSimpleAccelCal) {
        _startLogCalibration();
        _calTypeInProgress = CalTypeAccelFast;
        _vehicle->startCalibration(Vehicle::CalibrationAPMAccelSimple);
        return;
    }
    _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startVisualCalibration();
    _cancelButton->setEnabled(false);
    _orientationCalAreaHelpText->setProperty("text", tr("Hold still in the current orientation and press Next when ready"));

    // Reset all progress indication
    _orientationCalDownSideDone = false;
    _orientationCalUpsideDownSideDone = false;
    _orientationCalLeftSideDone = false;
    _orientationCalRightSideDone = false;
    _orientationCalTailDownSideDone = false;
    _orientationCalNoseDownSideDone = false;
    _orientationCalDownSideInProgress = false;
    _orientationCalUpsideDownSideInProgress = false;
    _orientationCalLeftSideInProgress = false;
    _orientationCalRightSideInProgress = false;
    _orientationCalNoseDownSideInProgress = false;
    _orientationCalTailDownSideInProgress = false;

    // Reset all visibility
    _orientationCalDownSideVisible = false;
    _orientationCalUpsideDownSideVisible = false;
    _orientationCalLeftSideVisible = false;
    _orientationCalRightSideVisible = false;
    _orientationCalTailDownSideVisible = false;
    _orientationCalNoseDownSideVisible = false;

    _calTypeInProgress = CalTypeAccel;
    _orientationCalDownSideVisible = true;
    _orientationCalUpsideDownSideVisible = true;
    _orientationCalLeftSideVisible = true;
    _orientationCalRightSideVisible = true;
    _orientationCalTailDownSideVisible = true;
    _orientationCalNoseDownSideVisible = true;

    emit orientationCalSidesDoneChanged();
    emit orientationCalSidesVisibleChanged();
    emit orientationCalSidesInProgressChanged();
    _updateAndEmitShowOrientationCalArea(true);

    _vehicle->startCalibration(Vehicle::CalibrationAccel);
}

void APMSensorsComponentController::calibrateMotorInterference(void)
{
    _calTypeInProgress = CalTypeCompassMot;
    _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startLogCalibration();
    _appendStatusLog(tr("Raise the throttle slowly to between 50% ~ 75% (the props will spin!) for 5 ~ 10 seconds."));
    _appendStatusLog(tr("Quickly bring the throttle back down to zero"));
    _appendStatusLog(tr("Press the Next button to complete the calibration"));
    _vehicle->startCalibration(Vehicle::CalibrationAPMCompassMot);
}

void APMSensorsComponentController::levelHorizon(void)
{
    _calTypeInProgress = CalTypeLevelHorizon;
    _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startLogCalibration();
    _appendStatusLog(tr("Hold the vehicle in its level flight position."));
    _vehicle->startCalibration(Vehicle::CalibrationLevel);
}

void APMSensorsComponentController::calibratePressure(void)
{
    _calTypeInProgress = CalTypePressure;
    _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startLogCalibration();
    _appendStatusLog(tr("Requesting pressure calibration..."));
    _vehicle->startCalibration(Vehicle::CalibrationAPMPressureAirspeed);
}

void APMSensorsComponentController::calibrateGyro(void)
{
    _calTypeInProgress = CalTypeGyro;
    _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _startLogCalibration();
    _appendStatusLog(tr("Requesting gyro calibration..."));
    _vehicle->startCalibration(Vehicle::CalibrationGyro);
}

void APMSensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    
    if (uasId != _vehicle->id()) {
        return;
    }

    QString originalMessageText = text;
    text = text.toLower();

    QStringList hidePrefixList = { QStringLiteral("prearm:"), QStringLiteral("ekf"), QStringLiteral("arm"), QStringLiteral("initialising") };
    for (const QString& hidePrefix: hidePrefixList) {
        if (text.startsWith(hidePrefix)) {
            return;
        }
    }

    _appendStatusLog(originalMessageText);
    qCDebug(APMSensorsComponentControllerLog) << originalMessageText << severity;
}

void APMSensorsComponentController::_refreshParams(void)
{
    QStringList fastRefreshList;
    
    fastRefreshList << QStringLiteral("COMPASS_OFS_X") << QStringLiteral("COMPASS_OFS_X") << QStringLiteral("COMPASS_OFS_X")
                    << QStringLiteral("INS_ACCOFFS_X") << QStringLiteral("INS_ACCOFFS_Y") << QStringLiteral("INS_ACCOFFS_Z");
    foreach (const QString &paramName, fastRefreshList) {
        _vehicle->parameterManager()->refreshParameter(FactSystem::defaultComponentId, paramName);
    }
    
    // Now ask for all to refresh
    _vehicle->parameterManager()->refreshParametersPrefix(FactSystem::defaultComponentId, QStringLiteral("COMPASS_"));
    _vehicle->parameterManager()->refreshParametersPrefix(FactSystem::defaultComponentId, QStringLiteral("INS_"));
}

void APMSensorsComponentController::_updateAndEmitShowOrientationCalArea(bool show)
{
    _showOrientationCalArea = show;
    emit showOrientationCalAreaChanged();
}

void APMSensorsComponentController::_hideAllCalAreas(void)
{
    _updateAndEmitShowOrientationCalArea(false);
}

void APMSensorsComponentController::cancelCalibration(void)
{
    _cancelButton->setEnabled(false);

    if (_calTypeInProgress == CalTypeOffboardCompass) {
        _waitingForCancel = true;
        emit waitingForCancelChanged();
        _compassCal.cancelCalibration();
    } else if (_calTypeInProgress == CalTypeOnboardCompass) {
        _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_DO_CANCEL_MAG_CAL, true /* showError */);
        _stopCalibration(StopCalibrationCancelled);
    } else {
        _waitingForCancel = true;
        emit waitingForCancelChanged();
        // The firmware doesn't always allow us to cancel calibration. The best we can do is wait
        // for it to timeout.
        _vehicle->stopCalibration(true /* showError */);
    }

}

void APMSensorsComponentController::nextClicked(void)
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        mavlink_message_t       msg;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        mavlink_msg_command_ack_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                          qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                          sharedLink->mavlinkChannel(),
                                          &msg,
                                          0,    // command
                                          1,    // result
                                          0,    // progress
                                          0,    // result_param2
                                          0,    // target_system
                                          0);   // target_component

        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);

        if (_calTypeInProgress == CalTypeCompassMot) {
            _stopCalibration(StopCalibrationSuccess);
        }
    }
}

bool APMSensorsComponentController::compassSetupNeeded(void) const
{
    return _sensorsComponent->compassSetupNeeded();
}

bool APMSensorsComponentController::accelSetupNeeded(void) const
{
    return _sensorsComponent->accelSetupNeeded();
}

bool APMSensorsComponentController::usingUDPLink(void)
{
    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (weakLink.expired()) {
        return false;
    } else {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();
        return sharedLink->linkConfiguration()->type() == LinkConfiguration::TypeUdp;
    }
}

void APMSensorsComponentController::_handleCommandAck(mavlink_message_t& message)
{
    if (_calTypeInProgress == CalTypeLevelHorizon || _calTypeInProgress == CalTypeGyro || _calTypeInProgress == CalTypePressure || _calTypeInProgress == CalTypeAccelFast) {
        mavlink_command_ack_t commandAck;
        mavlink_msg_command_ack_decode(&message, &commandAck);

        if (commandAck.command == MAV_CMD_PREFLIGHT_CALIBRATION) {
            switch (commandAck.result) {
            case MAV_RESULT_ACCEPTED:
                _appendStatusLog(tr("Successfully completed"));
                _stopCalibration(StopCalibrationSuccessShowLog);
                break;
            default:
                _appendStatusLog(tr("Failed"));
                _stopCalibration(StopCalibrationFailed);
                break;
            }
        }
    }
}

void APMSensorsComponentController::_handleMagCalProgress(mavlink_message_t& message)
{
    if (_calTypeInProgress == CalTypeOnboardCompass) {
        mavlink_mag_cal_progress_t magCalProgress;
        mavlink_msg_mag_cal_progress_decode(&message, &magCalProgress);

        qCDebug(APMSensorsComponentControllerVerboseLog) << "_handleMagCalProgress id:mask:pct"
                                                         << magCalProgress.compass_id << magCalProgress.cal_mask << magCalProgress.completion_pct;

        // How many compasses are we calibrating?
        int compassCalCount = 0;
        for (int i=0; i<3; i++) {
            if (magCalProgress.cal_mask & (1 << i)) {
                compassCalCount++;
            }
        }

        if (magCalProgress.compass_id < 3 && compassCalCount != 0) {
            // Each compass gets a portion of the overall progress
            _rgCompassCalProgress[magCalProgress.compass_id] = magCalProgress.completion_pct / compassCalCount;
        }

        if (_progressBar) {
            _progressBar->setProperty("value", (float)(_rgCompassCalProgress[0] + _rgCompassCalProgress[1] + _rgCompassCalProgress[2]) / 100.0);
        }
    }
}

void APMSensorsComponentController::_handleMagCalReport(mavlink_message_t& message)
{
    if (_calTypeInProgress == CalTypeOnboardCompass) {
        mavlink_mag_cal_report_t magCalReport;
        mavlink_msg_mag_cal_report_decode(&message, &magCalReport);

        qCDebug(APMSensorsComponentControllerVerboseLog) << "_handleMagCalReport id:mask:status:fitness"
                                                         << magCalReport.compass_id << magCalReport.cal_mask << magCalReport.cal_status << magCalReport.fitness;

        bool additionalCompassCompleted = false;
        if (magCalReport.compass_id < 3 && !_rgCompassCalComplete[magCalReport.compass_id]) {
            if (magCalReport.cal_status == MAG_CAL_SUCCESS) {
                _appendStatusLog(tr("Compass %1 calibration complete").arg(magCalReport.compass_id));
            } else {
                _appendStatusLog(tr("Compass %1 calibration below quality threshold").arg(magCalReport.compass_id));
            }
            _rgCompassCalComplete[magCalReport.compass_id] = true;
            _rgCompassCalSucceeded[magCalReport.compass_id] = magCalReport.cal_status == MAG_CAL_SUCCESS;
            _rgCompassCalFitness[magCalReport.compass_id] = magCalReport.fitness;
            additionalCompassCompleted = true;
        }

        if (_rgCompassCalComplete[0] && _rgCompassCalComplete[1] &&_rgCompassCalComplete[2]) {
            for (int i=0; i<3; i++) {
                qCDebug(APMSensorsComponentControllerLog) << QString("Onboard compass call report #%1: succeed:fitness %2:%3").arg(i).arg(_rgCompassCalSucceeded[i]).arg(_rgCompassCalFitness[i]);
            }
            emit compass1CalFitnessChanged(_rgCompassCalFitness[0]);
            emit compass2CalFitnessChanged(_rgCompassCalFitness[1]);
            emit compass3CalFitnessChanged(_rgCompassCalFitness[2]);
            emit compass1CalSucceededChanged(_rgCompassCalSucceeded[0]);
            emit compass2CalSucceededChanged(_rgCompassCalSucceeded[1]);
            emit compass3CalSucceededChanged(_rgCompassCalSucceeded[2]);
            if (_rgCompassCalSucceeded[0] && _rgCompassCalSucceeded[1] && _rgCompassCalSucceeded[2]) {
                _appendStatusLog(tr("All compasses calibrated successfully"));
                _appendStatusLog(tr("YOU MUST REBOOT YOUR VEHICLE NOW FOR NEW SETTINGS TO TAKE AFFECT"));
                _stopCalibration(StopCalibrationSuccessShowLog);
            } else {
                _appendStatusLog(tr("Compass calibration failed"));
                _appendStatusLog(tr("YOU MUST REBOOT YOUR VEHICLE NOW AND RETRY COMPASS CALIBRATION PRIOR TO FLIGHT"));
                _stopCalibration(StopCalibrationFailed);
            }
        } else if (additionalCompassCompleted) {
            _appendStatusLog(tr("Continue rotating..."));
        }

    }
}

void APMSensorsComponentController::_handleCommandLong(mavlink_message_t& message)
{
    bool                    updateImages = false;
    mavlink_command_long_t  commandLong;

    mavlink_msg_command_long_decode(&message, &commandLong);

    if (commandLong.command == MAV_CMD_ACCELCAL_VEHICLE_POS) {
        switch (static_cast<ACCELCAL_VEHICLE_POS>(static_cast<int>(commandLong.param1))) {
        case ACCELCAL_VEHICLE_POS_LEVEL:
            if (!_orientationCalDownSideInProgress) {
                updateImages = true;
                _orientationCalDownSideInProgress = true;
                _nextButton->setEnabled(true);
            }
            break;
        case ACCELCAL_VEHICLE_POS_LEFT:
            if (!_orientationCalLeftSideInProgress) {
                updateImages = true;
                _orientationCalDownSideDone =       true;
                _orientationCalDownSideInProgress = false;
                _orientationCalLeftSideInProgress = true;
                _progressBar->setProperty("value", (qreal)(17 / 100.0));
            }
            break;
        case ACCELCAL_VEHICLE_POS_RIGHT:
            if (!_orientationCalRightSideInProgress) {
                updateImages = true;
                _orientationCalLeftSideDone =       true;
                _orientationCalLeftSideInProgress = false;
                _orientationCalRightSideInProgress = true;
                _progressBar->setProperty("value", (qreal)(34 / 100.0));
            }
            break;
        case ACCELCAL_VEHICLE_POS_NOSEDOWN:
            if (!_orientationCalNoseDownSideInProgress) {
                updateImages = true;
                _orientationCalRightSideDone =       true;
                _orientationCalRightSideInProgress = false;
                _orientationCalNoseDownSideInProgress = true;
                _progressBar->setProperty("value", (qreal)(51 / 100.0));
            }
            break;
        case ACCELCAL_VEHICLE_POS_NOSEUP:
            if (!_orientationCalTailDownSideInProgress) {
                updateImages = true;
                _orientationCalNoseDownSideDone =       true;
                _orientationCalNoseDownSideInProgress = false;
                _orientationCalTailDownSideInProgress = true;
                _progressBar->setProperty("value", (qreal)(68 / 100.0));
            }
            break;
        case ACCELCAL_VEHICLE_POS_BACK:
            if (!_orientationCalUpsideDownSideInProgress) {
                updateImages = true;
                _orientationCalTailDownSideDone =       true;
                _orientationCalTailDownSideInProgress = false;
                _orientationCalUpsideDownSideInProgress = true;
                _progressBar->setProperty("value", (qreal)(85 / 100.0));
            }
            break;
        case ACCELCAL_VEHICLE_POS_SUCCESS:
            _stopCalibration(StopCalibrationSuccess);
            break;
        case ACCELCAL_VEHICLE_POS_FAILED:
            _stopCalibration(StopCalibrationFailed);
            break;
        case ACCELCAL_VEHICLE_POS_ENUM_END:
            // Make compiler happy
            break;
        }

        if (updateImages) {
            emit orientationCalSidesDoneChanged();
            emit orientationCalSidesInProgressChanged();
            emit orientationCalSidesRotateChanged();
        }
    }
}

void APMSensorsComponentController::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{
    Q_UNUSED(link);

    if (message.sysid != _vehicle->id()) {
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

void APMSensorsComponentController::_restorePreviousCompassCalFitness(void)
{
    if (_restoreCompassCalFitness) {
        _restoreCompassCalFitness = false;
        getParameterFact(FactSystem::defaultComponentId, _compassCalFitnessParam)->setRawValue(_previousCompassCalFitness);
    }
}
