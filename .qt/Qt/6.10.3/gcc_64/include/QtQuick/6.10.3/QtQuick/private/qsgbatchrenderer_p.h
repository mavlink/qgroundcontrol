// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGBATCHRENDERER_P_H
#define QSGBATCHRENDERER_P_H

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

#include <private/qsgrenderer_p.h>
#include <private/qsgdefaultrendercontext_p.h>
#include <private/qsgnodeupdater_p.h>
#include <private/qsgrendernode_p.h>
#include <private/qdatabuffer_p.h>
#include <private/qsgtexture_p.h>

#include <QtCore/QBitArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QStack>

#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

namespace QSGBatchRenderer
{

#define QSG_RENDERER_COORD_LIMIT 1000000.0f

struct Vec;
struct Rect;
struct Buffer;
struct Chunk;
struct Batch;
struct Node;
class Updater;
class Renderer;
class ShaderManager;

template <typename Type, int PageSize> class AllocatorPage
{
public:
    // The memory used by this allocator
    char data[sizeof(Type) * PageSize];

    // 'blocks' contains a list of free indices which can be allocated.
    // The first available index is found in PageSize - available.
    int blocks[PageSize];

    // 'available' is the number of available instances this page has left to allocate.
    int available;

    // This is not strictly needed, but useful for sanity checking and anyway
    // pretty small..
    QBitArray allocated;

    AllocatorPage()
        : available(PageSize)
        , allocated(PageSize)
    {
        for (int i=0; i<PageSize; ++i)
            blocks[i] = i;

        // Zero out all new pages.
        memset(data, 0, sizeof(data));
    }

    const Type *at(uint index) const
    {
        return (Type *) &data[index * sizeof(Type)];
    }

    Type *at(uint index)
    {
        return (Type *) &data[index * sizeof(Type)];
    }
};

template <typename Type, int PageSize> class Allocator
{
public:
    Allocator()
    {
        pages.push_back(new AllocatorPage<Type, PageSize>());
    }

    ~Allocator()
    {
        qDeleteAll(pages);
    }

    Type *allocate()
    {
        AllocatorPage<Type, PageSize> *p = 0;
        for (int i = m_freePage; i < pages.size(); i++) {
            if (pages.at(i)->available > 0) {
                p = pages.at(i);
                m_freePage = i;
                break;
            }
        }

        // we couldn't find a free page from m_freePage to the last page.
        // either there is no free pages, or there weren't any in the area we
        // scanned: rescanning is expensive, so let's just assume there isn't
        // one. when an item is released, we'll reset m_freePage anyway.
        if (!p) {
            p = new AllocatorPage<Type, PageSize>();
            m_freePage = pages.size();
            pages.push_back(p);
        }
        uint pos = p->blocks[PageSize - p->available];
        void *mem = p->at(pos);
        p->available--;
        p->allocated.setBit(pos);
        Type *t = (Type*)mem;
        return t;
    }

    void releaseExplicit(uint pageIndex, uint index)
    {
        AllocatorPage<Type, PageSize> *page = pages.at(pageIndex);
        if (!page->allocated.testBit(index))
            qFatal("Double delete in allocator: page=%d, index=%d", pageIndex , index);

        // Zero this instance as we're done with it.
        void *mem = page->at(index);
        memset(mem, 0, sizeof(Type));

        page->allocated[index] = false;
        page->available++;
        page->blocks[PageSize - page->available] = index;

        // Remove the pages if they are empty and they are the last ones. We need to keep the
        // order of pages since we have references to their index, so we can only remove
        // from the end.
        while (page->available == PageSize && pages.size() > 1 && pages.back() == page) {
            pages.pop_back();
            delete page;
            page = pages.back();
        }

        // Reset the free page to force a scan for a new free point.
        m_freePage = 0;
    }

    void release(Type *t)
    {
        int pageIndex = -1;
        for (int i=0; i<pages.size(); ++i) {
            AllocatorPage<Type, PageSize> *p = pages.at(i);
            if ((Type *) (&p->data[0]) <= t && (Type *) (&p->data[PageSize * sizeof(Type)]) > t) {
                pageIndex = i;
                break;
            }
        }
        Q_ASSERT(pageIndex >= 0);

        AllocatorPage<Type, PageSize> *page = pages.at(pageIndex);
        int index = (quint64(t) - quint64(&page->data[0])) / sizeof(Type);

        releaseExplicit(pageIndex, index);
    }

    QVector<AllocatorPage<Type, PageSize> *> pages;
    int m_freePage = 0;
};


inline bool hasMaterialWithBlending(QSGGeometryNode *n)
{
    return (n->opaqueMaterial() ? n->opaqueMaterial()->flags() & QSGMaterial::Blending
                                : n->material()->flags() & QSGMaterial::Blending);
}

struct Pt {
    float x, y;

