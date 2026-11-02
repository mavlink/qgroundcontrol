#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include "GstContextBridgeBase.h"

/// Shares Qt's GL display and context (QOpenGLContext::globalShareContext) with gst-gl through gst.gl.GLDisplay and
/// gst.gl.app_context, so decoders allocate textures QRhi can sample; without it gst-gl isolates.
namespace GstGlContextBridge {

/// The process-wide GL bridge.
GstContextBridgeBase& instance();

}  // namespace GstGlContextBridge

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
