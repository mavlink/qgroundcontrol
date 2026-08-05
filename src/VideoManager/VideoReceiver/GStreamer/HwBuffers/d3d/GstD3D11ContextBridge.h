#pragma once

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <gst/gst.h>

// Forward-declare the opaque type so currentDevice() parses without <gst/d3d11/gstd3d11.h>; dereferencing callers
// include the full header.
typedef struct _GstD3D11Device GstD3D11Device;

/// Process-wide shared GstD3D11Device answering NEED_CONTEXT (gst.d3d11.device.handle) with QRhi's ID3D11Device so
/// decoders are zero-copy.
namespace GstD3D11ContextBridge {

/// Idempotent; wraps QRhi's live ID3D11Device into the shared GstD3D11Device. False (logs once) if QRhi isn't
/// D3D11/ready — retry on a later NEED_CONTEXT.
bool prime();

/// NEED_CONTEXT for gst.d3d11.device.handle -> respond with the shared device; GST_BUS_DROP when consumed, else
/// GST_BUS_PASS. Thread-safe.
GstBusSyncReply handleSyncMessage(GstMessage* message);

/// Answer a sink-bin GST_QUERY_CONTEXT (gst.d3d11.device.handle) with the shared device. True when consumed.
bool answerContextQuery(GstQuery* query);

/// Drop the cached GstD3D11Device so the next prime() rebuilds; call from receiver teardown.
void reset();

/// Transfer-full ref to the cached device (caller unrefs), nullptr if not primed; survives reset().
GstD3D11Device* currentDevice();

}  // namespace GstD3D11ContextBridge

#endif  // Q_OS_WIN && QGC_HAS_GST_D3D11_GPU_PATH
