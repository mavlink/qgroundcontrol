// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QRHIWIDGET_H
#define QRHIWIDGET_H

#include <QtWidgets/qwidget.h>

QT_BEGIN_NAMESPACE

class QRhiWidgetPrivate;
class QRhi;
class QRhiTexture;
class QRhiRenderBuffer;
class QRhiRenderTarget;
class QRhiCommandBuffer;

class Q_WIDGETS_EXPORT QRhiWidget : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QRhiWidget)
    Q_PROPERTY(int sampleCount READ sampleCount WRITE setSampleCount NOTIFY sampleCountChanged)
    Q_PROPERTY(TextureFormat colorBufferFormat READ colorBufferFormat WRITE setColorBufferFormat NOTIFY colorBufferFormatChanged)
    Q_PROPERTY(QSize fixedColorBufferSize READ fixedColorBufferSize WRITE setFixedColorBufferSize NOTIFY fixedColorBufferSizeChanged)
    Q_PROPERTY(bool mirrorVertically READ isMirrorVerticallyEnabled WRITE setMirrorVertically NOTIFY mirrorVerticallyChanged)
    QDOC_PROPERTY(bool autoRenderTarget READ isAutoRenderTargetEnabled WRITE setAutoRenderTarget)

public:
    explicit QRhiWidget(QWidget *parent = nullptr, Qt::WindowFlags f = {});
    ~QRhiWidget() override;

    enum class Api {
        Null,
        OpenGL,
        Metal,
        Vulkan,
        Direct3D11,
        Direct3D12,
    };
    Q_ENUM(Api)

    enum class TextureFormat {
        RGBA8,
        RGBA16F,
        RGBA32F,
        RGB10A2,
    };
    Q_ENUM(TextureFormat)

    Api api() const;
    void setApi(Api api);

    bool isDebugLayerEnabled() const;
    void setDebugLayerEnabled(bool enable);

    int sampleCount() const;
    void setSampleCount(int samples);

    TextureFormat colorBufferFormat() const;
    void setColorBufferFormat(TextureFormat format);

    QSize fixedColorBufferSize() const;
    void setFixedColorBufferSize(QSize pixelSize);
    void setFixedColorBufferSize(int w, int h) { setFixedColorBufferSize(QSize(w, h)); }

    bool isMirrorVerticallyEnabled() const;
    void setMirrorVertically(bool enabled);

    QImage grabFramebuffer() const;

protected:
    explicit QRhiWidget(QRhiWidgetPrivate &dd, QWidget *parent = nullptr, Qt::WindowFlags f = {});

    bool isAutoRenderTargetEnabled() const;
    void setAutoRenderTarget(bool enabled);

    virtual void initialize(QRhiCommandBuffer *cb);
    virtual void render(QRhiCommandBuffer *cb);
    virtual void releaseResources();

    QRhi *rhi() const;
    QRhiTexture *colorTexture() const;
    QRhiRenderBuffer *msaaColorBuffer() const;
    QRhiTexture *resolveTexture() const;
    QRhiRenderBuffer *depthStencilBuffer() const;
    QRhiRenderTarget *renderTarget() const;

    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    bool event(QEvent *e) override;

Q_SIGNALS:
    void frameSubmitted();
    void renderFailed();
    void sampleCountChanged(int samples);
    void colorBufferFormatChanged(TextureFormat format);
    void fixedColorBufferSizeChanged(const QSize &pixelSize);
    void mirrorVerticallyChanged(bool enabled);
};

QT_END_NAMESPACE

#endif
