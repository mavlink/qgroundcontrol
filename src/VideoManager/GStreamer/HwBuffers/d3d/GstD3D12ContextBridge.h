#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <gst/gst.h>

#include "GstContextBridgeBase.h"

// Forward-declare the opaque type so currentDevice() parses without <gst/d3d12/gstd3d12.h>; dereferencing callers
// include the full header.
typedef struct _GstD3D12Device GstD3D12Device;

/// Process-wide shared GstD3D12Device on QRhi's adapter LUID, answering gst.d3d12.device.handle so decoders are
/// zero-copy.
namespace GstD3D12ContextBridge {

/// The process-wide D3D12 bridge.
GstContextBridgeBase& instance();

/// Transfer-full ref to the shared device (caller unrefs), nullptr if not primed.
GstD3D12Device* currentDevice();

}  // namespace GstD3D12ContextBridge

#endif  // QGC_HAS_GST_D3D12_GPU_PATH
