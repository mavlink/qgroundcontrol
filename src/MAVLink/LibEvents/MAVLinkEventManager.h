#pragma once

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include <cstdint>
#include <memory>

#include "QGCMAVLinkTypes.h"

class EventHandler;
class HealthAndArmingCheckReport;
class Vehicle;

namespace events::parser {
class ParsedEvent;
}

/// Owns per-component EventHandler instances and drives the Health & Arming
/// Check report.
class MAVLinkEventManager : public QObject
{
    Q_OBJECT

public:
    explicit MAVLinkEventManager(Vehicle *vehicle);
    ~MAVLinkEventManager() override;

    HealthAndArmingCheckReport *healthAndArmingCheckReport() const;

    /// Route MAVLink event protocol messages (EVENT, CURRENT_EVENT_SEQUENCE,
    /// RESPONSE_EVENT_ERROR) to the EventHandler for the source component.
    void handleEventMessage(const mavlink_message_t &message);

    /// Load event metadata JSON for a component, then compute takeoff and
    /// mission mode groups and publish them to the report.
    void setMetadata(uint8_t compid, const QString &metadataJsonFileName);

    /// True if an EventHandler already exists for this component and it
    /// reports the health_and_arming_check protocol as supported.
    bool healthAndArmingChecksSupported(uint8_t compid) const;

signals:
    /// Emitted for "default"-group events that should render as a status-text
    /// message. Vehicle routes this into its StatusTextHandler.
    void statusTextMessageFromEvent(uint8_t compid, int severity, const QString &text, const QString &description);

private:
    EventHandler &_eventHandlerForCompId(uint8_t compid);
    void _handleEvent(uint8_t comp_id, std::unique_ptr<events::parser::ParsedEvent> event);
    void _updateHealthReport(uint8_t compid);

    Vehicle *_vehicle{nullptr};
    QMap<uint8_t, QSharedPointer<EventHandler>> _events; ///< one protocol handler per component id
    std::unique_ptr<HealthAndArmingCheckReport> _healthAndArmingCheckReport;
};
