#include "SensorsComponentController.h"
#include "PX4SensorCalibrationStateMachine.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(SensorsComponentControllerLog, "AutoPilotPlugins.SensorsComponentController")

SensorsComponentController::SensorsComponentController(void)
    : SensorCalibrationControllerBase()
{
    connect(_vehicle, &Vehicle::sensorsParametersResetAck, this, &SensorsComponentController::_handleParametersReset);

    _stateMachine = new PX4SensorCalibrationStateMachine(this, this);
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

void SensorsComponentController::_resetInternalState(void)
{
    // Reset with all sides marked done (hides orientation cal UI)
    _orientationState.reset(true);
}

void SensorsComponentController::calibrateGyro(void)
{
    _stateMachine->calibrateGyro();
}

void SensorsComponentController::calibrateCompass(void)
{
    _stateMachine->calibrateCompass();
}

void SensorsComponentController::calibrateAccel(void)
{
    _stateMachine->calibrateAccel();
}

void SensorsComponentController::calibrateLevel(void)
{
    _stateMachine->calibrateLevel();
}

void SensorsComponentController::calibrateAirspeed(void)
{
    _stateMachine->calibrateAirspeed();
}

void SensorsComponentController::_handleUASTextMessage(int uasId, int compId, int severity, QString text, const QString &description)
{
    Q_UNUSED(compId);
    Q_UNUSED(severity);
    Q_UNUSED(description);

    if (uasId != _vehicle->id()) {
        return;
    }

    _stateMachine->handleTextMessage(text);
}

void SensorsComponentController::_refreshParams(void)
{
    QStringList fastRefreshList;

    // We ask for a refresh on these first so that the rotation combo show up as fast as possible
    fastRefreshList << "CAL_MAG0_ID" << "CAL_MAG1_ID" << "CAL_MAG2_ID" << "CAL_MAG0_ROT" << "CAL_MAG1_ROT" << "CAL_MAG2_ROT";
    for (const QString &paramName : std::as_const(fastRefreshList)) {
        _vehicle->parameterManager()->refreshParameter(ParameterManager::defaultComponentId, paramName);
    }

    // Now ask for all to refresh
    _vehicle->parameterManager()->refreshParametersPrefix(ParameterManager::defaultComponentId, "CAL_");
    _vehicle->parameterManager()->refreshParametersPrefix(ParameterManager::defaultComponentId, "SENS_");
}

void SensorsComponentController::cancelCalibration(void)
{
    _stateMachine->cancelCalibration();
}

void SensorsComponentController::_handleParametersReset(bool success)
{
    if (success) {
        qgcApp()->showAppMessage(tr("Reset successful"));

        QTimer::singleShot(1000, this, [this]() {
            _refreshParams();
        });
    }
    else {
        qgcApp()->showAppMessage(tr("Reset failed"));
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

void SensorsComponentController::setButtonsEnabled(bool enabled)
{
    if (_compassButton) {
        _compassButton->setProperty("enabled", enabled);
    }
    if (_gyroButton) {
        _gyroButton->setProperty("enabled", enabled);
    }
    if (_accelButton) {
        _accelButton->setProperty("enabled", enabled);
    }
    if (_airspeedButton) {
        _airspeedButton->setProperty("enabled", enabled);
    }
    if (_levelButton) {
        _levelButton->setProperty("enabled", enabled);
    }
    if (_setOrientationsButton) {
        _setOrientationsButton->setProperty("enabled", enabled);
    }
}
