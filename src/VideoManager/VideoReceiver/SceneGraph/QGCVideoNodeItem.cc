#include "QGCVideoNodeItem.h"

#include <QtQuick/QQuickWindow>
#include <rhi/qrhi.h>

QGCVideoNodeItem::QGCVideoNodeItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

QGCVideoNodeItem::~QGCVideoNodeItem() = default;

void QGCVideoNodeItem::setCurrentTexture(QRhiTexture* texture, const QSize& frameSize,
                                         QVideoFrameFormat::PixelFormat pixelFormat,
                                         const QMatrix4x4& externalTextureMatrix)
{
    _pendingTexture = texture;
    _frameSize = frameSize;
    _pixelFormat = pixelFormat;
    _externalTextureMatrix = externalTextureMatrix;
    _dirty = true;
    update();
}

QSGNode* QGCVideoNodeItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    auto* node = static_cast<QGCVideoRenderNode*>(oldNode);
    if (!node) {
        node = new QGCVideoRenderNode;
    }
    if (_dirty) {
        node->setFrame(_pendingTexture, _frameSize, _pixelFormat, _externalTextureMatrix);
        _dirty = false;
    }
    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}

QGCVideoRenderNode::QGCVideoRenderNode() = default;

QGCVideoRenderNode::~QGCVideoRenderNode() = default;

void QGCVideoRenderNode::setFrame(QRhiTexture* texture, const QSize& frameSize,
                                  QVideoFrameFormat::PixelFormat pixelFormat,
                                  const QMatrix4x4& externalTextureMatrix)
{
    _texture = texture;
    _frameSize = frameSize;
    _pixelFormat = pixelFormat;
    _externalTextureMatrix = externalTextureMatrix;
}

QSGRenderNode::StateFlags QGCVideoRenderNode::changedStates() const
{
    return {BlendState, ScissorState, ViewportState};
}

QSGRenderNode::RenderingFlags QGCVideoRenderNode::flags() const
{
    return {BoundedRectRendering, OpaqueRendering};
}

QRectF QGCVideoRenderNode::rect() const
{
    return QRectF(QPointF(0, 0), QSizeF(_frameSize));
}

void QGCVideoRenderNode::prepare()
{
    // TODO: build/update the QRhiGraphicsPipeline, sampler, SRB, and per-frame uniform buffer here (this runs inside
    // QRhi::beginFrame, before render()). Currently a no-op stub.
}

void QGCVideoRenderNode::render(const RenderState* /*state*/)
{
    // PARKED STUB — does not draw. To make this node present video for real, implement, using commandBuffer() and
    // renderTarget() from the QSGRenderNode base and the imported _texture:
    //
    //   - Plane sampling: NV12/P010 (R8+RG8 / R16+RG16 two-plane) and RGBA fast path; Android external-OES needs a
    //     samplerExternalOES variant. Pick the shader by _pixelFormat + QRhi backend.
    //   - Colour conversion matrices: BT.601 / BT.709 / BT.2020, selected from the frame's colour space.
    //   - Colour range: full vs limited (video) range scale/offset before the YUV->RGB matrix.
    //   - HDR tonemap: PQ (ST2084) / HLG transfer handling + BT2390 tonemap to the swapchain's max luminance; this is
    //     the whole point of bypassing QQuickVideoOutput. Honour the surface QRhiSwapChain::Format (SDR/HDR).
    //   - Orientation / external-texture matrix: apply _externalTextureMatrix and the item's transform/mirroring.
    //   - Subtitle overlay: composite QtMultimedia's subtitle layout (or our own) on top.
    //   - Per-backend shader variants: GL / Vulkan / D3D / Metal QShader permutations incl. the external-OES path.
    //
    // TODO (#7, compute-pipeline conversion): a QRhiComputePipeline (QRhi::Compute feature +
    // QRhiTexture::UsedWithLoadStore on the RGB target) could do P010/NV12 -> RGB and the HDR tonemap on the shared
    // GStreamer/QRhi device, avoiding a fragment pass. It is only meaningful inside this custom-node path (the
    // QtMultimedia sink can't host it) and must keep a fragment-shader fallback for GLES < 3.1 / GL < 4.3 where compute
    // is unavailable.
}

void QGCVideoRenderNode::releaseResources()
{
    // TODO: destroy the pipeline / sampler / SRB / uniform buffers created in prepare(). No-op while stubbed.
    _texture = nullptr;
}
