// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGRHIVISUALIZER_P_H
#define QSGRHIVISUALIZER_P_H

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

#include "qsgbatchrenderer_p.h"

#include <QtCore/qrandom.h>

QT_BEGIN_NAMESPACE

namespace QSGBatchRenderer
{

class RhiVisualizer : public Visualizer
{
public:
    RhiVisualizer(Renderer *renderer);
    ~RhiVisualizer();

    void prepareVisualize() override;
    void visualize() override;

    void releaseResources() override;

    struct DrawCall
    {
        static const int UBUF_SIZE = 152; // visualization.vert/frag
        struct {
            char data[UBUF_SIZE]; // matrix, rotation, color, pattern, projection
        } uniforms;
        struct {
            QRhiGraphicsPipeline::Topology topology;
            QRhiVertexInputAttribute::Format format;
            int count;
            int stride;
            const void *data; // only when using own vbuf
        } vertex;
        struct {
            QRhiCommandBuffer::IndexFormat format;
            int count;
            int stride;
            const void *data; // only when using own ibuf
        } index;
        struct {
            QRhiBuffer *vbuf; // either same for all draw calls and owned by the *Vis, or points to a Batch.Buffer.vbo.buf
            int vbufOffset;
            QRhiBuffer *ibuf; // same, but for index
            int ibufOffset;
            int ubufOffset;
        } buf;
    };

private:
    QShader m_vs;
    QShader m_fs;

    void recordDrawCalls(const QVector<DrawCall> &drawCalls,
                         QRhiCommandBuffer *cb,
                         QRhiShaderResourceBindings *srb,
                         bool blendOneOne = false);

    class PipelineCache {
    public:
        QRhiGraphicsPipeline *pipeline(RhiVisualizer *visualizer,
                                       QRhi *rhi,
                                       QRhiShaderResourceBindings *srb,
                                       QRhiRenderPassDescriptor *rpDesc,
                                       QRhiGraphicsPipeline::Topology topology,
                                       QRhiVertexInputAttribute::Format vertexFormat,
                                       quint32 vertexStride,
                                       bool blendOneOne);
        void releaseResources();
    private:
        struct Pipeline {
            QRhiGraphicsPipeline::Topology topology;
            QRhiVertexInputAttribute::Format format;
            quint32 stride;
            QRhiGraphicsPipeline *ps;
        };
        QVarLengthArray<Pipeline, 16> pipelines;
    };

    PipelineCache m_pipelines;

    class Fade {
    public:
        void prepare(RhiVisualizer *visualizer,
                     QRhi *rhi, QRhiResourceUpdateBatch *u, QRhiRenderPassDescriptor *rpDesc);
        void releaseResources();
        void render(QRhiCommandBuffer *cb);
    private:
        RhiVisualizer *visualizer;
        QRhiBuffer *vbuf = nullptr;
        QRhiBuffer *ubuf = nullptr;
        QRhiGraphicsPipeline *ps = nullptr;
        QRhiShaderResourceBindings *srb = nullptr;
    } m_fade;

    class ChangeVis {
    public:
        void prepare(Node *n, RhiVisualizer *visualizer,
                     QRhi *rhi, QRhiResourceUpdateBatch *u);
        void releaseResources();
        void render(QRhiCommandBuffer *cb);
    private:
        void gather(Node *n);
        RhiVisualizer *visualizer;
        QVector<DrawCall> drawCalls;
        QRhiBuffer *vbuf = nullptr;
        QRhiBuffer *ibuf = nullptr;
        QRhiBuffer *ubuf = nullptr;
        QRhiShaderResourceBindings *srb = nullptr;
    } m_changeVis;

    class BatchVis {
    public:
        void prepare(const QDataBuffer<Batch *> &opaqueBatches,
                     const QDataBuffer<Batch *> &alphaBatches,
                     RhiVisualizer *visualizer,
                     QRhi *rhi, QRhiResourceUpdateBatch *u,
                     bool forceUintIndex);
        void releaseResources();
        void render(QRhiCommandBuffer *cb);
    private:
        void gather(Batch *b);
        RhiVisualizer *visualizer;
        bool forceUintIndex;
        QVector<DrawCall> drawCalls;
        QRhiBuffer *ubuf = nullptr;
        QRhiShaderResourceBindings *srb = nullptr;
    } m_batchVis;

    class ClipVis {
    public:
        void prepare(QSGNode *node, RhiVisualizer *visualizer,
                     QRhi *rhi, QRhiResourceUpdateBatch *u);
        void releaseResources();
        void render(QRhiCommandBuffer *cb);
    private:
        void gather(QSGNode *node);
        RhiVisualizer *visualizer;
        QVector<DrawCall> drawCalls;
        QRhiBuffer *vbuf = nullptr;
        QRhiBuffer *ibuf = nullptr;
        QRhiBuffer *ubuf = nullptr;
        QRhiShaderResourceBindings *srb = nullptr;
    } m_clipVis;

    class OverdrawVis {
    public:
        void prepare(Node *n, RhiVisualizer *visualizer,
                     QRhi *rhi, QRhiResourceUpdateBatch *u);
        void releaseResources();
        void render(QRhiCommandBuffer *cb);
    private:
        void gather(Node *n);
        RhiVisualizer *visualizer;
        QVector<DrawCall> drawCalls;
        QRhiBuffer *vbuf = nullptr;
        QRhiBuffer *ibuf = nullptr;
        QRhiBuffer *ubuf = nullptr;
        QRhiShaderResourceBindings *srb = nullptr;
        float step = 0.0f;
        QMatrix4x4 rotation;
        struct {
            QRhiBuffer *vbuf = nullptr;
            QRhiBuffer *ubuf = nullptr;
            QRhiShaderResourceBindings *srb = nullptr;
            QRhiGraphicsPipeline *ps = nullptr;
        } box;
    } m_overdrawVis;

    QRandomGenerator m_randomGenerator;

    friend class Fade;
    friend class PipelineCache;
    friend class ChangeVis;
    friend class ClipVis;
    friend class OverdrawVis;
};

} // namespace QSGBatchRenderer

QT_END_NAMESPACE

#endif // QSGRHIVISUALIZER_P_H
