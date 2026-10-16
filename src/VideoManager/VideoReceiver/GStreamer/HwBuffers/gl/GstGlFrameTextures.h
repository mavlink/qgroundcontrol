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
        : _names(names)
    {
        wrapPlanes(rhi, size, pixelFormat,
                   std::span<const GLuint>(_names.data(), std::size_t(std::clamp(count, 0, GstHw::kMaxPlanes))));
    }

protected:
    std::array<GLuint, GstHw::kMaxPlanes> _names{};
};
