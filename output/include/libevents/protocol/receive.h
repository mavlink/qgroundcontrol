#pragma once

#include "../common/event_type.h"
#ifndef MAVLINK_MSG_ID_EVENT
#error "The mavlink header <mavlink.h> needs to be included before including this header"
#endif

#ifndef LIBEVENTS_DEBUG_PRINTF
#define LIBEVENTS_DEBUG_PRINTF(...)
#endif

#include <cstdint>
#include <functional>

namespace events
{
/**
 * @class ReceiveProtocol
 * Handles the MAVLink events protocol for receiving events. There should be one
 * instance per MAVLink source component id.
 */
class ReceiveProtocol
{
public:
    struct Callbacks {
        std::function<void(int num_events_lost)> error;  ///< lost events
        std::function<void(const mavlink_request_event_t&)> send_request_event_message;
        std::function<void(const mavlink_event_t&)> handle_event;
        std::function<void(int timeout_ms)> timer_control;  ///< control timer (single shot), timeout_ms <0 means to
                                                            ///< disable the timer
    };

    ReceiveProtocol(const Callbacks& callbacks, uint8_t our_system_id, uint8_t our_component_id, uint8_t system_id,
                    uint8_t component_id)
        : _callbacks(callbacks),
          _our_system_id(our_system_id),
          _our_component_id(our_component_id),
          _system_id(system_id),
          _component_id(component_id)
    {
    }

    void processMessage(const mavlink_message_t& msg)
    {
        switch (msg.msgid) {
            case MAVLINK_MSG_ID_EVENT: handleEvent(msg); break;
            case MAVLINK_MSG_ID_CURRENT_EVENT_SEQUENCE: handleCurrentEventSequence(msg); break;
            case MAVLINK_MSG_ID_RESPONSE_EVENT_ERROR: handleEventError(msg); break;
        }
    }

    void timerEvent()
    {
        LIBEVENTS_DEBUG_PRINTF("timeout, re-requesting seq=%i\n", _latest_sequence + 1);
        requestEvent(_latest_sequence + 1);
    }

private:
    enum class SequenceComparison { older = -1, equal = 0, newer = 1 };

    void handleEvent(const mavlink_message_t& message)
    {
        mavlink_event_t event_msg;
        mavlink_msg_event_decode(&message, &event_msg);

        if (_component_id != message.compid) {
            // If this happens, the ReceiveProtocol instance is used wrong
            LIBEVENTS_DEBUG_PRINTF("got unexpeced component id (%i != %i)\n", _component_id, message.compid);
            return;
        }

        // check for vehicle reboot (resets the sequence if necessary)
        checkTimestampReset(event_msg.event_time_boot_ms);

        if (!_has_sequence) {
            _has_sequence = true;
            _latest_sequence = event_msg.sequence - 1;
        }

        LIBEVENTS_DEBUG_PRINTF("Incoming event: last seq=%i, msg seq=%i, id=%08x\n", _latest_sequence,
                               event_msg.sequence, event_msg.id);

        switch (compareSequence(_latest_sequence + 1, event_msg.sequence)) {
            case SequenceComparison::older:  // duplicate: discard
                LIBEVENTS_DEBUG_PRINTF("Dropping duplicate event\n");
                return;
            case SequenceComparison::equal:  // all good
                _latest_sequence = event_msg.sequence;
                break;
            case SequenceComparison::newer:  // dropped events: re-request expected event
                requestEvent(_latest_sequence + 1);
                // TODO: buffer this event for later processing? For now we just drop it and we'll re-request it later
                return;
        }
        _last_timestamp_ms = event_msg.event_time_boot_ms;

        if (_callbacks.timer_control) {
            _callbacks.timer_control(-1);  // turn off timer
        }

        // need to request more events?
        if (_has_current_sequence) {
            if (compareSequence(_latest_sequence, _latest_current_sequence) == SequenceComparison::newer) {
                requestEvent(_latest_sequence + 1);
            }
        }

        // ignore events that are not for us
        if (event_msg.destination_system != _our_system_id && event_msg.destination_system != 0) {
            LIBEVENTS_DEBUG_PRINTF("Ignoring event not for us (sys id: %i != %i)\n", event_msg.destination_system,
                                   _our_system_id);
            return;
        }
        if (event_msg.destination_component != _our_component_id &&
            event_msg.destination_component != MAV_COMP_ID_ALL) {
            LIBEVENTS_DEBUG_PRINTF("Ignoring event not for us (comp id: %i != %i)\n", event_msg.destination_component,
                                   _our_component_id);
            return;
        }

        _callbacks.handle_event(event_msg);
    }

