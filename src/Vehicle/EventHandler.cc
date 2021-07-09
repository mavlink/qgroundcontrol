/****************************************************************************
 *
 * (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QSharedPointer>

#include "EventHandler.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_METATYPE(QSharedPointer<events::parser::ParsedEvent>);


EventHandler::EventHandler(QObject* parent, const QString& profile, handle_event_f handleEventCB,
            send_request_event_message_f sendRequestCB,
            uint8_t ourSystemId, uint8_t ourComponentId, uint8_t systemId, uint8_t componentId)
    : QObject(parent), _timer(parent),
    _handleEventCB(handleEventCB),
    _sendRequestCB(sendRequestCB),
    _compid(componentId)
{
    auto error_cb = [componentId](int num_events_lost) {
        qCWarning(EventsLog) << "Events got lost:" << num_events_lost << "comp_id:" << componentId;
    };

    auto timeout_cb = [this](int timeout_ms) {
        if (timeout_ms < 0) {
            _timer.stop();
        } else {
            _timer.setSingleShot(true);
            _timer.start(timeout_ms);
        }
    };

    _parser.setProfile(profile.toStdString());

    _parser.formatters().url = [](const std::string& content, const std::string& link) {
        return "<a href=\""+link+"\">"+content+"</a>"; };

    events::ReceiveProtocol::Callbacks callbacks{error_cb, _sendRequestCB,
        std::bind(&EventHandler::gotEvent, this, std::placeholders::_1), timeout_cb};
    _protocol = new events::ReceiveProtocol(callbacks, ourSystemId, ourComponentId, systemId, componentId);

    connect(&_timer, &QTimer::timeout, this, [this]() { _protocol->timerEvent(); });

    qRegisterMetaType<QSharedPointer<events::parser::ParsedEvent>>("ParsedEvent");
}

EventHandler::~EventHandler()
{
    delete _protocol;
}

void EventHandler::gotEvent(const mavlink_event_t& event)
{
    if (!_parser.hasDefinitions()) {
        if (_pendingEvents.size() > 50) { // limit size (not expected to happen)
            _pendingEvents.clear();
        }
        qCDebug(EventsLog) << "No metadata, queuing event, ID:" << event.id << "num pending:" << _pendingEvents.size();
        _pendingEvents.push_back(event);
        return;
    }

    std::unique_ptr<events::parser::ParsedEvent> parsed_event = _parser.parse(event);
    if (parsed_event == nullptr) {
        qCWarning(EventsLog) << "Got Event w/o known metadata: ID:" << event.id << "comp id:" << _compid;
        return;
    }

    qCDebug(EventsLog) << "Got Event: ID:" << parsed_event->id() << "namespace:" << parsed_event->eventNamespace().c_str() <<
            "name:" << parsed_event->name().c_str() << "msg:" << parsed_event->message().c_str();

    _handleEventCB(std::move(parsed_event));
}

void EventHandler::handleEvents(const mavlink_message_t& message)
{
    _protocol->processMessage(message);
}

void EventHandler::setMetadata(const QString &metadataJsonFileName, const QString &translationJsonFileName)
{
    auto translate = [](const std::string& s) {
        // TODO: use translation file
        return s;
    };
    if (_parser.loadDefinitionsFile(metadataJsonFileName.toStdString(), translate)) {
        if (_parser.hasDefinitions()) {
            // do we have queued events?
            for (const auto& event : _pendingEvents) {
                gotEvent(event);
            }
            _pendingEvents.clear();
        }
    } else {
        qCWarning(EventsLog) << "Failed to load events JSON metadata file";
    }
}

