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
class Parser;
} // namespace parser
} // namespace events

/// Event definitions read from an events metadata file, for EventHandler::setMetadata()
using EventMetadata = std::shared_ptr<events::parser::Parser>;

/// \brief Drives the MAVLink events protocol for a single component.
///
class EventHandler : public QObject
{
    Q_OBJECT

public:
    using send_request_event_message_f =
        std::function<void(const mavlink_request_event_t& msg, uint32_t targetSystemId)>;
    using handle_event_f = std::function<void(std::unique_ptr<events::parser::ParsedEvent>)>;

    EventHandler(QObject* parent, const QString& profile, handle_event_f handleEventCB,
                 send_request_event_message_f sendRequestCB, quint32 ourSystemId, uint8_t ourComponentId,
                 quint32 systemId, uint8_t componentId);
    ~EventHandler() override;

    void handleEvents(const mavlink_message_t &message);

    /// Reads and parses an events metadata file, decompressed if it is compressed and held to
    /// Compression::kMaxSmallFileBytes. It uses nothing but its argument, so that it can run on a worker thread.
    /// Null, after a warning, if the file cannot be read or parsed.
    static EventMetadata loadMetadata(const QString& metadataJsonFileName);

    /// Uses the definitions of @p metadata from now on, in place of any earlier ones, and handles the events that
    /// came before. Null @p metadata leaves the definitions as they are.
    void setMetadata(EventMetadata metadata);

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
