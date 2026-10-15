#pragma once

#include <algorithm>
#include <array>
#include <span>

#include <QtCore/QSize>
#include <QtGui/qopengl.h>
#include <QtMultimedia/QVideoFrameFormat>

#include "GstHwFrameTexturesBase.h"
#include "GstHwVideoBuffer.h"

/// Shared base for GL-texture-backed `QVideoFrameTextures` wrappers (GLMemory and DMABuf-via-EGLImage).
class GstGlFrameTextures : public GstHwFrameTexturesBase
{
public:
    GstGlFrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                       std::array<GLuint, GstHw::kMaxPlanes> names, int count)
        : _rhi(rhi)
        , _size(size)
        , _pixelFormat(pixelFormat)
        , _names(names)
    {
        std::array<quint64, GstHw::kMaxPlanes> handles{};
        std::copy(names.begin(), names.end(), handles.begin());
        wrapPlanes(rhi, size, pixelFormat,
                   std::span<const quint64>(handles.data(), std::size_t(std::clamp(count, 0, GstHw::kMaxPlanes))));
    }

protected:
    QRhi* _rhi = nullptr;
    QSize _size;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
    std::array<GLuint, GstHw::kMaxPlanes> _names{};
};
