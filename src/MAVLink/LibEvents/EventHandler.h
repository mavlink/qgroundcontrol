#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include <cstdint>
#include <functional>
#include <memory>

#include "QGCMAVLinkTypes.h"

namespace events {
class HealthAndArmingChecks;
namespace parser {
class ParsedEvent;
} // namespace parser
} // namespace events

/// Drives the MAVLink events protocol for a single component.
class EventHandler : public QObject
{
    Q_OBJECT

public:
    using send_request_event_message_f = std::function<void(const mavlink_request_event_t &msg)>;
    using handle_event_f = std::function<void(std::unique_ptr<events::parser::ParsedEvent>)>;

    EventHandler(QObject *parent,
                 const QString &profile,
                 handle_event_f handleEventCB,
                 send_request_event_message_f sendRequestCB,
                 uint8_t ourSystemId, uint8_t ourComponentId,
                 uint8_t systemId, uint8_t componentId);
    ~EventHandler() override;

    void handleEvents(const mavlink_message_t &message);
    void setMetadata(const QString &metadataJsonFileName);

    const events::HealthAndArmingChecks *healthAndArmingChecks() const;
    bool healthAndArmingCheckResultsValid() const;

    int getModeGroup(int32_t customMode) const;
    bool healthAndArmingChecksSupported() const;

signals:
    void healthAndArmingChecksUpdated();

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};
