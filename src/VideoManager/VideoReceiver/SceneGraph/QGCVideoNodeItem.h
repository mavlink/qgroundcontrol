#pragma once

#include <QtCore/QSize>
#include <QtGui/QMatrix4x4>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtQuick/QQuickItem>
#include <QtQuick/QSGRenderNode>

class QRhiTexture;

class QGCVideoNodeItem : public QQuickItem
{
    Q_OBJECT

public:
    explicit QGCVideoNodeItem(QQuickItem* parent = nullptr);
    ~QGCVideoNodeItem() override;

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
