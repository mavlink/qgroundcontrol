#include "GstD3D11PictureCopy.h"

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)

#include <algorithm>
#include <rhi/qrhi.h>

namespace GstD3D11PictureCopy {

namespace {

// Tags the textures copy() creates: only those may be overwritten by a later frame. The textures a frame replaces can
// also wrap a texture GStreamer still owns.
constexpr GUID kCopyTargetTag = {0x6d3c1f52, 0x8a4e, 0x4b7f, {0x9c, 0x21, 0x5e, 0x0b, 0x7a, 0x93, 0xd4, 0x18}};
constexpr quint8 kCopyTargetMark = 1;

bool isCopyTarget(ID3D11Texture2D* texture)
{
    quint8 mark = 0;
    UINT size = static_cast<UINT>(sizeof(mark));
    return SUCCEEDED(texture->GetPrivateData(kCopyTargetTag, &size, &mark)) && (mark == kCopyTargetMark);
}

/// @p extent pixels, as a @p format texture holds them.
UINT textureExtent(int extent, DXGI_FORMAT format)
{
    return static_cast<UINT>(hasEvenSizes(format) ? ((extent + 1) & ~1) : extent);
}

/// @p old's plane @p plane texture, AddRef'd, when it is a texture copy() made with @p desc's layout. Read through the
/// texture QRhi wraps: @p old keeps its handles until the new import has its textures.
ID3D11Texture2D* reusableTarget(const QVideoFrameTexturesUPtr& old, int plane, const D3D11_TEXTURE2D_DESC& desc)
{
    QRhiTexture* const wrapper = old ? old->texture(static_cast<uint>(plane)) : nullptr;
    auto* const texture =
        wrapper ? reinterpret_cast<ID3D11Texture2D*>(static_cast<quintptr>(wrapper->nativeTexture().object)) : nullptr;
    if (!texture || !isCopyTarget(texture)) {
        return nullptr;
    }
    D3D11_TEXTURE2D_DESC have = {};
    texture->GetDesc(&have);
    if ((have.Width != desc.Width) || (have.Height != desc.Height) || (have.Format != desc.Format)) {
        return nullptr;
    }
    texture->AddRef();
    return texture;
}

}  // namespace

bool onDevice(ID3D11Texture2D* texture, ID3D11Device* device)
{
    ID3D11Device* owner = nullptr;
    texture->GetDevice(&owner);
    const bool same = (owner != nullptr) && (owner == device);
    if (owner) {
        owner->Release();
    }
    return same;
}

bool hasEvenSizes(DXGI_FORMAT format)
{
    switch (format) {
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            return true;
        default:
            return false;
    }
}

ID3D11Texture2D* copy(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* source, UINT slice,
                      QRect picture, const QVideoFrameTexturesUPtr& old, int plane)
{
    D3D11_TEXTURE2D_DESC sourceDesc = {};
    source->GetDesc(&sourceDesc);
    const UINT left = (std::min) (textureExtent(picture.x(), sourceDesc.Format), sourceDesc.Width);
    const UINT top = (std::min) (textureExtent(picture.y(), sourceDesc.Format), sourceDesc.Height);
    const UINT width = (std::min) (textureExtent(picture.width(), sourceDesc.Format), sourceDesc.Width - left);
    const UINT height = (std::min) (textureExtent(picture.height(), sourceDesc.Format), sourceDesc.Height - top);

    D3D11_TEXTURE2D_DESC targetDesc = {};
    targetDesc.Width = width;
    targetDesc.Height = height;
    targetDesc.MipLevels = 1;
    targetDesc.ArraySize = 1;
    targetDesc.Format = sourceDesc.Format;
    targetDesc.SampleDesc.Count = 1;
    targetDesc.Usage = D3D11_USAGE_DEFAULT;
    targetDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ID3D11Texture2D* target = reusableTarget(old, plane, targetDesc);
    if (!target) {
        if (FAILED(device->CreateTexture2D(&targetDesc, nullptr, &target)) || !target) {
            return nullptr;
        }
        (void) target->SetPrivateData(kCopyTargetTag, static_cast<UINT>(sizeof(kCopyTargetMark)), &kCopyTargetMark);
    }

    const D3D11_BOX box = {left, top, 0, left + width, top + height, 1};
    context->CopySubresourceRegion(target, 0, 0, 0, 0, source, slice, &box);
    return target;
}

}  // namespace GstD3D11PictureCopy

#endif  // QGC_HAS_GST_D3D11_GPU_PATH
