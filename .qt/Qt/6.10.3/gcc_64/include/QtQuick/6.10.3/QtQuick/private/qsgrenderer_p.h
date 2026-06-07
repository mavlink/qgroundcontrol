// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRENDERER_P_H
#define QSGRENDERER_P_H

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

#include "qsgabstractrenderer_p_p.h"
#include "qsgnode.h"
#include "qsgmaterial.h"

#include <QtQuick/private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

class QSGNodeUpdater;
class QRhiRenderTarget;
class QRhiCommandBuffer;
class QRhiRenderPassDescriptor;
class QRhiResourceUpdateBatch;

class Q_QUICK_EXPORT QSGRenderTarget
{
public:
    QSGRenderTarget() { }

    QSGRenderTarget(QRhiRenderTarget *rt,
                    QRhiRenderPassDescriptor *rpDesc,
                    QRhiCommandBuffer *cb)
        : rt(rt), rpDesc(rpDesc), cb(cb) { }

    explicit QSGRenderTarget(QPaintDevice *paintDevice)
        : paintDevice(paintDevice) { }

    QRhiRenderTarget *rt = nullptr;
    // Store the rp descriptor obj separately, it can (even if often it won't)
    // be different from rt->renderPassDescriptor(); e.g. one user is the 2D
    // integration in Quick 3D which will use a different, but compatible rp.
    QRhiRenderPassDescriptor *rpDesc = nullptr;
    QRhiCommandBuffer *cb = nullptr;

    QPaintDevice *paintDevice = nullptr;

    int multiViewCount = 0;
};

class Q_QUICK_EXPORT QSGRenderer : public QSGAbstractRenderer
{
public:
    QSGRenderer(QSGRenderContext *context);
    virtual ~QSGRenderer();

    // Accessed by QSGMaterial[Rhi]Shader::RenderState.
    QMatrix4x4 currentProjectionMatrix(int index) const { return m_current_projection_matrix[index]; }
    QMatrix4x4 currentModelViewMatrix() const { return m_current_model_view_matrix; }
    QMatrix4x4 currentCombinedMatrix(int index) const { return m_current_projection_matrix[index] * m_current_model_view_matrix; }
    qreal currentOpacity() const { return m_current_opacity; }
    qreal determinant() const { return m_current_determinant; }

    void setDevicePixelRatio(qreal ratio) { m_device_pixel_ratio = ratio; }
    qreal devicePixelRatio() const { return m_device_pixel_ratio; }
    QSGRenderContext *context() const { return m_context; }

    bool isMirrored() const;
    void renderScene() override;
    void prepareSceneInline() override;
    void renderSceneInline() override;
    void nodeChanged(QSGNode *node, QSGNode::DirtyState state) override;

    QSGNodeUpdater *nodeUpdater() const;
    void setNodeUpdater(QSGNodeUpdater *updater);
    inline QSGMaterialShader::RenderState state(QSGMaterialShader::RenderState::DirtyStates dirty) const;
    virtual void setVisualizationMode(const QByteArray &) { }
    virtual bool hasVisualizationModeWithContinuousUpdate() const { return false; }
    virtual void releaseCachedResources() { }

    void clearChangedFlag() { m_changed_emitted = false; }

    // Accessed by QSGMaterialShader::RenderState.
    QByteArray *currentUniformData() const { return m_current_uniform_data; }
    QRhiResourceUpdateBatch *currentResourceUpdateBatch() const { return m_current_resource_update_batch; }
    QRhi *currentRhi() const { return m_rhi; }

    void setRenderTarget(const QSGRenderTarget &rt) { m_rt = rt; }
    const QSGRenderTarget &renderTarget() const { return m_rt; }

    void setRenderPassRecordingCallbacks(QSGRenderContext::RenderPassCallback start,
                                         QSGRenderContext::RenderPassCallback end,
                                         void *userData)
    {
        m_renderPassRecordingCallbacks.start = start;
        m_renderPassRecordingCallbacks.end = end;
        m_renderPassRecordingCallbacks.userData = userData;
    }

protected:
    virtual void render() = 0;

    virtual void prepareInline();
    virtual void renderInline();

    virtual void preprocess();

    void addNodesToPreprocess(QSGNode *node);
    void removeNodesToPreprocess(QSGNode *node);

    QVarLengthArray<QMatrix4x4, 1> m_current_projection_matrix; // includes adjustment, where applicable, so can be treated as Y up in NDC always
    QVarLengthArray<QMatrix4x4, 1> m_current_projection_matrix_native_ndc; // Vulkan has Y down in normalized device coordinates, others Y up...
    QMatrix4x4 m_current_model_view_matrix;
    qreal m_current_opacity;
    qreal m_current_determinant;
    qreal m_device_pixel_ratio;

    QSGRenderContext *m_context;

    QByteArray *m_current_uniform_data;
    QRhiResourceUpdateBatch *m_current_resource_update_batch;
    QRhi *m_rhi;
    QSGRenderTarget m_rt;
    struct {
        QSGRenderContext::RenderPassCallback start = nullptr;
        QSGRenderContext::RenderPassCallback end = nullptr;
        void *userData = nullptr;
    } m_renderPassRecordingCallbacks;

private:
    QSGNodeUpdater *m_node_updater;

    QSet<QSGNode *> m_nodes_to_preprocess;
    QSet<QSGNode *> m_nodes_dont_preprocess;

    uint m_changed_emitted : 1;
    uint m_is_rendering : 1;
    uint m_is_preprocessing : 1;
};

QSGMaterialShader::RenderState QSGRenderer::state(QSGMaterialShader::RenderState::DirtyStates dirty) const
{
    QSGMaterialShader::RenderState s;
    s.m_dirty = dirty;
    s.m_data = this;
    return s;
}


class Q_QUICK_EXPORT QSGNodeDumper : public QSGNodeVisitor {

public:
    static void dump(QSGNode *n);

    QSGNodeDumper() {}
    void visitNode(QSGNode *n) override;
    void visitChildren(QSGNode *n) override;

private:
    int m_indent = 0;
};

QT_END_NAMESPACE

#endif