    void map(const QMatrix4x4 &mat) {
        Pt r;
        const float *m = mat.constData();
        r.x = x * m[0] + y * m[4] + m[12];
        r.y = x * m[1] + y * m[5] + m[13];
        x = r.x;
        y = r.y;
    }

    void set(float nx, float ny) {
        x = nx;
        y = ny;
    }
};

inline QDebug operator << (QDebug d, const Pt &p) {
    d << "Pt(" << p.x << p.y << ")";
    return d;
}



struct Rect {
    Pt tl, br; // Top-Left (min) and Bottom-Right (max)

    void operator |= (const Pt &pt) {
        if (pt.x < tl.x)
            tl.x = pt.x;
        if (pt.x > br.x)
            br.x = pt.x;
        if (pt.y < tl.y)
            tl.y = pt.y;
        if (pt.y > br.y)
            br.y = pt.y;
    }

    void operator |= (const Rect &r) {
        if (r.tl.x < tl.x)
            tl.x = r.tl.x;
        if (r.tl.y < tl.y)
            tl.y = r.tl.y;
        if (r.br.x > br.x)
            br.x = r.br.x;
        if (r.br.y > br.y)
            br.y = r.br.y;
    }

    void map(const QMatrix4x4 &m);

    void set(float left, float top, float right, float bottom) {
        tl.set(left, top);
        br.set(right, bottom);
    }

    bool intersects(const Rect &r) {
        bool xOverlap = r.tl.x < br.x && r.br.x > tl.x;
        bool yOverlap = r.tl.y < br.y && r.br.y > tl.y;
        return xOverlap && yOverlap;
    }

    bool isOutsideFloatRange() const {
        return tl.x < -QSG_RENDERER_COORD_LIMIT
                || tl.y < -QSG_RENDERER_COORD_LIMIT
                || br.x > QSG_RENDERER_COORD_LIMIT
                || br.y > QSG_RENDERER_COORD_LIMIT;
    }
};

inline QDebug operator << (QDebug d, const Rect &r) {
    d << "Rect(" << r.tl.x << r.tl.y << r.br.x << r.br.y << ")";
    return d;
}

struct Buffer {
    quint32 size;
    // Data is only valid while preparing the upload. Exception is if we are using the
    // broken IBO workaround or we are using a visualization mode.
    char *data;
    QRhiBuffer *buf;
    uint nonDynamicChangeCount;
};

struct Element {
    Element()
        : boundsComputed(false)
        , boundsOutsideFloatRange(false)
        , translateOnlyToRoot(false)
        , removed(false)
        , orphaned(false)
        , isRenderNode(false)
        , isMaterialBlended(false)
    {
    }

    void setNode(QSGGeometryNode *n) {
        node = n;
        isMaterialBlended = hasMaterialWithBlending(n);
    }

    inline void ensureBoundsValid() {
        if (!boundsComputed)
            computeBounds();
    }
    void computeBounds();

    QSGGeometryNode *node = nullptr;
    Batch *batch = nullptr;
    Element *nextInBatch = nullptr;
    Node *root = nullptr;

    Rect bounds; // in device coordinates

    int order = 0;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiGraphicsPipeline *depthPostPassPs = nullptr;

