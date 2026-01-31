#include "APMSensorsComponentController.h"
#include "APMAutoPilotPlugin.h"
#include "APMSensorsComponent.h"
#include "APMSensorCalibrationStateMachine.h"
#include "MAVLinkProtocol.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QVariant>

QGC_LOGGING_CATEGORY(APMSensorsComponentControllerLog, "AutoPilotPlugins.APMSensorsComponentController")
QGC_LOGGING_CATEGORY(APMSensorsComponentControllerVerboseLog, "AutoPilotPlugins.APMSensorsComponentController:verbose")

APMSensorsComponentController::APMSensorsComponentController(QObject *parent)
    : SensorCalibrationControllerBase(parent)
{
    APMAutoPilotPlugin *const apmPlugin = qobject_cast<APMAutoPilotPlugin*>(_vehicle->autopilotPlugin());

    // Find the sensors component
    for (const QVariant &varVehicleComponent : apmPlugin->vehicleComponents()) {
        _sensorsComponent = qobject_cast<APMSensorsComponent*>(varVehicleComponent.value<VehicleComponent*>());
        if (_sensorsComponent) {
            break;
        }
    }

    if (_sensorsComponent) {
        (void) connect(_sensorsComponent, &VehicleComponent::setupCompleteChanged, this, &APMSensorsComponentController::setupNeededChanged);
    } else {
        qCWarning(APMSensorsComponentControllerLog) << "Sensors component is missing";
    }

    // Create the calibration state machine
    _stateMachine = new APMSensorCalibrationStateMachine(this, this);
}

APMSensorsComponentController::~APMSensorsComponentController()
{
    // State machine handles cleanup on destruction
}

void APMSensorsComponentController::_resetInternalState()
{
    // Reset with all sides marked done (hides orientation cal UI)
    _orientationState.reset(true);
}

void APMSensorsComponentController::_mavCommandResult(int vehicleId, int component, int command, int result, int failureCode)
{
    // Delegate to state machine
    _stateMachine->handleMavCommandResult(vehicleId, component, command, result, failureCode);
}

void APMSensorsComponentController::calibrateCompass()
{
    _stateMachine->calibrateCompass();
}

void APMSensorsComponentController::calibrateCompassNorth(float lat, float lon, int mask)
{
    _stateMachine->calibrateCompassNorth(lat, lon, mask);
}

void APMSensorsComponentController::calibrateAccel(bool doSimpleAccelCal)
{
    _stateMachine->calibrateAccel(doSimpleAccelCal);
}

void APMSensorsComponentController::calibrateMotorInterference()
{
    _stateMachine->calibrateMotorInterference();
}

void APMSensorsComponentController::levelHorizon()
{
    _stateMachine->levelHorizon();
}

void APMSensorsComponentController::calibratePressure()
{
    _stateMachine->calibratePressure();
}

void APMSensorsComponentController::calibrateGyro()
{
    _stateMachine->calibrateGyro();
}

void APMSensorsComponentController::_handleTextMessage(int sysid, int componentid, int severity, const QString &text, const QString &description)
{
    Q_UNUSED(componentid); Q_UNUSED(severity); Q_UNUSED(description);

    if (sysid != _vehicle->id()) {
        return;
    }

    const QString originalMessageText = text;
    const QString messageText = text.toLower();

    const QStringList hidePrefixList = { QStringLiteral("prearm:"), QStringLiteral("ekf"), QStringLiteral("arm"), QStringLiteral("initialising") };
    for (const QString &hidePrefix : hidePrefixList) {
        if (messageText.startsWith(hidePrefix)) {
            return;
        }
    }

    appendStatusLog(originalMessageText);
    qCDebug(APMSensorsComponentControllerLog) << originalMessageText << severity;
}

void APMSensorsComponentController::_refreshParams()
{
    static const QStringList fastRefreshList = {
        QStringLiteral("COMPASS_OFS_X"), QStringLiteral("COMPASS_OFS_Y"), QStringLiteral("COMPASS_OFS_Z"),
        QStringLiteral("INS_ACCOFFS_X"), QStringLiteral("INS_ACCOFFS_Y"), QStringLiteral("INS_ACCOFFS_Z")
    };

    for (const QString &paramName : fastRefreshList) {
        _vehicle->parameterManager()->refreshParameter(ParameterManager::defaultComponentId, paramName);
    }

    // Now ask for all to refresh
    _vehicle->parameterManager()->refreshParametersPrefix(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_"));
    _vehicle->parameterManager()->refreshParametersPrefix(ParameterManager::defaultComponentId, QStringLiteral("INS_"));
}

void APMSensorsComponentController::cancelCalibration()
{
    _stateMachine->cancelCalibration();
}

void APMSensorsComponentController::nextClicked()
{
    _stateMachine->nextClicked();
}

bool APMSensorsComponentController::compassSetupNeeded() const
{
    return _sensorsComponent->compassSetupNeeded();
}

bool APMSensorsComponentController::accelSetupNeeded() const
{
    return _sensorsComponent->accelSetupNeeded();
}

bool APMSensorsComponentController::usingUDPLink() const
{
    const SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return false;
    }

    return (sharedLink->linkConfiguration()->type() == LinkConfiguration::TypeUdp);
}

void APMSensorsComponentController::_mavlinkMessageReceived(LinkInterface *link, const mavlink_message_t &message)
{
    // Delegate to state machine
    _stateMachine->handleMavlinkMessage(link, message);
}

void APMSensorsComponentController::setButtonsEnabled(bool enabled)
{
    emit setAllCalButtonsEnabled(enabled);
}
