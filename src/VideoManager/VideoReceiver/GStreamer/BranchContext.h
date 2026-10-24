#pragma once

#include <functional>
#include <gst/gst.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

#include "GstScoped.h"

class PipelineDotDumper;
class StreamWatchdog;

/// Shared by GstVideoReceiver and its components.
Q_DECLARE_LOGGING_CATEGORY(GstVideoReceiverLog)

/// The tee branch a pipeline element belongs to, so a bus message can be blamed on the part that failed.
enum class PipelineBranch
{
    None,  ///< The source, the tee, or the pipeline itself.
    Decoding,
    Recording,
};

namespace GStreamer {

inline GQuark pipelineBranchQuark()
{
    static const GQuark quark = g_quark_from_static_string("qgc-pipeline-branch");
    return quark;
}

/// Marks @p element and everything inside it as part of @p branch.
inline void markBranch(GstElement* element, PipelineBranch branch)
{
    g_object_set_qdata(G_OBJECT(element), pipelineBranchQuark(), GINT_TO_POINTER(static_cast<int>(branch)));
}

/// The branch of @p object's nearest marked ancestor (itself included). Thread-safe.
inline PipelineBranch branchOf(GstObject* object)
{
    GstObjectPtr current = object ? adoptObject(GST_OBJECT(gst_object_ref(object))) : nullptr;
    while (current) {
        if (const gpointer mark = g_object_get_qdata(G_OBJECT(current.get()), pipelineBranchQuark())) {
            return static_cast<PipelineBranch>(GPOINTER_TO_INT(mark));
        }
        current = adoptObject(gst_object_get_parent(current.get()));
    }
    return PipelineBranch::None;
}

}  // namespace GStreamer

/// What a tee branch borrows from the PipelineSession that owns it. Everything is used on the receiver's worker
/// thread, except post(), which any thread may call.
struct BranchContext
{
    GstElement* pipeline = nullptr;
    GstElement* tee = nullptr;
    /// For logs: the stream's URI without credentials.
    QString redactedUri;
    const PipelineDotDumper* dotDumper = nullptr;
    StreamWatchdog* watchdog = nullptr;
    /// Runs a task on the worker thread unless the session is gone by then.
    std::function<void(std::function<void()>)> post;
};
