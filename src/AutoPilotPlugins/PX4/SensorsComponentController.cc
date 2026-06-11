#include "SensorsComponentController.h"
#include "AppMessages.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SensorsComponentControllerLog, "AutoPilotPlugins.SensorsComponentController")

SensorsComponentController::SensorsComponentController(void)
    : _statusLog                                (nullptr)
    , _progressBar                              (nullptr)
    , _showOrientationCalArea                   (false)
    , _gyroCalInProgress                        (false)
    , _magCalInProgress                         (false)
    , _accelCalInProgress                       (false)
    , _airspeedCalInProgress                    (false)
    , _levelCalInProgress                       (false)
    , _orientationCalDownSideVisible            (false)
    , _orientationCalUpsideDownSideVisible      (false)
    , _orientationCalLeftSideVisible            (false)
    , _orientationCalRightSideVisible           (false)
    , _orientationCalNoseDownSideVisible        (false)
    , _orientationCalTailDownSideVisible        (false)
    , _orientationCalDownSideState              (SideCalStateIdle)
    , _orientationCalUpsideDownSideState        (SideCalStateIdle)
    , _orientationCalLeftSideState              (SideCalStateIdle)
    , _orientationCalRightSideState             (SideCalStateIdle)
    , _orientationCalNoseDownSideState          (SideCalStateIdle)
    , _orientationCalTailDownSideState          (SideCalStateIdle)
    , _unknownFirmwareVersion                   (false)
    , _waitingForCancel                         (false)
{
    connect(_vehicle, &Vehicle::sensorsParametersResetAck, this, &SensorsComponentController::_handleParametersReset);

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

/// Appends the specified text to the status log area in the ui
void SensorsComponentController::_appendStatusLog(const QString& text)
{
    if (!_statusLog) {
        qWarning() << "Internal error";
        return;
    }

    QString varText = text;
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_ARG(QString, varText));
}

void SensorsComponentController::_startLogCalibration(void)
{
    _unknownFirmwareVersion = false;
    _hideAllCalAreas();

    connect(_vehicle, &Vehicle::textMessageReceived, this, &SensorsComponentController::_handleUASTextMessage);
}

void SensorsComponentController::_startVisualCalibration(void)
{
    _setAllSidesState(SideCalStateIncomplete);

    _progressBar->setProperty("value", 0);
}

/// Returns the orientation preview side indicators to the neutral Idle state.
/// Called when the preview switches to a different sensor so completion state from
/// a previous calibration doesn't carry over.
void SensorsComponentController::resetSidesToIdle(void)
{
    if (calibrationActive()) {
        return;
    }
    _setAllSidesState(SideCalStateIdle);
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

    if (code == StopCalibrationSuccess) {
        _setAllSidesState(SideCalStateCompleted);

        _progressBar->setProperty("value", 1);
    } else {
        // Calibration results are discarded: return any partially completed sides
        // to the neutral idle preview state
        _setAllSidesState(SideCalStateIdle);

        _progressBar->setProperty("value", 0);
    }

    _waitingForCancel = false;
    emit waitingForCancelChanged();

    _refreshParams();

    switch (code) {
        case StopCalibrationSuccess:
            _orientationCalAreaHelpText->setProperty("text", tr("Calibration complete"));
            if (!_airspeedCalInProgress && !_levelCalInProgress) {
                emit resetStatusTextArea();
            }
            if (_magCalInProgress) {
                emit magCalComplete();
            }
            break;

        case StopCalibrationCancelled:
            emit resetStatusTextArea();
            _hideAllCalAreas();
            break;

        default:
            // Assume failed
            _hideAllCalAreas();
            QGC::showAppMessage(tr("Calibration failed. Calibration log will be displayed."));
            break;
    }

    _magCalInProgress = false;
    _accelCalInProgress = false;
    _gyroCalInProgress = false;
    _airspeedCalInProgress = false;
    _levelCalInProgress = false;

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

void SensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text, const QString &description)
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
            if (_progressBar) {
                _progressBar->setProperty("value", (float)(p / 100.0));
            } else {
                qWarning() << "Internal error";
            }
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

            _orientationCalAreaHelpText->setProperty("text", tr("Place your vehicle into one of the Incomplete orientations shown below and hold it still"));

            if (text == "accel") {
                _accelCalInProgress = true;
                _orientationCalDownSideVisible = true;
                _orientationCalUpsideDownSideVisible = true;
                _orientationCalLeftSideVisible = true;
                _orientationCalRightSideVisible = true;
                _orientationCalTailDownSideVisible = true;
                _orientationCalNoseDownSideVisible = true;
            } else if (text == "mag") {

                // Work out what the autopilot is configured to
                int sides = 0;

                if (_vehicle->parameterManager()->parameterExists(ParameterManager::defaultComponentId, "CAL_MAG_SIDES")) {
                    // Read the requested calibration directions off the system
                    sides = _vehicle->parameterManager()->getParameter(ParameterManager::defaultComponentId, "CAL_MAG_SIDES")->rawValue().toFloat();
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

        if (_magCalInProgress) {
            _orientationCalAreaHelpText->setProperty("text", tr("Rotate the vehicle continuously as shown in the diagram until marked as Completed"));
        } else {
            _orientationCalAreaHelpText->setProperty("text", tr("Hold still in the current orientation"));
        }

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

        _orientationCalAreaHelpText->setProperty("text", tr("Place you vehicle into one of the orientations shown below and hold it still"));

        emit orientationCalSidesStateChanged();
        return;
    }

    if (text.endsWith("side already completed")) {
        _orientationCalAreaHelpText->setProperty("text", tr("Orientation already completed, place you vehicle into one of the incomplete orientations shown below and hold it still"));
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
    // The firmware doesn't allow us to cancel calibration. The best we can do is wait
    // for it to timeout.
    _waitingForCancel = true;
    emit waitingForCancelChanged();
    _vehicle->stopCalibration(true /* showError */);
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
