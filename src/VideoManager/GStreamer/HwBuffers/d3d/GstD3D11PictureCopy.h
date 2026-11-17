#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <d3d11.h>
#include <private/qhwvideobuffer_p.h>

#include <QtCore/QRect>

/// Copies of a picture out of a Direct3D 11 texture that holds more than it: a decoder's texture array or padded
/// surface, or a texture opened from another device. For the render thread, on the device QRhi renders with.
namespace GstD3D11PictureCopy {

/// True when @p texture lives on @p device, the comparison Qt's D3D11 texture bridge makes.
bool onDevice(ID3D11Texture2D* texture, ID3D11Device* device);

/// D3D11 creates 4:2:0 textures with even sizes only.
bool hasEvenSizes(DXGI_FORMAT format);

/// Copies @p picture of @p source's subresource @p slice into a texture of exactly its size, in @p source's format:
/// plane @p plane of @p old when that is a texture this function made with that layout, whose last read QRhi already
/// queued on @p context, else a new one on @p device. AddRef'd; null when the texture can't be created. The caller
/// keeps anyone else off @p context meanwhile.
ID3D11Texture2D* copy(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* source, UINT slice,
                      QRect picture, const QVideoFrameTexturesUPtr& old, int plane);

}  // namespace GstD3D11PictureCopy

#endif  // QGC_HAS_GST_D3D11_GPU_PATH
