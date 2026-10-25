#include "SensorsComponentController.h"

#include "AppMessages.h"
#include "MAVLinkLib.h"
#include "PX4SensorDevice.h"
#include "ParameterManager.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"

QGC_LOGGING_CATEGORY(SensorsComponentControllerLog, "AutoPilotPlugins.SensorsComponentController")

namespace {

// PX4 acks a calibration cancel twice: the commander answers TEMPORARILY_REJECTED because a calibration is running
// while the calibration itself takes the cancel and answers ACCEPTED. Either can arrive first, so neither is an error.
// The "calibration cancelled" status text is what ends the calibration.
// https://github.com/PX4/PX4-Autopilot/issues/29424
void px4SensorsCancelCalibrationResultHandler(void* /*resultHandlerData*/, int /*compId*/,
                                              const mavlink_command_ack_t& ack,
                                              Vehicle::MavCmdResultFailureCode_t failureCode)
{
    if (failureCode == Vehicle::MavCmdResultFailureNoResponseToCommand) {
        QGC::showAppMessage(SensorsComponentController::tr("Vehicle did not respond to the calibration cancel"));
        return;
    }
    if (ack.result == MAV_RESULT_ACCEPTED || ack.result == MAV_RESULT_TEMPORARILY_REJECTED) {
        return;
    }
    Vehicle::showCommandAckError(ack);
}

}  // namespace

SensorsComponentController::SensorsComponentController(void)
    : _showOrientationCalArea(false)
    , _gyroCalInProgress(false)
    , _magCalInProgress(false)
    , _accelCalInProgress(false)
    , _airspeedCalInProgress(false)
    , _levelCalInProgress(false)
    , _baroCalInProgress(false)
    , _orientationCalDownSideVisible(false)
    , _orientationCalUpsideDownSideVisible(false)
    , _orientationCalLeftSideVisible(false)
    , _orientationCalRightSideVisible(false)
    , _orientationCalNoseDownSideVisible(false)
    , _orientationCalTailDownSideVisible(false)
    , _orientationCalDownSideState(SideCalStateIdle)
    , _orientationCalUpsideDownSideState(SideCalStateIdle)
    , _orientationCalLeftSideState(SideCalStateIdle)
    , _orientationCalRightSideState(SideCalStateIdle)
    , _orientationCalNoseDownSideState(SideCalStateIdle)
    , _orientationCalTailDownSideState(SideCalStateIdle)
    , _unknownFirmwareVersion(false)
    , _waitingForCancel(false)
{
    connect(_vehicle, &Vehicle::sensorsParametersResetAck, this, &SensorsComponentController::_handleParametersReset);

    _largeVehicleCalibrationTimer.setSingleShot(true);
    _largeVehicleCalibrationTimer.setInterval(_largeVehicleCalibrationTimeoutMs);
    connect(&_largeVehicleCalibrationTimer, &QTimer::timeout, this,
            &SensorsComponentController::_largeVehicleCalibrationTimedOut);
}

bool SensorsComponentController::usingUDPLink(void)
{
    SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (sharedLink) {
        return sharedLink->linkConfiguration()->type() == LinkConfiguration::TypeUdp;
    } else {
        return false;
    }
}

/// Appends the specified text to the calibration log
void SensorsComponentController::_appendStatusLog(const QString& text)
{
    if (!_calibrationLog.isEmpty()) {
        _calibrationLog += QLatin1Char('\n');
    }
    _calibrationLog += text;
    emit calibrationLogChanged();
}

void SensorsComponentController::_setCalibrationResult(CalibrationResult result)
{
    _calibrationResult = result;
    emit calibrationResultChanged();
}

void SensorsComponentController::_startLogCalibration(void)
{
    _unknownFirmwareVersion = false;
    _calibrationFailureText.clear();
    _calibrationDetail.clear();
    _calibrationLog.clear();
    _sideAlreadyDone = false;
    emit calibrationLogChanged();
    _setCalibrationResult(CalibrationResultNone);
    _setCalibrationProgress(0);
    _hideAllCalAreas();

    connect(_vehicle, &Vehicle::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage,
            Qt::UniqueConnection);
}

void SensorsComponentController::_setCalibrationProgress(double progress)
{
    if (!qFuzzyIsNull(_calibrationProgress - progress)) {
        _calibrationProgress = progress;
        emit calibrationProgressChanged();
    }
}

void SensorsComponentController::_startVisualCalibration(void)
{
    _setAllSidesState(SideCalStateIncomplete);
    _setCalibrationProgress(0);
}

