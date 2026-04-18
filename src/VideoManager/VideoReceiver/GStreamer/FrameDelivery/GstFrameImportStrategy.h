#pragma once

#include <QtMultimedia/QAbstractVideoBuffer>
#include <QtMultimedia/QVideoFrameFormat>
#include <memory>
#include <gst/gst.h>
#include <gst/video/video-info.h>

namespace GstFrameImportStrategy {

enum class Kind : quint8
{
    Cpu,
    DmaBufRhi,
};

[[nodiscard]] Kind select(GstBuffer* buffer);
[[nodiscard]] std::unique_ptr<QAbstractVideoBuffer> create(Kind kind,
                                                           GstBuffer* buffer,
                                                           const GstVideoInfo& videoInfo,
                                                           const QVideoFrameFormat& format);

}  // namespace GstFrameImportStrategy