    uint boundsComputed : 1;
    uint boundsOutsideFloatRange : 1;
    uint translateOnlyToRoot : 1;
    uint removed : 1;
    uint orphaned : 1;
    uint isRenderNode : 1;
    uint isMaterialBlended : 1;
};

struct RenderNodeElement : public Element {

    RenderNodeElement(QSGRenderNode *rn)
        : renderNode(rn)
    {
        isRenderNode = true;
    }

    QSGRenderNode *renderNode;
};

struct BatchRootInfo {
    BatchRootInfo() {}
    QSet<Node *> subRoots;
    Node *parentRoot = nullptr;
    int lastOrder = -1;
    int firstOrder = -1;
    int availableOrders = 0;
};

struct ClipBatchRootInfo : public BatchRootInfo
{
    QMatrix4x4 matrix;
};

struct DrawSet
{
    DrawSet(int v, int z, int i)
        : vertices(v)
        , zorders(z)
        , indices(i)
    {
    }
    DrawSet() {}
    int vertices = 0;
    int zorders = 0;
    int indices = 0;
    int indexCount = 0;
};

enum BatchCompatibility
{
    BatchBreaksOnCompare,
    BatchIsCompatible
};

struct ClipState
{
    enum ClipTypeBit
    {
        NoClip = 0x00,
        ScissorClip = 0x01,
        StencilClip = 0x02
    };
    Q_DECLARE_FLAGS(ClipType, ClipTypeBit)

    const QSGClipNode *clipList;
    ClipType type;
    QRhiScissor scissor;
    int stencilRef;

    inline void reset();
};

struct StencilClipState
{
    StencilClipState() : drawCalls(1) { }

    bool updateStencilBuffer = false;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;

    struct StencilDrawCall {
        int stencilRef;
        int vertexCount;
        int indexCount;
        QRhiCommandBuffer::IndexFormat indexFormat;
        quint32 vbufOffset;
        quint32 ibufOffset;
        quint32 ubufOffset;
    };
    QDataBuffer<StencilDrawCall> drawCalls;

    inline void reset();
};

struct Batch
{
    Batch() : drawSets(1) {}
    bool geometryWasChanged(QSGGeometryNode *gn);
    BatchCompatibility isMaterialCompatible(Element *e) const;
    void invalidate();
    void cleanupRemovedElements();

    bool isTranslateOnlyToRoot() const;
    bool isSafeToBatch() const;

    // pseudo-constructor...
    void init() {
        // Only non-reusable members are reset here. See Renderer::newBatch().
        first = nullptr;
        root = nullptr;
        vertexCount = 0;
        indexCount = 0;
        isOpaque = false;
        needsUpload = false;
        merged = false;
        positionAttribute = -1;
        uploadedThisFrame = false;
        isRenderNode = false;
        ubufDataValid = false;
        needsPurge = false;
        clipState.reset();
        blendConstant = QColor();
    }

    Element *first;
    Node *root;

    int positionAttribute;

    int vertexCount;
    int indexCount;

    int lastOrderInBatch;

    uint isOpaque : 1;
    uint needsUpload : 1;
    uint merged : 1;
    uint isRenderNode : 1;
    uint ubufDataValid : 1;
    uint needsPurge : 1;

    mutable uint uploadedThisFrame : 1; // solely for debugging purposes

    Buffer vbo;
    Buffer ibo;
    QRhiBuffer *ubuf;
    ClipState clipState;
    StencilClipState stencilClipState;
    QColor blendConstant;

    QDataBuffer<DrawSet> drawSets;
};

// NOTE: Node is zero-initialized by the Allocator.
struct Node
{
    QSGNode *sgNode;
    void *data;

    Node *m_parent;
    Node *m_child;
    Node *m_next;
    Node *m_prev;

    Node *parent() const { return m_parent; }

    void append(Node *child) {
        Q_ASSERT(child);
        Q_ASSERT(!hasChild(child));
        Q_ASSERT(child->m_parent == nullptr);
        Q_ASSERT(child->m_next == nullptr);
        Q_ASSERT(child->m_prev == nullptr);

        if (!m_child) {
            child->m_next = child;
            child->m_prev = child;
            m_child = child;
        } else {
            m_child->m_prev->m_next = child;
            child->m_prev = m_child->m_prev;
            m_child->m_prev = child;
            child->m_next = m_child;
        }
        child->setParent(this);
    }

