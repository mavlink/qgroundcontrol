// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKRHIITEM_H
#define QQUICKRHIITEM_H

#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

class QQuickRhiItem;
class QQuickRhiItemPrivate;
class QQuickRhiItemNode;
class QRhi;
class QRhiCommandBuffer;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiRenderTarget;

class Q_QUICK_EXPORT QQuickRhiItemRenderer
{
public:
    QQuickRhiItemRenderer();
    virtual ~QQuickRhiItemRenderer();

protected:
    virtual void initialize(QRhiCommandBuffer *cb) = 0;
    virtual void synchronize(QQuickRhiItem *item) = 0;
    virtual void render(QRhiCommandBuffer *cb) = 0;

    void update();

    QRhi *rhi() const;
    QRhiTexture *colorTexture() const;
    QRhiRenderBuffer *msaaColorBuffer() const;
    QRhiTexture *resolveTexture() const;
    QRhiRenderBuffer *depthStencilBuffer() const;
    QRhiRenderTarget *renderTarget() const;

private:
    QQuickRhiItemNode *node;
    friend class QQuickRhiItem;
    friend class QQuickRhiItemNode;

    Q_DISABLE_COPY_MOVE(QQuickRhiItemRenderer)
};

class Q_QUICK_EXPORT QQuickRhiItem : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickRhiItem)

    Q_PROPERTY(int sampleCount READ sampleCount WRITE setSampleCount NOTIFY sampleCountChanged FINAL)
    Q_PROPERTY(TextureFormat colorBufferFormat READ colorBufferFormat WRITE setColorBufferFormat NOTIFY colorBufferFormatChanged FINAL)
    Q_PROPERTY(bool mirrorVertically READ isMirrorVerticallyEnabled WRITE setMirrorVertically NOTIFY mirrorVerticallyChanged FINAL)
    Q_PROPERTY(bool alphaBlending READ alphaBlending WRITE setAlphaBlending NOTIFY alphaBlendingChanged FINAL)
    Q_PROPERTY(int fixedColorBufferWidth READ fixedColorBufferWidth WRITE setFixedColorBufferWidth NOTIFY fixedColorBufferWidthChanged FINAL)
    Q_PROPERTY(int fixedColorBufferHeight READ fixedColorBufferHeight WRITE setFixedColorBufferHeight NOTIFY fixedColorBufferHeightChanged FINAL)
    Q_PROPERTY(QSize effectiveColorBufferSize READ effectiveColorBufferSize NOTIFY effectiveColorBufferSizeChanged FINAL)

public:
    enum class TextureFormat {
        RGBA8,
        RGBA16F,
        RGBA32F,
        RGB10A2
    };
    Q_ENUM(TextureFormat)

    explicit QQuickRhiItem(QQuickItem *parent = nullptr);
    ~QQuickRhiItem() override;

    int sampleCount() const;
    void setSampleCount(int samples);

    TextureFormat colorBufferFormat() const;
    void setColorBufferFormat(TextureFormat format);

    bool isMirrorVerticallyEnabled() const;
    void setMirrorVertically(bool enable);

    bool alphaBlending() const;
    void setAlphaBlending(bool enable);

    int fixedColorBufferWidth() const;
    void setFixedColorBufferWidth(int width);
    int fixedColorBufferHeight() const;
    void setFixedColorBufferHeight(int height);

    QSize effectiveColorBufferSize() const;

    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

Q_SIGNALS:
    void sampleCountChanged();
    void colorBufferFormatChanged();
    void autoRenderTargetChanged();
    void mirrorVerticallyChanged();
    void alphaBlendingChanged();
    void fixedColorBufferWidthChanged();
    void fixedColorBufferHeightChanged();
    void effectiveColorBufferSizeChanged();

protected:
    explicit QQuickRhiItem(QQuickRhiItemPrivate &dd, QQuickItem *parent = nullptr);

    virtual QQuickRhiItemRenderer *createRenderer() = 0;

    bool isAutoRenderTargetEnabled() const;
    void setAutoRenderTarget(bool enabled);

    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    bool event(QEvent *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;

private Q_SLOTS:
    void invalidateSceneGraph();

    friend class QQuickRhiItemNode;
};

QT_END_NAMESPACE

#endif // QQUICKRHIITEM_H
