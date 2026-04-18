#pragma once

#include <QtCore/QByteArray>
#include <QtMultimedia/QAbstractVideoBuffer>
#include <QtMultimedia/QVideoFrameFormat>
#include <cstdint>
#include <memory>
#include <gst/gst.h>
#include <gst/video/video-info.h>

/// Central strategy point for converting negotiated GStreamer video memory into
/// Qt video buffers.
///
/// The appsink bridge should not know platform memory details. It asks this
/// factory which caps to advertise and how to wrap each GstBuffer. Today QGC
/// supports CPU memory everywhere and Linux DMA-BUF through a QHwVideoBuffer
/// strategy when built with QGC_GST_DMABUF. Future GLMemory, D3D11Memory,
/// AndroidHardwareBuffer, or Metal texture support belongs here.
namespace GstVideoBufferFactory {

enum class MemoryKind : uint8_t
{
    Cpu,
    DmaBuf,
    Unknown,
};

/// Caps advertised by appsink, ordered by preference. Hardware memory appears
/// before CPU memory only when QGC can actually wrap it.
[[nodiscard]] QByteArray appsinkCaps();

/// Best-effort memory classification for diagnostics and wrapper selection.
[[nodiscard]] MemoryKind classify(GstBuffer* buffer);

/// Wrap @p buffer in the best QAbstractVideoBuffer implementation for the
/// memory kind. Always returns a CPU wrapper as a safe fallback when no
/// hardware-memory wrapper is available.
[[nodiscard]] std::unique_ptr<QAbstractVideoBuffer> create(GstBuffer* buffer,
                                                           const GstVideoInfo& videoInfo,
                                                           const QVideoFrameFormat& format);

}  // namespace GstVideoBufferFactory
