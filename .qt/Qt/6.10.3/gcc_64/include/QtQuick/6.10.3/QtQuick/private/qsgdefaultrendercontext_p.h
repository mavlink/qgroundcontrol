// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGDEFAULTRENDERCONTEXT_H
#define QSGDEFAULTRENDERCONTEXT_H

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

#include <QtQuick/private/qsgcontext_p.h>
#include <rhi/qshader.h>

QT_BEGIN_NAMESPACE

class QRhi;
class QRhiCommandBuffer;
class QRhiRenderPassDescriptor;
class QRhiResourceUpdateBatch;
class QRhiTexture;
class QRhiRenderBuffer;
class QSGMaterialShader;
class QSurface;
class QSGDefaultRenderContext;

namespace QSGRhiAtlasTexture {
    class Manager;
}

class QSGDepthStencilBuffer
{
public:
    QSGDepthStencilBuffer() { }
    QSGDepthStencilBuffer(QRhiRenderBuffer *ds) : ds(ds) { }
    ~QSGDepthStencilBuffer();
    QRhiRenderBuffer *ds = nullptr;
    QSGDefaultRenderContext *rc = nullptr;
};

class Q_QUICK_EXPORT QSGDefaultRenderContext : public QSGRenderContext
{
    Q_OBJECT
public:
    QSGDefaultRenderContext(QSGContext *context);

    QRhi *rhi() const override { return m_rhi; }
    bool isValid() const override { return m_rhi != nullptr; }

    static const int INIT_PARAMS_MAGIC = 0x50E;
    struct InitParams : public QSGRenderContext::InitParams {
        int sType = INIT_PARAMS_MAGIC; // help discovering broken code passing something else as 'context'
        QRhi *rhi = nullptr;
        int sampleCount = 1; // 1, 4, 8, ...
        // only used as a hint f.ex. in the texture atlas init
        QSize initialSurfacePixelSize;
        // The first window that will be used with this rc, if available.
        // Only a hint, to help picking better values for atlases.
        QSurface *maybeSurface = nullptr;
    };

    void initialize(const QSGRenderContext::InitParams *params) override;
    void invalidate() override;

    void prepareSync(qreal devicePixelRatio,
                     QRhiCommandBuffer *cb,
                     const QQuickGraphicsConfiguration &config) override;

    void beginNextFrame(QSGRenderer *renderer, const QSGRenderTarget &renderTarget,
                        RenderPassCallback mainPassRecordingStart,
                        RenderPassCallback mainPassRecordingEnd,
                        void *callbackUserData) override;
    void renderNextFrame(QSGRenderer *renderer) override;
    void endNextFrame(QSGRenderer *renderer) override;

    void preprocess() override;
    void invalidateGlyphCaches() override;
    void flushGlyphCaches() override;
    QSGDistanceFieldGlyphCache *distanceFieldGlyphCache(const QRawFont &font, int renderTypeQuality) override;
    QSGCurveGlyphAtlas *curveGlyphAtlas(const QRawFont &font) override;

    QSGTexture *createTexture(const QImage &image, uint flags) const override;
    QSGRenderer *createRenderer(QSGRendererInterface::RenderMode renderMode = QSGRendererInterface::RenderMode2D) override;
    QSGTexture *compressedTextureForFactory(const QSGCompressedTextureFactory *factory) const override;

    virtual void initializeRhiShader(QSGMaterialShader *shader, QShader::Variant shaderVariant);

    int maxTextureSize() const override { return m_maxTextureSize; }
    bool useDepthBufferFor2D() const { return m_useDepthBufferFor2D; }
    int msaaSampleCount() const { return m_initParams.sampleCount; }

    QRhiCommandBuffer *currentFrameCommandBuffer() const {
        // may be null if not in an active frame, but returning null is valid then
        return m_currentFrameCommandBuffer;
    }
    QRhiRenderPassDescriptor *currentFrameRenderPass() const {
        // may be null if not in an active frame, but returning null is valid then
        return m_currentFrameRenderPass;
    }

    qreal currentDevicePixelRatio() const
    {
        // Valid starting from QQuickWindow::syncSceneGraph(). This takes the
        // redirections, e.g. QQuickWindow::setRenderTarget(), into account.
        // This calculation logic matches what the renderer does, so this is
        // the same value that gets exposed in RenderState::devicePixelRatio()
        // to material shaders. This getter is useful to perform dpr-related
        // operations in the sync phase (in updatePaintNode()).
        return m_currentDevicePixelRatio;
    }

    QRhiResourceUpdateBatch *maybeGlyphCacheResourceUpdates();
    QRhiResourceUpdateBatch *glyphCacheResourceUpdates();
    void deferredReleaseGlyphCacheTexture(QRhiTexture *texture);
    void resetGlyphCacheResources();

    QSharedPointer<QSGDepthStencilBuffer> getDepthStencilBuffer(const QSize &size, int sampleCount);
    void addDepthStencilBuffer(const QSharedPointer<QSGDepthStencilBuffer> &ds);

protected:
    InitParams m_initParams;
    QRhi *m_rhi;
    int m_maxTextureSize;
    QSGRhiAtlasTexture::Manager *m_rhiAtlasManager;
    QRhiCommandBuffer *m_currentFrameCommandBuffer;
    QRhiRenderPassDescriptor *m_currentFrameRenderPass;
    qreal m_currentDevicePixelRatio;
    bool m_useDepthBufferFor2D;
    QRhiResourceUpdateBatch *m_glyphCacheResourceUpdates;
    QSet<QRhiTexture *> m_pendingGlyphCacheTextures;
    QHash<FontKey, QSGCurveGlyphAtlas *> m_curveGlyphAtlases;
    QHash<std::pair<QSize, int>, QWeakPointer<QSGDepthStencilBuffer>> m_depthStencilBuffers;

    friend class QSGDepthStencilBuffer;
};

QT_END_NAMESPACE

#endif // QSGDEFAULTRENDERCONTEXT_H
