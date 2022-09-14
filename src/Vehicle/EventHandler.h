/****************************************************************************
 *
 * (c) 2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QString>
#include <QVector>
#include <QTimer>

#include <functional>

#include <libevents/libs/cpp/protocol/receive.h>
#include <libevents/libs/cpp/parse/health_and_arming_checks.h>
#include <libevents/libs/cpp/parse/parser.h>
#include <libevents/libs/cpp/generated/events_generated.h>

class EventHandler : public QObject
{
    Q_OBJECT
public:
    using send_request_event_message_f = std::function<void(const mavlink_request_event_t& msg)>;
    using handle_event_f = std::function<void(std::unique_ptr<events::parser::ParsedEvent>)>;

    EventHandler(QObject* parent, const QString& profile, handle_event_f handleEventCB,
            send_request_event_message_f sendRequestCB,
            uint8_t ourSystemId, uint8_t ourComponentId, uint8_t systemId, uint8_t componentId);
    ~EventHandler();


    void handleEvents(const mavlink_message_t& message);

    void setMetadata(const QString& metadataJsonFileName);

    const events::HealthAndArmingChecks::Results& healthAndArmingCheckResults() const { return _healthAndArmingChecks.results(); }
    bool healthAndArmingCheckResultsValid() const { return _healthAndArmingChecksValid; }

    int getModeGroup(int32_t customMode);

    bool healthAndArmingChecksSupported() const {
        const auto& protocols = _parser.supportedProtocols(_compid);
        return protocols.find("health_and_arming_check") != protocols.end();
    }

signals:
    void healthAndArmingChecksUpdated();

private:
    void gotEvent(const mavlink_event_t& event);

    events::ReceiveProtocol* _protocol{nullptr};
    QTimer _timer;
    events::parser::Parser _parser;
    events::HealthAndArmingChecks _healthAndArmingChecks;
    bool _healthAndArmingChecksValid{false};
    QVector<mavlink_event_t> _pendingEvents; ///< stores incoming events until we have the metadata loaded
    handle_event_f _handleEventCB;
    send_request_event_message_f _sendRequestCB;
    const uint8_t _compid;
};
