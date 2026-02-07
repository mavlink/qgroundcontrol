#include "JoystickComponent.h"
#include <QtCore/QLoggingCategory>

#include "Joystick.h"
#include "JoystickManager.h"

Q_STATIC_LOGGING_CATEGORY(JoystickComponentLog, "AutoPilotPlugins.JoystickComponent")

JoystickComponent::JoystickComponent(Vehicle *vehicle, AutoPilotPlugin *autopilot, QObject *parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::KnownJoystickVehicleComponent, parent)
    , _name(tr("Joystick"))
{
    qCDebug(JoystickComponentLog) << this;

    (void) connect(JoystickManager::instance(), &JoystickManager::activeJoystickChanged, this, &JoystickComponent::_activeJoystickChanged);
    (void) connect(JoystickManager::instance(), &JoystickManager::activeJoystickChanged, this, &VehicleComponent::setupCompleteChanged);

    _activeJoystickChanged(JoystickManager::instance()->activeJoystick());
}

JoystickComponent::~JoystickComponent()
{
    qCDebug(JoystickComponentLog) << this;
}

QString JoystickComponent::description() const
{
    return tr("Configure joystick input, calibrate axes, and manage button assignments.");
}

bool JoystickComponent::setupComplete() const
{
    return !JoystickManager::instance()->activeJoystick() || JoystickManager::instance()->activeJoystick()->settings()->calibrated()->rawValue().toBool();
}

QUrl JoystickComponent::setupSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/VehicleSetup/JoystickComponent.qml"));
}

QUrl JoystickComponent::summaryQmlSource() const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/Common/JoystickComponentSummary.qml"));
}

QString JoystickComponent::joystickStatusText() const
{
    if (!_activeJoystick) {
        return tr("No joystick detected");
    }

    if (_activeJoystick->axisCount() == 0) {
        return tr("Buttons only");
    }

    if (!_activeJoystick->requiresCalibration()) {
        return tr("Ready");
    }

    return _activeJoystick->settings()->calibrated()->rawValue().toBool() ? tr("Calibrated") : tr("Needs calibration");
}

QString JoystickComponent::joystickFeaturesText() const
{
    if (!_activeJoystick) {
        return QString();
    }

    QStringList features;

    if (_activeJoystick->hasRumble()) {
        features.append(tr("Rumble"));
    }
    if (_activeJoystick->hasRumbleTriggers()) {
        features.append(tr("Trigger Rumble"));
    }
    if (_activeJoystick->hasLED()) {
        features.append(tr("LED"));
    }
    if (_activeJoystick->hasGyroscope()) {
        features.append(tr("Gyro"));
    }
    if (_activeJoystick->hasAccelerometer()) {
        features.append(tr("Accel"));
    }
    if (_activeJoystick->touchpadCount() > 0) {
        features.append(tr("Touchpad"));
    }

    return features.join(QStringLiteral(", "));
}

void JoystickComponent::_activeJoystickChanged(Joystick *joystick)
{
    if (_activeJoystick) {
        (void) disconnect(_activeJoystick->settings()->calibrated(), &Fact::rawValueChanged, this, &VehicleComponent::setupCompleteChanged);
        (void) disconnect(_activeJoystick->settings()->calibrated(), &Fact::rawValueChanged, this, &JoystickComponent::_joystickCalibrationChanged);
        (void) disconnect(_activeJoystick, &Joystick::batteryStateChanged, this, &JoystickComponent::_joystickBatteryChanged);
        _activeJoystick = nullptr;
    }

    if (joystick) {
        _activeJoystick = joystick;
        (void) connect(_activeJoystick->settings()->calibrated(), &Fact::rawValueChanged, this, &VehicleComponent::setupCompleteChanged);
        (void) connect(_activeJoystick->settings()->calibrated(), &Fact::rawValueChanged, this, &JoystickComponent::_joystickCalibrationChanged);
        (void) connect(_activeJoystick, &Joystick::batteryStateChanged, this, &JoystickComponent::_joystickBatteryChanged);
    }

    emit activeJoystickChanged();
    emit joystickStatusChanged();
}

void JoystickComponent::_joystickCalibrationChanged()
{
    emit joystickStatusChanged();
}

void JoystickComponent::_joystickBatteryChanged()
{
    emit joystickStatusChanged();
}
