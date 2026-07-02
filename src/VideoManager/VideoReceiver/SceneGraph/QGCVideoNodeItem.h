#pragma once

#include <QtCore/QSize>
#include <QtGui/QMatrix4x4>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtQuick/QQuickItem>
#include <QtQuick/QSGRenderNode>

class QRhiTexture;

/// \brief PARKED HDR-passthrough alternative to the QtMultimedia QQuickVideoOutput sink.
///
/// Scaffold for a custom scene-graph video item that presents an already-imported zero-copy `QRhiTexture` (from the
/// HwBuffers GPU paths) straight into QGC's render thread via a `QSGRenderNode`, bypassing QQuickVideoOutput /
/// QGCQVideoSinkController. The motivation is HDR passthrough: routing the decoder's P010 surface through our own
/// node lets us keep the wide-gamut/PQ data instead of QtMultimedia's SDR-leaning conversion.
///
/// This is NOT wired into the live video path and does not replace the QtMultimedia sink. `render()` is a documented
/// stub; the TODO list there enumerates exactly what must be implemented to go live. It exists to compile-check the
/// scene-graph plumbing and hold the design.
class QGCVideoNodeItem : public QQuickItem
{
    Q_OBJECT

public:
    explicit QGCVideoNodeItem(QQuickItem* parent = nullptr);
    ~QGCVideoNodeItem() override;

    /// Hand the node the next frame's imported texture + metadata. Render-thread consumes it in updatePaintNode.
    /// No ownership taken: the caller (the GPU path's QVideoFrameTextures) keeps the texture alive for the frame.
    void setCurrentTexture(QRhiTexture* texture, const QSize& frameSize, QVideoFrameFormat::PixelFormat pixelFormat,
                           const QMatrix4x4& externalTextureMatrix);

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private:
    QRhiTexture* _pendingTexture = nullptr;
    QSize _frameSize;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
    QMatrix4x4 _externalTextureMatrix;
    bool _dirty = false;
};

/// Scene-graph node that draws the imported video texture with its own RHI pipeline. Stub: see render() TODOs.
class QGCVideoRenderNode : public QSGRenderNode
{
public:
    QGCVideoRenderNode();
    ~QGCVideoRenderNode() override;

    void setFrame(QRhiTexture* texture, const QSize& frameSize, QVideoFrameFormat::PixelFormat pixelFormat,
                  const QMatrix4x4& externalTextureMatrix);

    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;

    void prepare() override;
    void render(const RenderState* state) override;
    void releaseResources() override;

private:
    QRhiTexture* _texture = nullptr;
    QSize _frameSize;
    QVideoFrameFormat::PixelFormat _pixelFormat = QVideoFrameFormat::Format_Invalid;
    QMatrix4x4 _externalTextureMatrix;
};