    void remove(Node *child) {
        Q_ASSERT(child);
        Q_ASSERT(hasChild(child));

        // only child..
        if (child->m_next == child) {
            m_child = nullptr;
        } else {
            if (m_child == child)
                m_child = child->m_next;
            child->m_next->m_prev = child->m_prev;
            child->m_prev->m_next = child->m_next;
        }
        child->m_next = nullptr;
        child->m_prev = nullptr;
        child->setParent(nullptr);
    }

    Node *firstChild() const { return m_child; }

    Node *sibling() const {
        Q_ASSERT(m_parent);
        return m_next == m_parent->m_child ? nullptr : m_next;
    }

    void setParent(Node *p) {
        Q_ASSERT(m_parent == nullptr || p == nullptr);
        m_parent = p;
    }

    bool hasChild(Node *child) const {
        Node *n = m_child;
        while (n && n != child)
            n = n->sibling();
        return n;
    }



    QSGNode::DirtyState dirtyState;

    uint isOpaque : 1;
    uint isBatchRoot : 1;
    uint becameBatchRoot : 1;

    inline QSGNode::NodeType type() const { return sgNode->type(); }

    inline Element *element() const {
        Q_ASSERT(sgNode->type() == QSGNode::GeometryNodeType);
        return (Element *) data;
    }

    inline RenderNodeElement *renderNodeElement() const {
        Q_ASSERT(sgNode->type() == QSGNode::RenderNodeType);
        return (RenderNodeElement *) data;
    }

    inline ClipBatchRootInfo *clipInfo() const {
        Q_ASSERT(sgNode->type() == QSGNode::ClipNodeType);
        return (ClipBatchRootInfo *) data;
    }

    inline BatchRootInfo *rootInfo() const {
        Q_ASSERT(sgNode->type() == QSGNode::ClipNodeType
                 || (sgNode->type() == QSGNode::TransformNodeType && isBatchRoot));
        return (BatchRootInfo *) data;
    }
};

class Updater : public QSGNodeUpdater
{
public:
    Updater(Renderer *r);

    void visitOpacityNode(Node *n);
    void visitTransformNode(Node *n);
    void visitGeometryNode(Node *n);
    void visitClipNode(Node *n);
    void updateRootTransforms(Node *n);
    void updateRootTransforms(Node *n, Node *root, const QMatrix4x4 &combined);

    void updateStates(QSGNode *n) override;
    void visitNode(Node *n);
    void registerWithParentRoot(QSGNode *subRoot, QSGNode *parentRoot);

private:
    Renderer *renderer;

    QDataBuffer<Node *> m_roots;
    QDataBuffer<QMatrix4x4> m_rootMatrices;

    int m_added;
    int m_transformChange;
    int m_opacityChange;

