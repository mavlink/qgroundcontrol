#pragma once

#include <gst/gst.h>

#include "BranchContext.h"
#include "GstScoped.h"

/// \brief The tee ! queue ! valve head every tee branch starts with.
///
/// Built closed (the valve drops frames) and marked as one GStreamer::PipelineBranch, so bus messages from it are
/// blamed on that branch. A branch opens the valve once it has linked what follows it, which then starts from the
/// keyframe open() asks upstream for, and closes it again before taking that out. The pipeline owns both elements.
///
/// Threading: the receiver's worker, except the overrun handler (streaming threads).
class TeeBranchHead
{
public:
    /// Adds the queue and the closed valve to @p pipeline and links them behind @p tee. False on failure, logged with
    /// @p what naming the branch; nothing stays built then.
    [[nodiscard]] bool build(GstElement* pipeline, GstElement* tee, GStreamer::PipelineBranch branch, const char* what);
    /// Forgets the elements, which the pipeline still owns, and disconnects the overrun handler.
    void reset();

    /// Connects @p callback to the queue's "overrun", which fires on a streaming thread when the queue is full, before
    /// it leaks or blocks.
    void onOverrun(GCallback callback, gpointer data);

    /// Lets frames through and asks upstream for a keyframe to start from.
    void open();
    /// Drops frames again; nothing before build().
    void close();
    /// Asks upstream for a keyframe; nothing before build().
    void requestKeyframe() const;

    GstElement* queue() const { return _queue; }

    GstElement* valve() const { return _valve; }

    /// Whether @p object is the queue or the valve.
    bool contains(GstObject* object) const;

private:
    GstElement* _queue = nullptr;
    GstElement* _valve = nullptr;
    GStreamer::ScopedSignal _overrun;
};