void SensorsComponentController::_setAllSidesState(SideCalState state)
{
    _orientationCalDownSideState = state;
    _orientationCalUpsideDownSideState = state;
    _orientationCalLeftSideState = state;
    _orientationCalRightSideState = state;
    _orientationCalTailDownSideState = state;
    _orientationCalNoseDownSideState = state;

    emit orientationCalSidesStateChanged();
}

void SensorsComponentController::_stopCalibration(SensorsComponentController::StopCalibrationCode code)
{
    disconnect(_vehicle, &Vehicle::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage);

    // The page shows every calibration in its row or in its panel, so a failure needs no message box
    const bool inlineCalibration = _baroCalInProgress || _levelCalInProgress || _gyroCalInProgress;

    if (code == StopCalibrationSuccess) {
        _setAllSidesState(SideCalStateCompleted);
        _setCalibrationProgress(1);
    } else {
        // Calibration results are discarded: return any partially completed sides
        // to the neutral idle preview state
        _setAllSidesState(SideCalStateIdle);
        _setCalibrationProgress(0);
    }

    _waitingForCancel = false;
    emit waitingForCancelChanged();

    _refreshParams();

    switch (code) {
        case StopCalibrationSuccess:
            _setCalibrationResult(CalibrationResultSuccess);
            if (_magCalInProgress) {
                _vehicle->setRebootRequired();
                emit magCalComplete();
            }
            break;

        case StopCalibrationCancelled:
            _hideAllCalAreas();
            break;

        default:
            // Assume failed
            _hideAllCalAreas();
            _setCalibrationResult(CalibrationResultFailed);
            break;
    }

    if (inlineCalibration) {
        emit inlineCalibrationFinished(code == StopCalibrationSuccess, _calibrationFailureText);
    }

    _magCalInProgress = false;
    _accelCalInProgress = false;
    _gyroCalInProgress = false;
    _airspeedCalInProgress = false;
    _levelCalInProgress = false;
    _baroCalInProgress = false;

    emit calibrationActiveChanged();
}

void SensorsComponentController::calibrateGyro(void)
{
    _startLogCalibration();
    _vehicle->startCalibration(QGCMAVLink::CalibrationGyro);
}

void SensorsComponentController::calibrateCompass(void)
{
    _startLogCalibration();
    _vehicle->startCalibration(QGCMAVLink::CalibrationMag);
}

void SensorsComponentController::calibrateAccel(void)
{
    _startLogCalibration();
    _vehicle->startCalibration(QGCMAVLink::CalibrationAccel);
}

void SensorsComponentController::calibrateLevel(void)
{
    _startLogCalibration();
    _vehicle->startCalibration(QGCMAVLink::CalibrationLevel);
}

void SensorsComponentController::calibrateAirspeed(void)
{
    _startLogCalibration();
    _vehicle->startCalibration(QGCMAVLink::CalibrationPX4Airspeed);
}

void SensorsComponentController::calibrateBaro(void)
{
    _startLogCalibration();
    _vehicle->startCalibration(QGCMAVLink::CalibrationPX4Pressure);
}

void SensorsComponentController::calibrateCompassLargeVehicle(double headingDegrees)
{
    // Watch first: a command which cannot be sent at all reports its failure before sendMavCommand returns.
    // The result of a command which the vehicle refuses is reported here instead of as an error box.
    _watchLargeVehicleCalibration();

    // The firmware takes the vehicle's GPS position when the latitude and longitude are not set
    _vehicle->sendMavCommand(_vehicle->defaultComponentId(), MAV_CMD_FIXED_MAG_CAL_YAW,
                             false,  // showError
                             static_cast<float>(headingDegrees),
                             0,      // Unused by PX4
                             0,      // Latitude, unset
                             0);     // Longitude, unset
}

SensorsComponentController::LargeVehicleCalibrationResult SensorsComponentController::largeVehicleCalibrationResult(
    const QString& text)
{
    // Texts from the firmware's do_mag_calibration_quick() and the worker thread
    if (text.contains(QStringLiteral("Mag quick calibration finished"), Qt::CaseInsensitive)) {
        return LargeVehicleCalibrationResult::Succeeded;
    }

    static const QStringList failureTexts = {
        QStringLiteral("mag quick cal"),
        QStringLiteral("calibration failed"),
        QStringLiteral("calibration save failed"),
        QStringLiteral("cannot open calibration storage"),
    };
    for (const QString& failureText : failureTexts) {
        if (text.contains(failureText, Qt::CaseInsensitive)) {
            return LargeVehicleCalibrationResult::Failed;
        }
    }

    return LargeVehicleCalibrationResult::Pending;
}

