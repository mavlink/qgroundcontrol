#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include <gst/gl/gstgl_fwd.h>

#include "GstContextBridgeBase.h"

/// Shares Qt's GL display and context (QOpenGLContext::globalShareContext) with gst-gl through gst.gl.GLDisplay and
/// gst.gl.app_context, so decoders allocate textures QRhi can sample; without it gst-gl isolates.
namespace GstGlContextBridge {

/// The process-wide GL bridge.
GstContextBridgeBase& instance();

/// True when @p context is in the share group of Qt's context as the bridge wrapped it, so Qt samples the textures it
/// creates. False for a context gst-gl made on its own, as it does when no element was handed the bridge's. Any
/// thread.
bool sharesQtContext(GstGLContext* context);

}  // namespace GstGlContextBridge

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