    /**
     * Compare 2 sequence numbers with wrap-around handling.
     * @return 'equal' if equal, 'older' if incoming is old (duplicate), 'newer' if incoming is newer (dropped events)
     */
    SequenceComparison compareSequence(uint16_t expected_sequence, uint16_t incoming_sequence)
    {
        if (expected_sequence == incoming_sequence) {
            return SequenceComparison::equal;
        }

        // this handles warp-arounds correctly
        const uint16_t diff = incoming_sequence - expected_sequence;
        if (diff > UINT16_MAX / 2) {
            return SequenceComparison::older;
        }
        return SequenceComparison::newer;
    }

    void requestEvent(uint16_t sequence)
    {
        mavlink_request_event_t msg{};
        msg.target_system = _system_id;
        msg.target_component = _component_id;
        msg.first_sequence = msg.last_sequence = sequence;

        LIBEVENTS_DEBUG_PRINTF("requesting seq %i\n", sequence);

        _callbacks.send_request_event_message(msg);
        // start a timer (or reset existing timer)
        if (_callbacks.timer_control) {
            _callbacks.timer_control(100);
        }
    }

    void handleCurrentEventSequence(const mavlink_message_t& message)
    {
        mavlink_current_event_sequence_t event_sequence;
        mavlink_msg_current_event_sequence_decode(&message, &event_sequence);

        if (event_sequence.flags & MAV_EVENT_CURRENT_SEQUENCE_FLAGS_RESET) {
            LIBEVENTS_DEBUG_PRINTF("current sequence: reset flag set\n");
            _has_sequence = false;
        }
        if (!_has_sequence) {
            _has_sequence = true;
            _latest_sequence = event_sequence.sequence;
        }

        if (compareSequence(_latest_sequence, event_sequence.sequence) == SequenceComparison::newer) {
            requestEvent(_latest_sequence + 1);
        }
        _has_current_sequence = true;
        _latest_current_sequence = event_sequence.sequence;
    }

    void handleEventError(const mavlink_message_t& message)
    {
        mavlink_response_event_error_t event_error;
        mavlink_msg_response_event_error_decode(&message, &event_error);

        if (event_error.target_system != _our_system_id || event_error.target_component != _our_component_id) {
            return;
        }

        if (compareSequence(_latest_sequence + 1, event_error.sequence) != SequenceComparison::equal) {
            // not a response to our requested sequence number, or we already got the event meanwhile
            return;
        }

        // here we know that we dropped one or more events
        uint16_t num_events_lost = event_error.sequence_oldest_available - _latest_sequence - 1;
        _callbacks.error(num_events_lost);

        // request oldest known event (which resets the timer as well)
        _latest_sequence = event_error.sequence_oldest_available - 1;
        requestEvent(_latest_sequence + 1);
    }

    void checkTimestampReset(uint32_t timestamp)
    {
        if (_last_timestamp_ms == 0) {
            _last_timestamp_ms = timestamp;
        }
        // detect vehicle reboot based on timestamp with some margin and conservative wrap-around
        // handling (in case we missed the current sequence with the reset flag set)
        if (timestamp + 10000 < _last_timestamp_ms && _last_timestamp_ms < UINT32_MAX - 60000) {
            LIBEVENTS_DEBUG_PRINTF("sequence reset based on timestamp\n");
            _has_sequence = false;
            _has_current_sequence = false;
        }
    }

    const Callbacks _callbacks;

    uint16_t _latest_sequence;  ///< latest received sequence number
    bool _has_sequence{false};
    uint32_t _last_timestamp_ms{0};

    bool _has_current_sequence{false};
    uint32_t _latest_current_sequence;  ///< latest received sequence number via mavlink_current_event_sequence_t

    uint8_t _our_system_id;
    uint8_t _our_component_id;

    uint8_t _system_id;
    uint8_t _component_id;
};

} /* namespace events */
