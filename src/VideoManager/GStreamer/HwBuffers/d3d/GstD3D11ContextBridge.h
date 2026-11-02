#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <gst/gst.h>

#include "GstContextBridgeBase.h"

// Forward-declare the opaque type so currentDevice() parses without <gst/d3d11/gstd3d11.h>; dereferencing callers
// include the full header.
typedef struct _GstD3D11Device GstD3D11Device;

/// Process-wide shared GstD3D11Device wrapping QRhi's ID3D11Device, answering gst.d3d11.device.handle so decoders
/// are zero-copy.
namespace GstD3D11ContextBridge {

/// The process-wide D3D11 bridge.
GstContextBridgeBase& instance();

/// Transfer-full ref to the shared device (caller unrefs), nullptr if not primed.
GstD3D11Device* currentDevice();

}  // namespace GstD3D11ContextBridge

#endif  // QGC_HAS_GST_D3D11_GPU_PATH
