#pragma once

#include <QtCore/QSize>
#include <QtGui/qopengl.h>
#include <QtMultimedia/QVideoFrameFormat>
#include <array>
#include <private/qvideotexturehelper_p.h>
#include <rhi/qrhi.h>

#include "GstHwFrameTexturesBase.h"
#include "GstHwVideoBuffer.h"

/// Shared base for GL-texture-backed `QVideoFrameTextures` wrappers (GLMemory and DMABuf-via-EGLImage).
class GstGlFrameTextures : public GstHwFrameTexturesBase
{
public:
    using FallbackPolicy = QVideoTextureHelper::TextureDescription::FallbackPolicy;

    GstGlFrameTextures(QRhi* rhi, QSize size, QVideoFrameFormat::PixelFormat pixelFormat,
                       std::array<GLuint, GstHw::kMaxPlanes> names, int count,
                       FallbackPolicy fallback = FallbackPolicy::Enable)
        : _rhi(rhi), _size(size), _pixelFormat(pixelFormat), _names(names)
    {
        _count = count;
        const auto* desc = QVideoTextureHelper::textureDescription(pixelFormat);
        if (!desc)
            return;
        for (int i = 0; i < _count; ++i) {
            // GL_NONE (0) is silently accepted by createFrom but samples as black; gst-gl can hand us 0 if a plane
            // wasn't uploaded yet.
            if (_names[i] == 0)
                continue;
            const QSize planeSize = desc->rhiPlaneSize(size, i, rhi);
            _textures[i].reset(rhi->newTexture(desc->rhiTextureFormat(i, rhi, fallback), planeSize, 1, {}));
            if (_textures[i] && !_textures[i]->createFrom({_names[i], 0})) {
                _textures[i].reset();
            }
        }
    }

protected:
    QRhi* _rhi = nullptr;
    QSize _size;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
    std::array<GLuint, GstHw::kMaxPlanes> _names{};
};
