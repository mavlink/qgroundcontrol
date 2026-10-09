#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <QtCore/QByteArray>
#include <QtCore/QByteArrayView>

#include "GPSStreamDemux.h"

/// Sticky failure of the current receiver session; I/O and control stop until the next configure().
enum class GPSProtocolError : uint8_t
{
    None,
    Cancelled,        ///< A stop was requested; not a receiver or link fault.
    Transport,        ///< The link could not read, write, or change its baud rate.
    Protocol,         ///< The receiver violated or rejected the control protocol.
    InvalidArgument,  ///< The driver requested an invalid write, or received before configuring.
    ConsentRequired,  ///< Configuration needs a persistent receiver change the caller did not allow.
};

enum class [[nodiscard]] GPSCommandOutcome
{
    Pending,
    Written,
    Acknowledged,
    ReadbackVerified,
    Rejected,
    TimedOut,
    Cancelled,
    TransportError,
};

/// Evidence from one configuration command, not a claim of physical receiver operation.
struct [[nodiscard]] GPSConfigurationEvidence
{
    QByteArray command;
    GPSCommandOutcome outcome = GPSCommandOutcome::Pending;
    /// When the attempt began; its deadline counts from here.
    uint64_t startedAtUs = 0;
    int acceptedBytes = 0;
    int writtenBytes = 0;
    bool required = true;

    [[nodiscard]] bool succeeded() const
    {
        return outcome == GPSCommandOutcome::Acknowledged || outcome == GPSCommandOutcome::ReadbackVerified;
    }
};

struct GPSConfigurationStep
{
    QByteArray command;
    std::chrono::milliseconds timeout;
    bool required = true;
};

/// Resolves the outstanding command from one text reply a decoder offers; Pending keeps waiting.
using GPSTextMatcher = std::function<GPSCommandOutcome(std::string_view reply)>;
/// Resolves the outstanding command from each dispatched frame, after the family decoded it; Pending keeps waiting.
using GPSFrameMatcher = std::function<GPSCommandOutcome(const GPSFrame& frame)>;
/// Polled after each decoded chunk, for replies a family tracks in its own decoder state; Pending keeps waiting.
using GPSReplyPoll = std::function<GPSCommandOutcome()>;

/// Bounded raw-byte matching for receivers whose replies need not be complete ASCII lines.
class GPSRawAckMatcher
{
public:
    GPSRawAckMatcher(QByteArrayView accepted, QByteArrayView rejected)
        : _accepted(accepted)
        , _rejected(rejected)
    {}

    bool valid() const
    {
        return !_accepted.empty() && _accepted.size() <= _window.size() && _rejected.size() <= _window.size();
    }

    void append(std::span<const uint8_t> bytes)
    {
        if (!valid()) {
            return;
        }
        for (const uint8_t byte : bytes) {
            if (_size == _window.size()) {
                std::move(_window.begin() + 1, _window.end(), _window.begin());
                --_size;
            }
            _window[_size++] = static_cast<char>(byte);
            const std::string_view received(_window.data(), _size);
            if (!_rejected.empty() && received.ends_with(_rejected)) {
                _outcome = GPSCommandOutcome::Rejected;
            } else if (_outcome != GPSCommandOutcome::Rejected && received.ends_with(_accepted)) {
                _outcome = GPSCommandOutcome::Acknowledged;
            }
        }
    }

    GPSCommandOutcome outcome() const { return _outcome; }

private:
    std::string_view _accepted;
    std::string_view _rejected;
    std::array<char, 256> _window{};
    size_t _size = 0;
    GPSCommandOutcome _outcome = GPSCommandOutcome::Pending;
};

/// Receiver configuration as ordered data, run by GPSCommandChannel::runSequence(). Each command attempt is one
/// transact(), so its bytes, deadline and evidence are those of a hand-written transact().
struct GPSCommandSequence
{
    /// Matched in the raw received bytes, for replies that need not be complete frames (see GPSRawAckMatcher).
    struct RawReply
    {
        QByteArray accepted{};
        QByteArray rejected{};
    };

    struct Command
    {
        /// Evidence label and per-attempt timeout. A required command that fails ends the sequence.
        GPSConfigurationStep step{};
        QByteArray wire{};
        std::variant<GPSTextMatcher, RawReply> reply{};
        unsigned attempts = 1;
    };

    struct [[nodiscard]] Result
    {
        /// The failed required step, or the step during which a sticky failure ended the sequence.
        std::optional<size_t> failedStep = std::nullopt;
        QByteArray failedLabel{};
        /// The failed command's last outcome.
        GPSCommandOutcome outcome = GPSCommandOutcome::Pending;

        [[nodiscard]] bool succeeded() const { return !failedStep.has_value(); }
    };

    std::vector<Command> steps{};
};

struct GPSCommandOptions
{
    bool required = true;
    /// Names the command in evidence instead of its wire text, such as when the text carries base coordinates.
    QByteArray label{};
};

/// One sequence command that writes @a wire and waits up to @a timeout for @a reply. Evidence names it by its label,
/// or by @a wire without its line ending.
[[nodiscard]] inline GPSCommandSequence::Command gpsCommand(
    QByteArray wire, std::chrono::milliseconds timeout,
    std::variant<GPSTextMatcher, GPSCommandSequence::RawReply> reply, GPSCommandOptions options = {})
{
    GPSConfigurationStep step{.command = options.label.isEmpty() ? wire.trimmed() : std::move(options.label),
                              .timeout = timeout,
                              .required = options.required};
    return {.step = std::move(step), .wire = std::move(wire), .reply = std::move(reply)};
}