    QMatrix4x4 m_identityMatrix;
};

struct GraphicsState
{
    bool depthTest = false;
    bool depthWrite = false;
    QRhiGraphicsPipeline::CompareOp depthFunc = QRhiGraphicsPipeline::Less;
    bool blending = false;
    QRhiGraphicsPipeline::BlendFactor srcColor = QRhiGraphicsPipeline::One;
    QRhiGraphicsPipeline::BlendFactor dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    QRhiGraphicsPipeline::BlendFactor srcAlpha = QRhiGraphicsPipeline::One;
    QRhiGraphicsPipeline::BlendFactor dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    QRhiGraphicsPipeline::BlendOp opColor = QRhiGraphicsPipeline::Add;
    QRhiGraphicsPipeline::BlendOp opAlpha = QRhiGraphicsPipeline::Add;
    QRhiGraphicsPipeline::ColorMask colorWrite = QRhiGraphicsPipeline::ColorMask(0xF);
    QRhiGraphicsPipeline::CullMode cullMode = QRhiGraphicsPipeline::None;
    bool usesScissor = false;
    bool stencilTest = false;
    int sampleCount = 1;
    QSGGeometry::DrawingMode drawMode = QSGGeometry::DrawTriangles;
    float lineWidth = 1.0f;
    QRhiGraphicsPipeline::PolygonMode polygonMode = QRhiGraphicsPipeline::Fill;
    int multiViewCount = 0;
};

bool operator==(const GraphicsState &a, const GraphicsState &b) noexcept;
bool operator!=(const GraphicsState &a, const GraphicsState &b) noexcept;
size_t qHash(const GraphicsState &s, size_t seed = 0) noexcept;

struct ShaderManagerShader;

struct GraphicsPipelineStateKey
{
    GraphicsState state;
    const ShaderManagerShader *sms;
    QVector<quint32> renderTargetDescription;
    QVector<quint32> srbLayoutDescription;
    struct {
        size_t renderTargetDescriptionHash;
        size_t srbLayoutDescriptionHash;
    } extra;
    static GraphicsPipelineStateKey create(const GraphicsState &state,
                                           const ShaderManagerShader *sms,
                                           const QRhiRenderPassDescriptor *rpDesc,
                                           const QRhiShaderResourceBindings *srb)
    {
        const QVector<quint32> rtDesc = rpDesc->serializedFormat();
        const QVector<quint32> srbDesc = srb->serializedLayoutDescription();
        return { state, sms, rtDesc, srbDesc, { qHash(rtDesc), qHash(srbDesc) } };
    }
};

bool operator==(const GraphicsPipelineStateKey &a, const GraphicsPipelineStateKey &b) noexcept;
bool operator!=(const GraphicsPipelineStateKey &a, const GraphicsPipelineStateKey &b) noexcept;
size_t qHash(const GraphicsPipelineStateKey &k, size_t seed = 0) noexcept;

struct ShaderKey
{
    QSGMaterialType *type;
    QSGRendererInterface::RenderMode renderMode;
    int multiViewCount;
};

bool operator==(const ShaderKey &a, const ShaderKey &b) noexcept;
bool operator!=(const ShaderKey &a, const ShaderKey &b) noexcept;
size_t qHash(const ShaderKey &k, size_t seed = 0) noexcept;

struct ShaderManagerShader
{
    ~ShaderManagerShader() {
        delete materialShader;
    }
    QSGMaterialShader *materialShader = nullptr;
    QRhiVertexInputLayout inputLayout;
    QVarLengthArray<QRhiShaderStage, 2> stages;
    float lastOpacity;
};

class ShaderManager : public QObject
{
    Q_OBJECT
public:
    using Shader = ShaderManagerShader;

    ShaderManager(QSGDefaultRenderContext *ctx) : context(ctx) { }
    ~ShaderManager() {
        qDeleteAll(rewrittenShaders);
        qDeleteAll(stockShaders);
    }

    void clearCachedRendererData();

    QHash<GraphicsPipelineStateKey, QRhiGraphicsPipeline *> pipelineCache;

    QMultiHash<QVector<quint32>, QRhiShaderResourceBindings *> srbPool;
    QVector<quint32> srbLayoutDescSerializeWorkspace;

public Q_SLOTS:
    void invalidated();

public:
    Shader *prepareMaterial(QSGMaterial *material,
                            const QSGGeometry *geometry = nullptr,
                            QSGRendererInterface::RenderMode renderMode = QSGRendererInterface::RenderMode2D,
                            int multiViewCount = 0);
    Shader *prepareMaterialNoRewrite(QSGMaterial *material,
                                     const QSGGeometry *geometry = nullptr,
                                     QSGRendererInterface::RenderMode renderMode = QSGRendererInterface::RenderMode2D,
                                     int multiViewCount = 0);

private:
    QHash<ShaderKey, Shader *> rewrittenShaders;
    QHash<ShaderKey, Shader *> stockShaders;

    QSGDefaultRenderContext *context;
};

struct RenderPassState
{
    QRhiViewport viewport;
    QColor clearColor;
    QRhiDepthStencilClearValue dsClear;
    bool viewportSet;
    bool scissorSet;
};

class Visualizer
{
public:
    enum VisualizeMode {
        VisualizeNothing,
        VisualizeBatches,
        VisualizeClipping,
        VisualizeChanges,
        VisualizeOverdraw
    };

