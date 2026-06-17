#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <gst/gst.h>

// Forward-declare the opaque type so currentDevice() parses without <gst/d3d12/gstd3d12.h>; dereferencing callers
// include the full header.
typedef struct _GstD3D12Device GstD3D12Device;

/// Process-wide shared GstD3D12Device answering NEED_CONTEXT (gst.d3d12.device.handle) for QRhi's adapter LUID so
/// decoders are zero-copy.
namespace GstD3D12ContextBridge {

/// Idempotent; builds the shared GstD3D12Device for QRhi's adapter LUID. False (logs once) if QRhi isn't D3D12/ready —
/// retry on a later NEED_CONTEXT.
bool prime();

/// NEED_CONTEXT for gst.d3d12.device.handle -> respond with the shared device; GST_BUS_DROP when consumed, else
/// GST_BUS_PASS. Thread-safe.
GstBusSyncReply handleSyncMessage(GstMessage* message);

/// Answer a sink-bin GST_QUERY_CONTEXT (gst.d3d12.device.handle) with the shared device. True when consumed.
bool answerContextQuery(GstQuery* query);

/// Drop the cached GstD3D12Device so the next prime() rebuilds; call from receiver teardown.
void reset();

/// Transfer-full ref to the cached device (caller unrefs), nullptr if not primed; survives reset().
GstD3D12Device* currentDevice();

}  // namespace GstD3D12ContextBridge

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D12_GPU_PATH
