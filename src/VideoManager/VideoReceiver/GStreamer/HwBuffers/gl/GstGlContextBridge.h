#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include <gst/gst.h>

/// Process-wide shared GstGL display/context answering gst-gl NEED_CONTEXT so decoders allocate textures QRhi can
/// sample (zero-copy); without it gst-gl isolates.
namespace GstGlContextBridge {

/// Idempotent; builds a shared display+context from QOpenGLContext::globalShareContext. True on success.
bool prime();

/// Inspect a NEED_CONTEXT and respond with the shared display/context; returns GST_BUS_DROP when consumed, else
/// GST_BUS_PASS. Thread-safe.
GstBusSyncReply handleSyncMessage(GstMessage* message);

/// Answer a downstream GST_QUERY_CONTEXT (gst.gl.GLDisplay/app_context); true -> caller signals GST_PAD_PROBE_HANDLED.
bool answerContextQuery(GstQuery* query);

/// Drop the cached display/context so the next prime() rebuilds; call from receiver teardown.
void reset();

/// Clear exhausted-retry latch so a later NEED_CONTEXT can prime; no-op if already primed.
void rearm();

}  // namespace GstGlContextBridge

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