void SensorsComponentController::_watchLargeVehicleCalibration(void)
{
    connect(_vehicle, &Vehicle::textMessageReceived, this,
            &SensorsComponentController::_handleLargeVehicleCalibrationMessage, Qt::UniqueConnection);
    connect(_vehicle, &Vehicle::mavCommandResult, this, &SensorsComponentController::_handleLargeVehicleCommandResult,
            Qt::UniqueConnection);
    _largeVehicleCalibrationTimer.start();
    _setLargeVehicleCalibrationPending(true);
}

void SensorsComponentController::_stopWatchingLargeVehicleCalibration(void)
{
    _largeVehicleCalibrationTimer.stop();
    disconnect(_vehicle, &Vehicle::textMessageReceived, this,
               &SensorsComponentController::_handleLargeVehicleCalibrationMessage);
    disconnect(_vehicle, &Vehicle::mavCommandResult, this,
               &SensorsComponentController::_handleLargeVehicleCommandResult);
    _setLargeVehicleCalibrationPending(false);
}

void SensorsComponentController::_setLargeVehicleCalibrationPending(bool pending)
{
    if (_largeVehicleCalibrationPending != pending) {
        _largeVehicleCalibrationPending = pending;
        emit largeVehicleCalibrationPendingChanged();
    }
}

QString SensorsComponentController::largeVehicleCommandFailureText(int failureCode)
{
    switch (failureCode) {
        case Vehicle::MavCmdResultFailureNoResponseToCommand:
            return tr("The vehicle did not respond.");
        case Vehicle::MavCmdResultFailureDuplicateCommand:
            return tr("A calibration request is already waiting for the vehicle.");
        default:
            break;
    }

    return tr(
        "The vehicle did not accept the calibration. It may be armed or busy, or its firmware may not support it.");
}

void SensorsComponentController::_handleLargeVehicleCommandResult(quint32 vehicleId, int targetComponent, int command,
                                                                  int ackResult, int failureCode)
{
    Q_UNUSED(targetComponent);

    if (vehicleId != _vehicle->id() || command != MAV_CMD_FIXED_MAG_CAL_YAW) {
        return;
    }

    if (ackResult == MAV_RESULT_ACCEPTED || ackResult == MAV_RESULT_IN_PROGRESS) {
        return;
    }

    _stopWatchingLargeVehicleCalibration();
    emit largeVehicleCalibrationFinished(false, largeVehicleCommandFailureText(failureCode));
}

void SensorsComponentController::_handleLargeVehicleCalibrationMessage(quint32 uasId, int compId, int severity,
                                                                       QString text, const QString& description)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    Q_UNUSED(description);

    if (uasId != _vehicle->id()) {
        return;
    }

    const LargeVehicleCalibrationResult result = largeVehicleCalibrationResult(text);
    if (result == LargeVehicleCalibrationResult::Pending) {
        return;
    }

    _stopWatchingLargeVehicleCalibration();
    _refreshParams();
    emit largeVehicleCalibrationFinished(result == LargeVehicleCalibrationResult::Succeeded, text);
}

void SensorsComponentController::_largeVehicleCalibrationTimedOut(void)
{
    // The firmware may have saved offsets without saying so, so refresh anyway
    _stopWatchingLargeVehicleCalibration();
    _refreshParams();
    emit largeVehicleCalibrationFinished(false,
                                         tr("No result from the vehicle. Check the compass offsets, or try again."));
}

QString SensorsComponentController::deviceDescription(int deviceId) const
{
    return PX4SensorDevice::description(static_cast<quint32>(deviceId));
}

SensorsComponentController::ActiveCalibration SensorsComponentController::activeCalibration() const
{
    if (_magCalInProgress) {
        return ActiveCalibrationCompass;
    }
    if (_gyroCalInProgress) {
        return ActiveCalibrationGyro;
    }
    if (_accelCalInProgress) {
        return ActiveCalibrationAccel;
    }
    if (_levelCalInProgress) {
        return ActiveCalibrationLevel;
    }
    if (_airspeedCalInProgress) {
        return ActiveCalibrationAirspeed;
    }
    if (_baroCalInProgress) {
        return ActiveCalibrationBaro;
    }
    return ActiveCalibrationNone;
}

