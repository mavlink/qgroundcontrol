#include "EventHandler.h"
#include "QGCLoggingCategory.h"
#include "LibEvents.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QTimer>
#include <QtCore/QVector>

#include <utility>

QGC_LOGGING_CATEGORY(EventHandlerLog, "MAVLink.LibEvents.EventHandler")

struct EventHandler::Impl
{
    Impl(EventHandler *q_,
         const QString &profile,
         handle_event_f handleEventCB_,
         send_request_event_message_f sendRequestCB_,
         uint8_t ourSystemId, uint8_t ourComponentId,
         uint8_t systemId, uint8_t componentId)
        : q(q_)
        , timer(q_)
        , handleEventCB(std::move(handleEventCB_))
        , sendRequestCB(std::move(sendRequestCB_))
        , compid(componentId)
    {
        auto error_cb = [this](int num_events_lost) {
            healthAndArmingChecks.reset();
            qCWarning(EventHandlerLog) << "Events got lost:" << num_events_lost
                                       << "comp_id:" << compid;
        };

        auto timeout_cb = [this](int timeout_ms) {
            if (timeout_ms < 0) {
                timer.stop();
            } else {
                timer.setSingleShot(true);
                timer.start(timeout_ms);
            }
        };

        parser.setProfile(profile.toStdString());
        parser.formatters().url = [](const std::string &content, const std::string &link) {
            return "<a href=\"" + link + "\">" + content + "</a>";
        };
        parser.formatters().param = [](const std::string &content) {
            return "<a href=\"param://" + content + "\">" + content + "</a>";
        };
        parser.formatters().escape = [](const std::string &str) {
            return QString::fromStdString(str).toHtmlEscaped().toStdString();
        };

        events::ReceiveProtocol::Callbacks callbacks{
            error_cb,
            sendRequestCB,
            std::bind(&Impl::gotEvent, this, std::placeholders::_1),
            timeout_cb};
        protocol = new events::ReceiveProtocol(callbacks, ourSystemId, ourComponentId,
                                               systemId, componentId);

        QObject::connect(&timer, &QTimer::timeout, q, [this]() { protocol->timerEvent(); });
    }

    ~Impl() { delete protocol; }

    void gotEvent(const mavlink_event_t &event);

    EventHandler *q;
    events::ReceiveProtocol *protocol{nullptr};
    QTimer timer;
    events::parser::Parser parser;
    events::HealthAndArmingChecks healthAndArmingChecks;
    bool healthAndArmingChecksValid{false};
    QVector<mavlink_event_t> pendingEvents; ///< stores incoming events until we have the metadata loaded
    handle_event_f handleEventCB;
    send_request_event_message_f sendRequestCB;
    const uint8_t compid;
};

void EventHandler::Impl::gotEvent(const mavlink_event_t &event)
{
    if (!parser.hasDefinitions()) {
        if (pendingEvents.size() > 50) { // limit size (not expected to happen)
            pendingEvents.clear();
        }
        qCDebug(EventHandlerLog) << "No metadata, queuing event, ID:" << event.id
                                 << "num pending:" << pendingEvents.size();
        pendingEvents.push_back(event);
        return;
    }

    std::unique_ptr<events::parser::ParsedEvent> parsed_event = parser.parse(events::EventType(event));
    if (parsed_event == nullptr) {
        qCWarning(EventHandlerLog) << "Got Event w/o known metadata: ID:" << event.id
                                   << "comp id:" << compid;
        return;
    }

    qCDebug(EventHandlerLog) << "Got Event: ID:" << parsed_event->id()
                             << "namespace:" << parsed_event->eventNamespace().c_str()
                             << "name:" << parsed_event->name().c_str()
                             << "msg:" << parsed_event->message().c_str();

    if (healthAndArmingChecks.handleEvent(*parsed_event)) {
        healthAndArmingChecksValid = true;
        emit q->healthAndArmingChecksUpdated();
    }
    handleEventCB(std::move(parsed_event));
}

/*===========================================================================*/

EventHandler::EventHandler(QObject *parent,
                           const QString &profile,
                           handle_event_f handleEventCB,
                           send_request_event_message_f sendRequestCB,
                           uint8_t ourSystemId, uint8_t ourComponentId,
                           uint8_t systemId, uint8_t componentId)
    : QObject(parent)
    , _impl(std::make_unique<Impl>(this, profile,
                                   std::move(handleEventCB),
                                   std::move(sendRequestCB),
                                   ourSystemId, ourComponentId,
                                   systemId, componentId))
{
}

EventHandler::~EventHandler() = default;

void EventHandler::handleEvents(const mavlink_message_t &message)
{
    _impl->protocol->processMessage(message);
}

void EventHandler::setMetadata(const QString &metadataJsonFileName)
{
    if (_impl->parser.loadDefinitionsFile(metadataJsonFileName.toStdString())) {
        if (_impl->parser.hasDefinitions()) {
            // Flush queued events now that metadata is available.
            for (const auto &event : _impl->pendingEvents) {
                _impl->gotEvent(event);
            }
            _impl->pendingEvents.clear();
        }
    } else {
        qCWarning(EventHandlerLog) << "Failed to load events JSON metadata file";
    }
}

const events::HealthAndArmingChecks *EventHandler::healthAndArmingChecks() const
{
    return &_impl->healthAndArmingChecks;
}

bool EventHandler::healthAndArmingCheckResultsValid() const
{
    return _impl->healthAndArmingChecksValid;
}

int EventHandler::getModeGroup(int32_t customMode) const
{
    events::parser::Parser::NavigationModeGroups groups = _impl->parser.navigationModeGroups(_impl->compid);
    for (const auto &groupIter : groups.groups) {
        if (groupIter.second.find(customMode) != groupIter.second.end()) {
            return groupIter.first;
        }
    }
    return -1;
}

bool EventHandler::healthAndArmingChecksSupported() const
{
    const auto &protocols = _impl->parser.supportedProtocols(_impl->compid);
    return protocols.find("health_and_arming_check") != protocols.end();
}
