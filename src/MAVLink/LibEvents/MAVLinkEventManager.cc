#include "MAVLinkEventManager.h"

#include "EventHandler.h"
#include "FirmwarePlugin.h"
#include "HealthAndArmingCheckReport.h"
#include "MAVLinkProtocol.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"
#include "VehicleLinkManager.h"
#include "LibEvents.h"

#include <functional>
#include <string>
#include <utility>

QGC_LOGGING_CATEGORY(MAVLinkEventManagerLog, "MAVLink.LibEvents.EventManager")

MAVLinkEventManager::MAVLinkEventManager(Vehicle *vehicle)
    : QObject(vehicle)
    , _vehicle(vehicle)
    , _healthAndArmingCheckReport(std::make_unique<HealthAndArmingCheckReport>(vehicle))
{
    // Re-evaluate the report whenever the vehicle's flight mode changes so the
    // "can arm for the current mode" derivation stays fresh.
    connect(_vehicle, &Vehicle::flightModeChanged, this, [this]() {
        for (auto it = _events.begin(); it != _events.end(); ++it) {
            if (it.value()->healthAndArmingCheckResultsValid()) {
                _updateHealthReport(it.key());
            }
        }
    });
}

MAVLinkEventManager::~MAVLinkEventManager() = default;

HealthAndArmingCheckReport *MAVLinkEventManager::healthAndArmingCheckReport() const
{
    return _healthAndArmingCheckReport.get();
}

void MAVLinkEventManager::handleEventMessage(const mavlink_message_t &message)
{
    _eventHandlerForCompId(message.compid).handleEvents(message);
}

bool MAVLinkEventManager::healthAndArmingChecksSupported(uint8_t compid) const
{
    const auto it = _events.find(compid);
    if (it == _events.end()) {
        return false;
    }
    return it.value()->healthAndArmingChecksSupported();
}

void MAVLinkEventManager::setMetadata(uint8_t compid, const QString &metadataJsonFileName)
{
    EventHandler &handler = _eventHandlerForCompId(compid);
    handler.setMetadata(metadataJsonFileName);

    // Resolve mode groups for the well-known "takeoff" and "mission" flight
    // modes. These feed the report's canTakeoff / canStartMission derivations.
    int modeGroups[2]{-1, -1};
    FirmwarePlugin *firmwarePlugin = _vehicle->firmwarePlugin();
    const QString modes[2]{firmwarePlugin->takeOffFlightMode(), _vehicle->missionFlightMode()};
    for (size_t i = 0; i < std::size(modeGroups); ++i) {
        uint8_t base_mode{};
        uint32_t custom_mode{};
        if (firmwarePlugin->setFlightMode(modes[i], &base_mode, &custom_mode)) {
            modeGroups[i] = handler.getModeGroup(custom_mode);
            if (modeGroups[i] == -1) {
                qCDebug(MAVLinkEventManagerLog) << "Failed to get mode group for mode"
                                                << modes[i]
                                                << "(Might not be in metadata)";
            }
        }
    }
    _healthAndArmingCheckReport->setModeGroups(modeGroups[0], modeGroups[1]);
}

EventHandler &MAVLinkEventManager::_eventHandlerForCompId(uint8_t compid)
{
    auto eventData = _events.find(compid);
    if (eventData != _events.end()) {
        return *eventData->data();
    }

    // Send mavlink REQUEST_EVENT on behalf of the protocol state machine.
    auto sendRequestEventMessageCB = [this](const mavlink_request_event_t &msg) {
        SharedLinkInterfacePtr sharedLink = _vehicle->vehicleLinkManager()->primaryLink().lock();
        if (!sharedLink) {
            return;
        }
        mavlink_message_t message;
        mavlink_msg_request_event_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                              MAVLinkProtocol::getComponentId(),
                                              sharedLink->mavlinkChannel(),
                                              &message,
                                              &msg);
        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
    };

    const QString profile = QStringLiteral("dev"); // TODO: should be configurable

    QSharedPointer<EventHandler> eventHandler{new EventHandler(
        this,
        profile,
        std::bind(&MAVLinkEventManager::_handleEvent, this, compid, std::placeholders::_1),
        sendRequestEventMessageCB,
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        _vehicle->id(),
        compid)};
    eventData = _events.insert(compid, eventHandler);

    connect(eventHandler.data(), &EventHandler::healthAndArmingChecksUpdated, this, [this, compid]() {
        _updateHealthReport(compid);
    });

    return *eventData->data();
}

void MAVLinkEventManager::_updateHealthReport(uint8_t compid)
{
    const auto it = _events.find(compid);
    if (it == _events.end()) {
        return;
    }
    const uint32_t effectiveMode = _vehicle->effectiveCustomMode();
    _healthAndArmingCheckReport->update(compid, *it.value(), it.value()->getModeGroup(effectiveMode));
}

void MAVLinkEventManager::_handleEvent(uint8_t comp_id, std::unique_ptr<events::parser::ParsedEvent> event)
{
    int severity = -1;
    switch (events::externalLogLevel(event->eventData().log_levels)) {
        case events::Log::Emergency: severity = MAV_SEVERITY_EMERGENCY; break;
        case events::Log::Alert:     severity = MAV_SEVERITY_ALERT;     break;
        case events::Log::Critical:  severity = MAV_SEVERITY_CRITICAL;  break;
        case events::Log::Error:     severity = MAV_SEVERITY_ERROR;     break;
        case events::Log::Warning:   severity = MAV_SEVERITY_WARNING;   break;
        case events::Log::Notice:    severity = MAV_SEVERITY_NOTICE;    break;
        case events::Log::Info:      severity = MAV_SEVERITY_INFO;      break;
        default: break;
    }

    // Health / arming-check events are rendered via HealthAndArmingCheckReport, not as raw status text.
    if (event->group() == "health" || event->group() == "arming_check") {
        return;
    }
    if (event->group() == "calibration") {
        qCDebug(MAVLinkEventManagerLog) << "Calibration Event Receieved";
        return;
    }

    // Only default-group events at a known severity map to status text.
    if (event->group() != "default" || severity == -1) {
        return;
    }

    std::string message = event->message();
    std::string description = event->description();

    if (event->type() == "append_health_and_arming_messages" && event->numArguments() > 0) {
        const uint32_t customMode = event->argumentValue(0).value.val_uint32_t;
        EventHandler &handler = _eventHandlerForCompId(comp_id);
        const int modeGroup = handler.getModeGroup(customMode);
        const std::vector<events::HealthAndArmingChecks::Check> checks = handler.healthAndArmingChecks()->results().checks(modeGroup);

        std::vector<std::string> messageChecks;
        for (const auto &check : checks) {
            if (events::externalLogLevel(check.log_levels) <= events::Log::Warning) {
                messageChecks.push_back(check.message);
            }
        }
        if (messageChecks.empty()) {
            for (const auto &check : checks) {
                messageChecks.push_back(check.message);
            }
        }
        if (!message.empty() && !messageChecks.empty()) {
            message += "\n";
        }
        if (messageChecks.size() == 1) {
            message += messageChecks[0];
        } else {
            for (const auto &messageCheck : messageChecks) {
                message += "- " + messageCheck + "\n";
            }
        }
    }

    if (message.empty()) {
        return;
    }

    const QString text = QString::fromStdString(message);
    // PX4 "[cal]" messages are delivered separately via the calibration flow;
    // suppress them from the general status-text stream.
    if (_vehicle->px4Firmware() && text.startsWith(QStringLiteral("[cal]"))) {
        return;
    }

    emit statusTextMessageFromEvent(comp_id, severity, text, QString::fromStdString(description));
}
