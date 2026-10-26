#include "ActuatorComponent.h"
#include "MAVLinkLib.h"
#include "QGCApplication.h"
#include "GeometryImage.h"
#include "Actuators/Actuators.h"
#include "Vehicle.h"

#include <QtQml/QQmlApplicationEngine>

static bool imageProviderAdded{false};

ActuatorComponent::ActuatorComponent(Vehicle* vehicle, AutoPilotPlugin* autopilot, QObject* parent)
    : VehicleComponent(vehicle, autopilot, AutoPilotPlugin::UnknownVehicleComponent, parent)
    , _name(tr("Actuators"))
    , _actuators(*vehicle->actuators())
{
    if (!imageProviderAdded) {
        // TODO: qmlAppEngine should not be accessed inside app
        qgcApp()->qmlAppEngine()->addImageProvider(QLatin1String("actuators"), GeometryImage::VehicleGeometryImageProvider::instance());
        imageProviderAdded = true;
    }

    connect(&_actuators, &Actuators::hasUnsetRequiredFunctionsChanged, this, [this]() { _triggerUpdated({}); });
}

QString ActuatorComponent::name(void) const
{
    return _name;
}

QString ActuatorComponent::description(void) const
{
    return "";
}

QString ActuatorComponent::iconResource(void) const
{
    return QStringLiteral("/qmlimages/MotorComponentIcon.svg");
}

bool ActuatorComponent::requiresSetup(void) const
{
    return true;
}

bool ActuatorComponent::setupComplete(void) const
{
    return !_actuators.hasUnsetRequiredFunctions();
}

QStringList ActuatorComponent::setupCompleteChangedTriggerList(void) const
{
    return QStringList();
}

QUrl ActuatorComponent::setupSource(void) const
{
    return QUrl::fromUserInput(QStringLiteral("qrc:/qml/QGroundControl/AutoPilotPlugins/PX4/ActuatorComponent.qml"));
}

QUrl ActuatorComponent::summaryQmlSource(void) const
{
    return QUrl();
}

QList<VehicleSummaryItem> ActuatorComponent::summaryItems() const
{
    if (!_actuators.isInitialized()) {
        return {};
    }

    QList<VehicleSummaryItem> items;
    Mixer::Mixers* const mixer = _actuators.mixer();
    const QString geometry = mixer->title();
    if (!geometry.isEmpty()) {
        items.append({tr("Geometry"), geometry});
    }
    if (_actuators.hasUnsetRequiredFunctions()) {
        items.append({tr("Required Outputs"), tr("Setup required"), VehicleSummaryItem::Attention});
    }

    const auto groupTitle = [mixer](int function) {
        const Mixer::ActuatorTypes& actuatorTypes = mixer->actuatorTypes();
        for (auto it = actuatorTypes.cbegin(); it != actuatorTypes.cend(); ++it) {
            if (function < it->functionMin || function > it->functionMax) {
                continue;
            }
            switch (ActuatorGeometry::typeFromStr(it.key())) {
            case ActuatorGeometry::Type::Motor:
                return tr("Motors");
            case ActuatorGeometry::Type::Servo:
                return tr("Servos");
            default:
                break;
            }
            break;
        }

        return tr("Other Outputs");
    };

    const QMap<int, QStringList> channels = _actuators.channelsByFunction();
    for (auto it = channels.cbegin(); it != channels.cend(); ++it) {
        const QString label = mixer->getSpecificLabelForFunction(it.key());
        items.append({label.isEmpty() ? tr("Function %1").arg(it.key()) : label,
                      it.value().join(QStringLiteral(", ")), VehicleSummaryItem::Normal, groupTitle(it.key())});
    }

    return items;
}
