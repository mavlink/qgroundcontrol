#pragma once

#include <QtCore/QDeadlineTimer>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include "GstScoped.h"

/// Finalization of the active recording, settled by the first of: splitmuxsink-fragment-closed or a branch/pipeline
/// EOS (closed), or a recorder ERROR (failed). Armed with the recording's file sink, so messages about an earlier
/// file can never settle the current one.
///
/// Threading: settled from the bus handler on whichever thread posts; armed, disarmed and waited on by the worker.
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
    Outcome outcome() const;

    /// Settles the armed recording with @p outcome unless it already ended. Returns whether this call settled it.
    bool settle(Outcome outcome);
    /// As settle(), but only when @p source is (inside) the armed file sink. @p belongs reports that membership.
    bool settleFrom(GstObject* source, Outcome outcome, bool* belongs = nullptr);

    /// Blocks until the recording ends or @p deadline expires; returns Pending on timeout.
    Outcome wait(QDeadlineTimer deadline);

private:
    bool _settleLocked(Outcome outcome);

    mutable QMutex _mutex;
    QWaitCondition _settled;
    GStreamer::GstElementPtr _fileSink;
    Outcome _outcome = Outcome::Pending;
};
