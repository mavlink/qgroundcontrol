#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "GPSCommandTransaction.h"

/// Resolves the outstanding command from one decoded reply; Pending keeps waiting.
using GPSReplyMatcher = std::function<GPSCommandOutcome(std::string_view reply)>;

/// Receiver configuration as ordered steps, run by GPSProtocol::runSequence(). Each command attempt is one
/// GPSProtocol::transact(), so bytes, deadlines and evidence are those of a hand-written transact() call.
struct GPSConfigurationSequence
{
    /// Matched in the raw received bytes, for replies that need not be complete frames (see GPSRawAckMatcher).
    struct RawReply
    {
        std::string accepted;
        std::string rejected;
    };

    struct Command
    {
        /// Evidence label and per-attempt timeout. A required command that fails ends the sequence.
        GPSConfigurationStep step;
        std::string wire;
        /// A matcher receives the replies the protocol decoder offers through GPSProtocol::offerReply().
        std::variant<GPSReplyMatcher, RawReply> reply;
        unsigned attempts = 1;
    };

    /// Escape hatch for receiver behaviour that is not one command and its reply.
    struct Custom
    {
        std::string label;
        std::function<bool()> run;
        bool required = true;
    };

    struct Result
    {
        /// The failed required step, or the step during which an I/O failure ended the sequence.
        std::optional<size_t> failedStep = std::nullopt;
        std::string failedLabel;
        /// The failed command's last outcome, the I/O failure's outcome, or Pending for a failed custom step.
        GPSCommandOutcome outcome = GPSCommandOutcome::Pending;

        bool succeeded() const { return !failedStep.has_value(); }
    };

    std::vector<std::variant<Command, Custom>> steps;
};