    Visualizer(Renderer *renderer);
    virtual ~Visualizer();

    VisualizeMode mode() const { return m_visualizeMode; }
    void setMode(VisualizeMode mode) { m_visualizeMode = mode; }

    virtual void visualizeChangesPrepare(Node *n, uint parentChanges = 0);
    virtual void prepareVisualize() = 0;
    virtual void visualize() = 0;

    virtual void releaseResources() = 0;

protected:
    Renderer *m_renderer;
    VisualizeMode m_visualizeMode;
    QHash<Node *, uint> m_visualizeChangeSet;
};

class Q_QUICK_EXPORT Renderer : public QSGRenderer
{
public:
    Renderer(QSGDefaultRenderContext *ctx, QSGRendererInterface::RenderMode renderMode = QSGRendererInterface::RenderMode2D);
    ~Renderer();

protected:
    void nodeChanged(QSGNode *node, QSGNode::DirtyState state) override;
    void render() override;
    void prepareInline() override;
    void renderInline() override;
    void releaseCachedResources() override;

    struct PreparedRenderBatch {
        const Batch *batch;
        ShaderManager::Shader *sms;
    };

    struct RenderPassContext {
        bool valid = false;
        QVarLengthArray<PreparedRenderBatch, 64> opaqueRenderBatches;
        QVarLengthArray<PreparedRenderBatch, 64> alphaRenderBatches;
        QElapsedTimer timer;
        quint64 timeRenderLists;
        quint64 timePrepareOpaque;
        quint64 timePrepareAlpha;
        quint64 timeSorting;
        quint64 timeUploadOpaque;
        quint64 timeUploadAlpha;
    };

    // update batches and queue and commit rhi resource updates
    void prepareRenderPass(RenderPassContext *ctx);
    // records the beginPass()
    void beginRenderPass(RenderPassContext *ctx);
    // records the draw calls, must be preceded by a prepareRenderPass at minimum,
    // and also surrounded by begin/endRenderPass unless we are recording inside an
    // already started pass.
    void recordRenderPass(RenderPassContext *ctx);
    // does visualizing if enabled and records the endPass()
    void endRenderPass(RenderPassContext *ctx);

private:
    enum RebuildFlag {
        BuildRenderListsForTaggedRoots      = 0x0001,
        BuildRenderLists                    = 0x0002,
        BuildBatches                        = 0x0004,
        FullRebuild                         = 0xffff
    };

    friend class Updater;
    friend class RhiVisualizer;

    void destroyGraphicsResources();
    void map(Buffer *buffer, quint32 byteSize, bool isIndexBuf = false);
    void unmap(Buffer *buffer, bool isIndexBuf = false);

    void buildRenderListsFromScratch();
    void buildRenderListsForTaggedRoots();
    void tagSubRoots(Node *node);
    void buildRenderLists(QSGNode *node);

    void deleteRemovedElements();
    void cleanupBatches(QDataBuffer<Batch *> *batches);
    void prepareOpaqueBatches();
    bool checkOverlap(int first, int last, const Rect &bounds);
    void prepareAlphaBatches();
    void invalidateBatchAndOverlappingRenderOrders(Batch *batch);

    void uploadBatch(Batch *b);
    void uploadMergedElement(Element *e, int vaOffset, char **vertexData, char **zData, char **indexData, void *iBasePtr, int *indexCount);

