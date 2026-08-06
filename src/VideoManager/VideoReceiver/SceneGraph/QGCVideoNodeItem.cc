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
}

void QGCVideoRenderNode::render(const RenderState* /*state*/)
{
}

void QGCVideoRenderNode::releaseResources()
{
    _texture = nullptr;
}
