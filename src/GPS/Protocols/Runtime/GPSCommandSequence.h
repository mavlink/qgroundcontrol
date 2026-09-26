#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <QtCore/QByteArray>

#include "GPSCommandTransaction.h"
#include "GPSFrame.h"
#include "GPSTask.h"

/// Resolves the outstanding command from one text reply a decoder offers; Pending keeps waiting.
using GPSTextMatcher = std::function<GPSCommandOutcome(std::string_view reply)>;
/// Resolves the outstanding command from each dispatched frame, after the family decoded it; Pending keeps waiting.
using GPSFrameMatcher = std::function<GPSCommandOutcome(const GPSFrame& frame)>;
/// Polled after each decoded chunk, for replies a family tracks in its own decoder state; Pending keeps waiting.
using GPSReplyPoll = std::function<GPSCommandOutcome()>;

/// Receiver configuration as ordered data, run by GPSCommandChannel::runSequence(). Each command attempt is one
/// transact(), so its bytes, deadline and evidence are those of a hand-written transact().
struct GPSCommandSequence
{
    /// Matched in the raw received bytes, for replies that need not be complete frames (see GPSRawAckMatcher).
    struct RawReply
    {
        std::string accepted{};
        std::string rejected{};
    };

    struct Command
    {
        /// Evidence label and per-attempt timeout. A required command that fails ends the sequence.
        GPSConfigurationStep step{};
        QByteArray wire{};
        std::variant<GPSTextMatcher, RawReply, GPSFrameMatcher> reply{};
        unsigned attempts = 1;
    };

    /// Escape hatch for receiver behaviour that is not one command and its reply. It may await channel operations.
    struct Custom
    {
        std::string label{};
        std::function<GPSTask<bool>()> run{};
        bool required = true;
    };

    struct [[nodiscard]] Result
    {
        /// The failed required step, or the step during which a sticky failure ended the sequence.
        std::optional<size_t> failedStep = std::nullopt;
        std::string failedLabel{};
        /// The failed command's last outcome, the sticky failure's outcome, or Pending for a failed custom step.
        GPSCommandOutcome outcome = GPSCommandOutcome::Pending;

        [[nodiscard]] bool succeeded() const { return !failedStep.has_value(); }
    };

    std::vector<std::variant<Command, Custom>> steps{};
};