    bool ensurePipelineState(Element *e, const ShaderManager::Shader *sms, bool depthPostPass = false);
    QRhiTexture *dummyTexture();
    void updateMaterialDynamicData(ShaderManager::Shader *sms, QSGMaterialShader::RenderState &renderState,
                                   QSGMaterial *material, const Batch *batch, Element *e, int ubufOffset, int ubufRegionSize,
                                   char *directUpdatePtr);
    void updateMaterialStaticData(ShaderManager::Shader *sms, QSGMaterialShader::RenderState &renderState,
                                  QSGMaterial *material, Batch *batch, bool *gstateChanged);
    void checkLineWidth(QSGGeometry *g);
    bool prepareRenderMergedBatch(Batch *batch, PreparedRenderBatch *renderBatch);
    void renderMergedBatch(PreparedRenderBatch *renderBatch, bool depthPostPass = false);
    bool prepareRenderUnmergedBatch(Batch *batch, PreparedRenderBatch *renderBatch);
    void renderUnmergedBatch(PreparedRenderBatch *renderBatch, bool depthPostPass = false);
    void setViewportAndScissors(QRhiCommandBuffer *cb, const Batch *batch);
    void setGraphicsPipeline(QRhiCommandBuffer *cb, const Batch *batch, Element *e, bool depthPostPass = false);
    ClipState::ClipType updateStencilClip(const QSGClipNode *clip);
    void updateClip(const QSGClipNode *clipList, const Batch *batch);
    void applyClipStateToGraphicsState();
    QRhiGraphicsPipeline *buildStencilPipeline(const Batch *batch, bool firstStencilClipInBatch);
    void updateClipState(const QSGClipNode *clipList, Batch *batch);
    void enqueueStencilDraw(const Batch *batch);
    const QMatrix4x4 &matrixForRoot(Node *node);
    void renderRenderNode(Batch *batch);
    bool prepareRhiRenderNode(Batch *batch, PreparedRenderBatch *renderBatch);
    void renderRhiRenderNode(const Batch *batch);
    void setActiveShader(QSGMaterialShader *program, ShaderManager::Shader *shader);
    void setActiveRhiShader(QSGMaterialShader *program, ShaderManager::Shader *shader);

    bool changeBatchRoot(Node *node, Node *newRoot);
    void registerBatchRoot(Node *childRoot, Node *parentRoot);
    void removeBatchRootFromParent(Node *childRoot);
    void nodeChangedBatchRoot(Node *node, Node *root);
    void turnNodeIntoBatchRoot(Node *node);
    void nodeWasTransformed(Node *node, int *vertexCount);
    void nodeWasRemoved(Node *node);
    void nodeWasAdded(QSGNode *node, Node *shadowParent);
    BatchRootInfo *batchRootInfo(Node *node);
    void updateLineWidth(QSGGeometry *g);

    inline Batch *newBatch();
    void invalidateAndRecycleBatch(Batch *b);
    void releaseElement(Element *e, bool inDestructor = false);

    void setVisualizationMode(const QByteArray &mode) override;
    bool hasVisualizationModeWithContinuousUpdate() const override;

    QSGDefaultRenderContext *m_context;
    QSGRendererInterface::RenderMode m_renderMode;
    QSet<Node *> m_taggedRoots;
    QDataBuffer<Element *> m_opaqueRenderList;
    QDataBuffer<Element *> m_alphaRenderList;
    int m_nextRenderOrder;
    bool m_partialRebuild;
    QSGNode *m_partialRebuildRoot;
    bool m_forceNoDepthBuffer;

    QHash<QSGRenderNode *, RenderNodeElement *> m_renderNodeElements;
    QDataBuffer<Batch *> m_opaqueBatches;
    QDataBuffer<Batch *> m_alphaBatches;
    QHash<QSGNode *, Node *> m_nodes;

    QDataBuffer<Batch *> m_batchPool;
    QDataBuffer<Element *> m_elementsToDelete;
    QDataBuffer<Element *> m_tmpAlphaElements;
    QDataBuffer<Element *> m_tmpOpaqueElements;

    QDataBuffer<QRhiBuffer *> m_vboPool;
    QDataBuffer<QRhiBuffer *> m_iboPool;
    quint32 m_vboPoolCost;
    quint32 m_iboPoolCost;

    uint m_rebuild;
    qreal m_zRange;
#if defined(QSGBATCHRENDERER_INVALIDATE_WEDGED_NODES)
    int m_renderOrderRebuildLower;
    int m_renderOrderRebuildUpper;
#endif

    int m_batchNodeThreshold;
    int m_batchVertexThreshold;
    int m_srbPoolThreshold;
    int m_bufferPoolSizeLimit;

