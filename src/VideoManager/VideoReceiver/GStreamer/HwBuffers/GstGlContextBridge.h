#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include <gst/gst.h>

/// Process-wide shared GstGL display/context bridging Qt's GL with GStreamer's
/// gst-gl elements (glupload, glcolorconvert, gldownload, the *gl decoders).
///
/// gst-gl elements ask the pipeline for GST_GL_DISPLAY_CONTEXT_TYPE and
/// "gst.gl.app_context" via GST_MESSAGE_NEED_CONTEXT. If we respond with our
/// shared display+context, the decoder allocates textures in a context Qt's
/// QRhi (GL backend) can sample from — true zero-copy.
///
/// Without this bridge, gst-gl creates an internal context isolated from Qt;
/// any GL textures it produces are unreachable from QRhi's render thread.
namespace GstGlContextBridge {

/// Idempotent. Tries to build a shared display+context from Qt's
/// QOpenGLContext::globalShareContext (must already exist — typically true once
/// the first QQuickWindow has been shown). Returns true on success.
bool prime();

/// Inspect a NEED_CONTEXT message; if it's for the gst-gl context types this
/// bridge serves, respond with the shared display/context and consume the
/// message. Returns GST_BUS_DROP when consumed, GST_BUS_PASS otherwise. Cheap
/// when the message isn't relevant. Thread-safe.
GstBusSyncReply handleSyncMessage(GstMessage *message);

/// Answer a downstream GST_QUERY_CONTEXT for gst.gl.GLDisplay or gst.gl.app_context
/// using the primed display/context. Returns true iff the query was filled — caller
/// should signal GST_PAD_PROBE_HANDLED to short-circuit upstream propagation.
bool answerContextQuery(GstQuery *query);

/// Drop the cached display/context so the next prime() rebuilds against the
/// current Qt GL state. Call from receiver teardown — between sessions Qt may
/// destroy and recreate its GL context, leaving the cached gst-gl wrappers
/// stale.
void reset();

/// Clear exhausted-retry latch so a later NEED_CONTEXT can prime; no-op if already primed.
void rearm();

} // namespace GstGlContextBridge

#endif // QGC_HAS_GST_GLMEMORY_GPU_PATH
