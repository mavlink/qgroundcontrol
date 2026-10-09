#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "GPSCommand.h"
#include "UBX/UBXConfigKeys.h"
#include "UBX/UBXFrame.h"

namespace UBX {

struct Acknowledgement
{
    uint16_t message;
    bool accepted;
};

/// The CFG-MSG output rates of one message on each I/O port, as a poll returns them.
struct MessageRates
{
    uint16_t message;
    std::array<uint8_t, 6> rates;
};

/// Owns control-response correlation: decoding reports replies through accept(); only configuration opens requests.
class ReceiverController
{
public:
    /// A baud change can lose its ACK on UART; subsequent VALSETs must prove their values by readback.
    void requireConfigurationReadback(uint16_t message) { _unacknowledgedConfiguration = message; }

    bool configurationReadbackRequired() const { return _unacknowledgedConfiguration.has_value(); }

    void beginAcknowledgement(uint16_t message)
    {
        _lastMessage.reset();
        _lateRejection = false;
        _awaitingMessage = message;
        _acknowledgement = GPSCommandOutcome::Pending;
    }

    void finishAcknowledgement()
    {
        _lastMessage = _awaitingMessage;
        _awaitingMessage.reset();
    }

    bool lateRejection() const { return _lateRejection; }

    bool awaitingAcknowledgement() const { return _awaitingMessage.has_value(); }

    GPSCommandOutcome acknowledgement() const { return _acknowledgement; }

    void accept(Acknowledgement response)
    {
        if (!response.accepted &&
            (_lastMessage == response.message || _unacknowledgedConfiguration == response.message)) {
            _lateRejection = true;
        }
        if (_awaitingMessage == response.message && _acknowledgement != GPSCommandOutcome::Rejected) {
            _acknowledgement = response.accepted ? GPSCommandOutcome::Acknowledged : GPSCommandOutcome::Rejected;
        }
        if (response.message == Msg::CFG_VALGET.value() && !response.accepted) {
            // A VALGET with an unknown key is rejected, so the VALSET with it was rejected too.
            if (_takeReadbackReply()) {
                _readbackRejected = true;
            }
        }
        if (_ratePoll && response.message == Msg::CFG_MSG.value()) {
            if (_ratePoll->rates) {
                _ratePoll->acknowledged = true;
            } else {
                ++_ratePoll->repliesBeforeRates;
                _ratePoll->rejected = _ratePoll->rejected || !response.accepted;
            }
        }
    }

    void beginReadback(std::span<const uint32_t> keys)
    {
        _readback = {};
        _readbackPending = !keys.empty() && keys.size() <= _readback.values.size();
        _readbackReady = false;
        _readbackRejected = false;
        _readbackReplied = false;
        if (_readbackPending) {
            _readback.count = keys.size();
            for (size_t index = 0; index < keys.size(); ++index) {
                _readback.values[index].key = keys[index];
            }
        }
    }

    void finishReadback()
    {
        // A readback settled by an earlier reply, or by its timeout, still has its own reply to come.
        if (_readbackPending && !_readbackReplied) {
            _readbackReplyOutstanding = true;
        }
        _readbackPending = false;
    }

    bool readbackPending() const { return _readbackPending; }

    /// A reply to a readback settled without it can still arrive.
    bool readbackReplyOutstanding() const { return _readbackReplyOutstanding; }

    bool readbackReady() const { return _readbackReady; }

    /// The receiver rejected the pending readback: at least one of its keys is unknown.
    bool readbackRejected() const { return _readbackRejected; }

    const ConfigurationValues& readback() const { return _readback; }

    /// A CFG-VALGET response; nullopt when it is malformed.
    void accept(const std::optional<ConfigurationValues>& response)
    {
        // Values name their keys, so they are matched whichever poll they answer.
        _takeReadbackReply();
        if (!_readbackPending || !response || response->count != _readback.count) {
            return;
        }
        std::array<ConfigurationValue, ConfigurationValues::MAX_KEYS> values{};
        uint16_t seen = 0;
        for (size_t entry = 0; entry < response->count; ++entry) {
            size_t index = 0;
            while (index < _readback.count && _readback.values[index].key != response->values[entry].key) {
                ++index;
            }
            if (index == _readback.count || (seen & (1u << index))) {
                return;
            }
            values[index] = response->values[entry];
            seen |= 1u << index;
        }
        _readback.values = values;
        _readbackReady = true;
        _readbackReplied = true;
    }

    /// Attributes replies to a CFG-MSG poll of @a message, sent after a rate of it went unanswered. Replies arrive in
    /// order and the poll's rates name their message: an ACK or NAK before the rates answers the rate or rejects the
    /// poll, and the one after them acknowledges the poll.
    void beginRatePoll(uint16_t message) { _ratePoll = RatePoll{.message = message}; }

    void finishRatePoll() { _ratePoll.reset(); }

    bool ratePollPending() const { return _ratePoll.has_value(); }

    /// The polled rates, once they arrived.
    std::optional<std::array<uint8_t, 6>> polledRates() const { return _ratePoll ? _ratePoll->rates : std::nullopt; }

    /// A NAK came before the polled rates: the receiver rejected the rate or the poll.
    bool ratePollRejected() const { return _ratePoll && _ratePoll->rejected; }

    /// Nothing more is to arrive for the rate or its poll: the poll's ACK followed its rates, or the rate's late reply
    /// came before the poll's NAK.
    bool ratePollSettled() const
    {
        return _ratePoll && ((_ratePoll->rates && _ratePoll->acknowledged) || _ratePoll->repliesBeforeRates >= 2);
    }

    void accept(const MessageRates& response)
    {
        if (_ratePoll && response.message == _ratePoll->message && !_ratePoll->rates) {
            _ratePoll->rates = response.rates;
        }
    }

private:
    struct RatePoll
    {
        uint16_t message = 0;
        std::optional<std::array<uint8_t, 6>> rates{};
        unsigned repliesBeforeRates = 0;
        bool rejected = false;
        bool acknowledged = false;
    };

    /// Takes one readback reply, a response or a NAK. @return whether it answers the pending readback rather than
    /// one settled without its reply.
    bool _takeReadbackReply()
    {
        if (std::exchange(_readbackReplyOutstanding, false) || !_readbackPending) {
            return false;
        }
        _readbackReplied = true;
        return true;
    }

    std::optional<uint16_t> _unacknowledgedConfiguration;
    std::optional<uint16_t> _awaitingMessage;
    std::optional<uint16_t> _lastMessage;
    bool _lateRejection = false;
    GPSCommandOutcome _acknowledgement = GPSCommandOutcome::Pending;
    ConfigurationValues _readback;
    bool _readbackPending = false;
    bool _readbackReady = false;
    bool _readbackRejected = false;
    bool _readbackReplied = false;
    bool _readbackReplyOutstanding = false;
    std::optional<RatePoll> _ratePoll;
};

}  // namespace UBX
