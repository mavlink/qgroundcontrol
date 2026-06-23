#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <gst/gst.h>

// Forward-declare the opaque GObject type so the currentDevice() accessor below parses
// for callers that haven't pulled in <gst/d3d11/gstd3d11.h>. Bridge implementation and
// any caller that dereferences the pointer must include the full header.
typedef struct _GstD3D11Device GstD3D11Device;

/// Process-wide shared GstD3D11Device bridging Qt's D3D11 RHI device with
/// GStreamer's d3d11 elements (`d3d11h264dec`, `d3d11vp9dec`, `d3d11convert`).
///
/// d3d11 elements ask the pipeline for `GST_D3D11_DEVICE_HANDLE_CONTEXT_TYPE`
/// ("gst.d3d11.device.handle") via GST_MESSAGE_NEED_CONTEXT. If we respond with
/// a GstD3D11Device wrapping the same `ID3D11Device*` QRhi uses, the decoder
/// allocates textures on a device QRhi can sample from — true zero-copy.
///
/// Without this bridge, d3d11 elements create an internal device isolated from
/// QRhi; textures from one device are unusable on the other.
namespace GstD3D11ContextBridge {

/// Idempotent. Returns true when a shared GstD3D11Device has been built by
/// wrapping the live `ID3D11Device*` exposed via QRhi's native handles.
/// Returns false (and logs once) if QRhi isn't D3D11, isn't yet initialized,
/// or the wrap call fails — caller should retry on a later NEED_CONTEXT.
bool prime();

/// Inspect a NEED_CONTEXT message; if it's for `gst.d3d11.device.handle`,
/// respond with the shared GstD3D11Device and consume the message. Returns
/// GST_BUS_DROP when consumed, GST_BUS_PASS otherwise. Cheap when the message
/// isn't relevant. Thread-safe.
GstBusSyncReply handleSyncMessage(GstMessage *message);

/// Drop the cached GstD3D11Device so the next prime() rebuilds against the
/// current QRhi device. Call from receiver teardown.
void reset();

/// Transfer-full ref to the cached shared device (caller unrefs), or nullptr if not primed.
/// Caller validates GstD3D11Memory device matches; transfer-full keeps it alive across reset().
GstD3D11Device *currentDevice();

} // namespace GstD3D11ContextBridge

#endif // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