void SensorsComponentController::_handleUASTextMessage(quint32 uasId, int compId, int severity, QString text,
                                                       const QString& description)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    Q_UNUSED(description);

    if (uasId != _vehicle->id()) {
        return;
    }

    // Needed for level horizon calibration
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");

    if (text.contains("progress <")) {
        QString percent = text.split("<").last().split(">").first();
        bool ok;
        int p = percent.toInt(&ok);
        if (ok) {
            _setCalibrationProgress(p / 100.0);
        }
        return;
    }

    _appendStatusLog(text);
    qCDebug(SensorsComponentControllerLog) << text;

    if (_unknownFirmwareVersion) {
        // We don't know how to do visual cal with the version of firwmare
        return;
    }

    // All calibration messages start with [cal]
    QString calPrefix("[cal] ");
    if (!text.startsWith(calPrefix)) {
        // The firmware logs the reason a calibration failed, such as "motion during calibration", just before it
        // reports the failure
        _calibrationDetail = text;
        return;
    }
    text = text.right(text.length() - calPrefix.length());

    QString calStartPrefix("calibration started: ");
    if (text.startsWith(calStartPrefix)) {
        text = text.right(text.length() - calStartPrefix.length());

        // Split version number and cal type
        QStringList parts = text.split(" ");
        if (parts.count() != 2 && parts[0].toInt() != _supportedFirmwareCalVersion) {
            _unknownFirmwareVersion = true;
            QString msg = tr("Unsupported calibration firmware version, using log");
            _appendStatusLog(msg);
            qDebug() << msg;
            return;
        }

        _startVisualCalibration();

        text = parts[1];
        if (text == "accel" || text == "mag" || text == "gyro") {
            // _startVisualCalibration() above reset all side indicators to Incomplete

            // Reset all visibility
            _orientationCalDownSideVisible = false;
            _orientationCalUpsideDownSideVisible = false;
            _orientationCalLeftSideVisible = false;
            _orientationCalRightSideVisible = false;
            _orientationCalTailDownSideVisible = false;
            _orientationCalNoseDownSideVisible = false;

            if (text == "accel") {
                _accelCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalUpsideDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalRightSideVisible = true;
                _orientationCalTailDownSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else if (text == "mag") {
                // Work out what the autopilot is configured to. CAL_MAG_SIDES is the legacy name of SENS_MAG_SIDES.
                int sides = 0;

                const QString sidesParamName = _vehicle->parameterManager()->parameterExists(
                                                   ParameterManager::defaultComponentId, "SENS_MAG_SIDES")
                                                   ? QStringLiteral("SENS_MAG_SIDES")
                                                   : QStringLiteral("CAL_MAG_SIDES");

                if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId,
                                                                  sidesParamName)) {
                    // Read the requested calibration directions off the system
                    Fact* const calMagSides = _vehicle->parameterManager()->getParameter(
                        ParameterManager::defaultComponentId, sidesParamName);
                    if (calMagSides) {
                        sides = calMagSides->rawValue().toFloat();
                    } else {
                        qCWarning(SensorsComponentControllerLog)
                            << sidesParamName << "disappeared while starting calibration";
                        sides = (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
                    }
                } else {
                    // There is no valid setting, default to all six sides
                    sides = (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0);
                }

                _magCalInProgress = true;
                _orientationCalTailDownSideVisible =   ((sides & (1 << 0)) > 0);
                _orientationCalNoseDownSideVisible =   ((sides & (1 << 1)) > 0);
                _orientationCalLeftSideVisible =       ((sides & (1 << 2)) > 0);
                _orientationCalRightSideVisible =      ((sides & (1 << 3)) > 0);
                _orientationCalUpsideDownSideVisible = ((sides & (1 << 4)) > 0);
                _orientationCalDownSideVisible =       ((sides & (1 << 5)) > 0);
            } else if (text == "gyro") {
                _gyroCalInProgress = true;
                _orientationCalDownSideVisible = true;
            } else {
                qWarning() << "Unknown calibration message type" << text;
            }
            emit orientationCalSidesVisibleChanged();
            _updateAndEmitShowOrientationCalArea(true);
        } else if (text == "airspeed") {
            _airspeedCalInProgress = true;
        } else if (text == "level") {
            _levelCalInProgress = true;
        } else if (text == "baro") {
            _baroCalInProgress = true;
        }
        emit calibrationActiveChanged();
        return;
    }

    if (text.endsWith("orientation detected")) {
        QString side = text.section(" ", 0, 0);
        qCDebug(SensorsComponentControllerLog) << "Side started" << side;

        if (side == "down") {
            _orientationCalDownSideState = SideCalStateInProgress;
        } else if (side == "up") {
            _orientationCalUpsideDownSideState = SideCalStateInProgress;
        } else if (side == "left") {
            _orientationCalLeftSideState = SideCalStateInProgress;
        } else if (side == "right") {
            _orientationCalRightSideState = SideCalStateInProgress;
        } else if (side == "front") {
            _orientationCalNoseDownSideState = SideCalStateInProgress;
        } else if (side == "back") {
            _orientationCalTailDownSideState = SideCalStateInProgress;
        }

        _sideAlreadyDone = false;

        emit orientationCalSidesStateChanged();
        return;
    }

    if (text.endsWith("side done, rotate to a different side")) {
        QString side = text.section(" ", 0, 0);
        qCDebug(SensorsComponentControllerLog) << "Side finished" << side;

        if (side == "down") {
            _orientationCalDownSideState = SideCalStateCompleted;
        } else if (side == "up") {
            _orientationCalUpsideDownSideState = SideCalStateCompleted;
        } else if (side == "left") {
            _orientationCalLeftSideState = SideCalStateCompleted;
        } else if (side == "right") {
            _orientationCalRightSideState = SideCalStateCompleted;
        } else if (side == "front") {
            _orientationCalNoseDownSideState = SideCalStateCompleted;
        } else if (side == "back") {
            _orientationCalTailDownSideState = SideCalStateCompleted;
        }

        _sideAlreadyDone = false;

        emit orientationCalSidesStateChanged();
        return;
    }

    if (text.endsWith("side already completed")) {
        _sideAlreadyDone = true;
        emit orientationCalSidesStateChanged();
        return;
    }

    QString calCompletePrefix("calibration done:");
    if (text.startsWith(calCompletePrefix)) {
        _stopCalibration(StopCalibrationSuccess);
        return;
    }

    if (text.startsWith("calibration cancelled")) {
        _stopCalibration(_waitingForCancel ? StopCalibrationCancelled : StopCalibrationFailed);
        return;
    }

    if (text.startsWith("calibration failed")) {
        _calibrationFailureText =
            _calibrationDetail.isEmpty() ? text : _calibrationDetail + QStringLiteral(": ") + text;
        _stopCalibration(StopCalibrationFailed);
        return;
    }
}