    Visualizer *m_visualizer;

    ShaderManager *m_shaderManager; // per rendercontext, shared
    QSGMaterial *m_currentMaterial;
    QSGMaterialShader *m_currentProgram;
    ShaderManager::Shader *m_currentShader;
    ClipState m_currentClipState;

    QDataBuffer<char> m_vertexUploadPool;
    QDataBuffer<char> m_indexUploadPool;

    Allocator<Node, 256> m_nodeAllocator;
    Allocator<Element, 64> m_elementAllocator;

    RenderPassContext m_mainRenderPassContext;
    QRhiResourceUpdateBatch *m_resourceUpdates = nullptr;
    uint m_ubufAlignment;
    bool m_uint32IndexForRhi;
    GraphicsState m_gstate;
    RenderPassState m_pstate;
    QStack<GraphicsState> m_gstateStack;
    QHash<QSGSamplerDescription, QRhiSampler *> m_samplers;
    QRhiTexture *m_dummyTexture = nullptr;

    struct StencilClipCommonData {
        QRhiGraphicsPipeline *replacePs = nullptr;
        QRhiGraphicsPipeline *incrPs = nullptr;
        QShader vs;
        QShader fs;
        QRhiVertexInputLayout inputLayout;
        QRhiGraphicsPipeline::Topology topology;
        inline void reset();
    } m_stencilClipCommon;

    inline int mergedIndexElemSize() const;
    inline bool useDepthBuffer() const;
    inline void setStateForDepthPostPass();
};

Batch *Renderer::newBatch()
{
    Batch *b;
    int size = m_batchPool.size();
    if (size) {
        b = m_batchPool.at(size - 1);
        // vbo, ibo, ubuf, stencil-related buffers are reused
        m_batchPool.resize(size - 1);
    } else {
        b = new Batch();
        Q_ASSERT(offsetof(Batch, ibo) == sizeof(Buffer) + offsetof(Batch, vbo));
        memset(&b->vbo, 0, sizeof(Buffer) * 2); // Clear VBO & IBO
        b->ubuf = nullptr;
        b->stencilClipState.reset();
    }
    // initialize (when new batch) or reset (when reusing a batch) the non-reusable fields
    b->init();
    return b;
}

int Renderer::mergedIndexElemSize() const
{
    return m_uint32IndexForRhi ? sizeof(quint32) : sizeof(quint16);
}

// "use" here means that both depth test and write is wanted (the latter for
// opaque batches only). Therefore neither RenderMode2DNoDepthBuffer nor
// RenderMode3D must result in true. So while RenderMode3D requires a depth
// buffer, this here must say false. In addition, m_forceNoDepthBuffer is a
// dynamic override relevant with QSGRenderNode.
//
bool Renderer::useDepthBuffer() const
{
    return !m_forceNoDepthBuffer && m_renderMode == QSGRendererInterface::RenderMode2D;
}

void Renderer::setStateForDepthPostPass()
{
    m_gstate.colorWrite = {};
    m_gstate.depthWrite = true;
    m_gstate.depthTest = true;
    m_gstate.depthFunc = QRhiGraphicsPipeline::Less;
}

void Renderer::StencilClipCommonData::reset()
{
    delete replacePs;
    replacePs = nullptr;

    delete incrPs;
    incrPs = nullptr;

    vs = QShader();
    fs = QShader();
}

void ClipState::reset()
{
    clipList = nullptr;
    type = NoClip;
    stencilRef = 0;
}

void StencilClipState::reset()
{
    updateStencilBuffer = false;

    delete srb;
    srb = nullptr;

    delete vbuf;
    vbuf = nullptr;

    delete ibuf;
    ibuf = nullptr;

    delete ubuf;
    ubuf = nullptr;

    drawCalls.reset();
}

}

Q_DECLARE_TYPEINFO(QSGBatchRenderer::GraphicsState, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QSGBatchRenderer::GraphicsPipelineStateKey, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QSGBatchRenderer::RenderPassState, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QSGBatchRenderer::DrawSet, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QSGBATCHRENDERER_P_H
