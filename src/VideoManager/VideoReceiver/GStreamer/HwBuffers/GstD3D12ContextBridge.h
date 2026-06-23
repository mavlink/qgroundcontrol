#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)

#include <gst/gst.h>

// Forward-declare the opaque GObject type so the currentDevice() accessor below parses
// for callers that haven't pulled in <gst/d3d12/gstd3d12.h>. Bridge implementation and
// any caller that dereferences the pointer must include the full header.
typedef struct _GstD3D12Device GstD3D12Device;

/// Process-wide shared GstD3D12Device bridging Qt's D3D12 RHI adapter with
/// GStreamer's d3d12 elements (`d3d12h264dec`, `d3d12vp9dec`, `d3d12convert`).
///
/// d3d12 elements ask the pipeline for `GST_D3D12_DEVICE_HANDLE_CONTEXT_TYPE`
/// ("gst.d3d12.device.handle") via GST_MESSAGE_NEED_CONTEXT. If we respond with
/// a GstD3D12Device created for the same adapter LUID as QRhi uses, the decoder
/// allocates resources on a compatible adapter — true zero-copy.
///
/// Without this bridge, d3d12 elements create an internal device isolated from
/// QRhi; resources from one device are unusable on the other.
namespace GstD3D12ContextBridge {

/// Idempotent. Returns true when a shared GstD3D12Device has been built for
/// the same adapter LUID as QRhi's D3D12 device (composed from the high/low
/// halves QRhi exposes via its native handles). Returns false (and logs once)
/// if QRhi isn't D3D12, isn't yet initialized, or the device-from-LUID call
/// fails — caller should retry on a later NEED_CONTEXT.
bool prime();

/// Inspect a NEED_CONTEXT message; if it's for `gst.d3d12.device.handle`,
/// respond with the shared GstD3D12Device and consume the message. Returns
/// GST_BUS_DROP when consumed, GST_BUS_PASS otherwise. Cheap when the message
/// isn't relevant. Thread-safe.
GstBusSyncReply handleSyncMessage(GstMessage *message);

/// Drop the cached GstD3D12Device so the next prime() rebuilds against the
/// current QRhi device. Call from receiver teardown.
void reset();

/// Transfer-full ref to the cached shared device (caller unrefs), or nullptr if not primed.
/// Caller validates GstD3D12Memory device matches; transfer-full keeps it alive across reset().
GstD3D12Device *currentDevice();

} // namespace GstD3D12ContextBridge

#endif // Q_OS_WIN && QGC_HAS_GST_D3D12_GPU_PATH