void SensorsComponentController::_refreshParams(void)
{
    _vehicle->parameterManager()->bulkRefresh(ParameterManager::defaultComponentId, {
        QStringLiteral("CAL_MAG0_ID"), QStringLiteral("CAL_MAG1_ID"), QStringLiteral("CAL_MAG2_ID"),
        QStringLiteral("CAL_MAG0_ROT"), QStringLiteral("CAL_MAG1_ROT"), QStringLiteral("CAL_MAG2_ROT"),
        QStringLiteral("CAL_*"),
        QStringLiteral("SENS_*"),
    }, false /* notifyFailure */);
}

void SensorsComponentController::_updateAndEmitShowOrientationCalArea(bool show)
{
    _showOrientationCalArea = show;
    emit showOrientationCalAreaChanged();
}

void SensorsComponentController::_hideAllCalAreas(void)
{
    _updateAndEmitShowOrientationCalArea(false);
}

void SensorsComponentController::cancelCalibration(void)
{
    _waitingForCancel = true;
    emit waitingForCancelChanged();

    const Vehicle::MavCmdAckHandlerInfo_t handlerInfo{
        .resultHandler = px4SensorsCancelCalibrationResultHandler,
        .resultHandlerData = nullptr,
        .progressHandler = nullptr,
        .progressHandlerData = nullptr,
    };
    // All zero params cancel the running calibration
    _vehicle->sendMavCommandWithHandler(&handlerInfo, _vehicle->defaultComponentId(), MAV_CMD_PREFLIGHT_CALIBRATION);
}

void SensorsComponentController::_handleParametersReset(bool success)
{
    if (success) {
        QGC::showAppMessage(tr("Reset successful"));

        QTimer::singleShot(1000, this, [this]() {
            _refreshParams();
        });
    }
    else {
        QGC::showAppMessage(tr("Reset failed"));
    }
}

void SensorsComponentController::resetFactoryParameters()
{
    auto compId = _vehicle->defaultComponentId();

    _vehicle->sendMavCommand(compId,
                             MAV_CMD_PREFLIGHT_STORAGE,
                             true,  // showError
                             3,     // Reset factory parameters
                             -1);   // Don't do anything with mission storage
}
