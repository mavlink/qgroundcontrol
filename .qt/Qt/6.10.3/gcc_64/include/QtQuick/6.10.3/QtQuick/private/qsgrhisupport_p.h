// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRHISUPPORT_P_H
#define QSGRHISUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsgrenderloop_p.h"
#include "qsgrendererinterface.h"

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class QSGDefaultRenderContext;
class QOffscreenSurface;
class QQuickGraphicsConfiguration;

// Opting in/out of QRhi and choosing the default/requested backend is managed
// by this singleton. This is because this information may be needed before
// creating a render loop. A well-written render loop sets up its QRhi and
// related machinery using the helper functions in here.
//
// In addition, the class provides handy conversion and query stuff for the
// renderloop and the QSGRendererInterface implementations.
//
class Q_QUICK_EXPORT QSGRhiSupport
{
public:
    static QSGRhiSupport *instance_internal();
    static QSGRhiSupport *instance();
    static int chooseSampleCount(int samples, QRhi *rhi);
    static int chooseSampleCountForWindowWithRhi(QWindow *window, QRhi *rhi);
    static QImage grabAndBlockInCurrentFrame(QRhi *rhi, QRhiCommandBuffer *cb, QRhiTexture *src = nullptr);
    static void checkEnvQSgInfo();

#if QT_CONFIG(opengl)
    static QRhiTexture::Format toRhiTextureFormatFromGL(uint format, QRhiTexture::Flags *flags);
#endif

#if QT_CONFIG(vulkan)
    static QRhiTexture::Format toRhiTextureFormatFromVulkan(uint format, QRhiTexture::Flags *flags);
#endif

#if defined(Q_OS_WIN)
    static QRhiTexture::Format toRhiTextureFormatFromDXGI(uint format, QRhiTexture::Flags *flags);
#endif

#if QT_CONFIG(metal)
    static QRhiTexture::Format toRhiTextureFormatFromMetal(uint format, QRhiTexture::Flags *flags);
#endif

    void configure(QSGRendererInterface::GraphicsApi api);

    QRhi::Implementation rhiBackend() const { return m_rhiBackend; }
    QString rhiBackendName() const;
    QSGRendererInterface::GraphicsApi graphicsApi() const;

    QSurface::SurfaceType windowSurfaceType() const;

    const void *rifResource(QSGRendererInterface::Resource res,
                            const QSGDefaultRenderContext *rc,
                            const QQuickWindow *w);

    QOffscreenSurface *maybeCreateOffscreenSurface(QWindow *window);
    struct RhiCreateResult {
        QRhi *rhi;
        bool own;
    };
    RhiCreateResult createRhi(QQuickWindow *window, QSurface *offscreenSurface, bool forcePreferSwRenderer = false);
    void destroyRhi(QRhi *rhi, const QQuickGraphicsConfiguration &config);
    void prepareWindowForRhi(QQuickWindow *window);

    QImage grabOffscreen(QQuickWindow *window);
#ifdef Q_OS_WEBOS
    QImage grabOffscreenForProtectedContent(QQuickWindow *window);
#endif

    void applySwapChainFormat(QRhiSwapChain *scWithWindowSet, QQuickWindow *window);

    QRhiTexture::Format toRhiTextureFormat(uint nativeFormat, QRhiTexture::Flags *flags) const;

    bool attemptReinitWithSwRastUponFail() const;

private:
    QSGRhiSupport();
    void applySettings();
    void adjustToPlatformQuirks();
    void preparePipelineCache(QRhi *rhi, QQuickWindow *window);
    void finalizePipelineCache(QRhi *rhi, const QQuickGraphicsConfiguration &config);
    struct {
        bool valid = false;
        QSGRendererInterface::GraphicsApi api;
    } m_requested;
    bool m_settingsApplied = false;
    QRhi::Implementation m_rhiBackend = QRhi::Null;
};

QT_END_NAMESPACE

#endif // QSGRHISUPPORT_P_H
