#pragma once

#include "GstScoped.h"

/// Finalization of the active recording, settled by the first of: the recorder's EOS (closed) or a recorder ERROR
/// (failed). Armed with the recording's file sink, so messages about an earlier file can never settle the current one.
///
/// Threading: the receiver's worker only; the bus messages that settle it are handled there.
class RecordingFinalizer
{
public:
    enum class Outcome
    {
        Pending,
        Closed,
        Failed,
    };

    void arm(GstElement* fileSink);
    /// Returns how the recording ended.
    Outcome disarm();

    bool armed() const { return _fileSink != nullptr; }

    Outcome outcome() const { return _outcome; }

    /// Settles the armed recording with @p outcome unless it already ended. Returns whether this call settled it.
    bool settle(Outcome outcome);
    /// As settle(), but only when @p source is (inside) the armed file sink. @p belongs reports that membership.
    bool settleFrom(GstObject* source, Outcome outcome, bool* belongs = nullptr);

private:
    GStreamer::GstElementPtr _fileSink;
    Outcome _outcome = Outcome::Pending;
};
