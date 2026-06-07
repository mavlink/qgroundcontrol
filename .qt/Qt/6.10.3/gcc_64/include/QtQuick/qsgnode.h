// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGNODE_H
#define QSGNODE_H

#include <QtCore/qlist.h>
#include <QtQuick/qsggeometry.h>
#include <QtGui/QMatrix4x4>

#include <float.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG
#define QSG_RUNTIME_DESCRIPTION
#endif

class QSGAbstractRenderer;
class QSGRenderer;

class QSGNode;
class QSGRootNode;
class QSGGeometryNode;
class QSGTransformNode;
class QSGClipNode;
class QSGNodePrivate;
class QSGBasicGeometryNodePrivate;
class QSGGeometryNodePrivate;

namespace QSGBatchRenderer {
    class Renderer;
    class Updater;
}

class Q_QUICK_EXPORT QSGNode
{
public:
    enum NodeType {
        BasicNodeType,
        GeometryNodeType,
        TransformNodeType,
        ClipNodeType,
        OpacityNodeType,
        RootNodeType,
        RenderNodeType
    };

    enum Flag {
        // Lower 16 bites reserved for general node
        OwnedByParent               = 0x0001,
        UsePreprocess               = 0x0002,

        // 0x00ff0000 bits reserved for node subclasses

        // QSGBasicGeometryNode
        OwnsGeometry                = 0x00010000,
        OwnsMaterial                = 0x00020000,
        OwnsOpaqueMaterial          = 0x00040000,

        // Uppermost 8 bits are reserved for internal use.
        IsVisitableNode             = 0x01000000
#ifdef Q_QDOC
        , InternalReserved            = 0x01000000
#endif
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum DirtyStateBit {
        DirtySubtreeBlocked         = 0x0080,
        DirtyMatrix                 = 0x0100,
        DirtyNodeAdded              = 0x0400,
        DirtyNodeRemoved            = 0x0800,
        DirtyGeometry               = 0x1000,
        DirtyMaterial               = 0x2000,
        DirtyOpacity                = 0x4000,

        DirtyForceUpdate            = 0x8000,

        DirtyUsePreprocess          = UsePreprocess,

        DirtyPropagationMask        = DirtyMatrix
                                      | DirtyNodeAdded
                                      | DirtyOpacity
                                      | DirtyForceUpdate

    };
    Q_DECLARE_FLAGS(DirtyState, DirtyStateBit)

    QSGNode();
    virtual ~QSGNode();

    QSGNode *parent() const { return m_parent; }

    void removeChildNode(QSGNode *node);
    void removeAllChildNodes();
    void prependChildNode(QSGNode *node);
    void appendChildNode(QSGNode *node);
    void insertChildNodeBefore(QSGNode *node, QSGNode *before);
    void insertChildNodeAfter(QSGNode *node, QSGNode *after);
    void reparentChildNodesTo(QSGNode *newParent);

    int childCount() const;
    QSGNode *childAtIndex(int i) const;
    QSGNode *firstChild() const { return m_firstChild; }
    QSGNode *lastChild() const { return m_lastChild; }
    QSGNode *nextSibling() const { return m_nextSibling; }
    QSGNode* previousSibling() const { return m_previousSibling; }

    inline NodeType type() const { return m_type; }

    QT_DEPRECATED void clearDirty() { }
    void markDirty(DirtyState bits);
    QT_DEPRECATED DirtyState dirtyState() const { return { }; }

    virtual bool isSubtreeBlocked() const;

    Flags flags() const { return m_nodeFlags; }
    void setFlag(Flag, bool = true);
    void setFlags(Flags, bool = true);

    virtual void preprocess() { }

protected:
    QSGNode(NodeType type);
    QSGNode(QSGNodePrivate &dd, NodeType type);

private:
    friend class QSGRootNode;
    friend class QSGBatchRenderer::Renderer;
    friend class QSGRenderer;

    void init();
    void destroy();

    QSGNode *m_parent = nullptr;
    NodeType m_type = BasicNodeType;
    QSGNode *m_firstChild = nullptr;
    QSGNode *m_lastChild = nullptr;
    QSGNode *m_nextSibling = nullptr;
    QSGNode *m_previousSibling = nullptr;
    int m_subtreeRenderableCount = 0;

    Flags m_nodeFlags;

protected:
    friend class QSGNodePrivate;

    QScopedPointer<QSGNodePrivate> d_ptr;
};

void Q_QUICK_EXPORT qsgnode_set_description(QSGNode *node, const QString &description);

class Q_QUICK_EXPORT QSGBasicGeometryNode : public QSGNode
{
public:
    ~QSGBasicGeometryNode() override;

    void setGeometry(QSGGeometry *geometry);
    const QSGGeometry *geometry() const { return m_geometry; }
    QSGGeometry *geometry() { return m_geometry; }

    const QMatrix4x4 *matrix() const { return m_matrix; }
    const QSGClipNode *clipList() const { return m_clip_list; }

    void setRendererMatrix(const QMatrix4x4 *m) { m_matrix = m; }
    void setRendererClipList(const QSGClipNode *c) { m_clip_list = c; }

protected:
    QSGBasicGeometryNode(NodeType type);
    QSGBasicGeometryNode(QSGBasicGeometryNodePrivate &dd, NodeType type);

private:
    friend class QSGNodeUpdater;

    QSGGeometry *m_geometry;

    Q_DECL_UNUSED_MEMBER int m_reserved_start_index;
    Q_DECL_UNUSED_MEMBER int m_reserved_end_index;

    const QMatrix4x4 *m_matrix;
    const QSGClipNode *m_clip_list;
};

class QSGMaterial;

class Q_QUICK_EXPORT QSGGeometryNode : public QSGBasicGeometryNode
{
public:
    QSGGeometryNode();
    ~QSGGeometryNode() override;

    void setMaterial(QSGMaterial *material);
    QSGMaterial *material() const { return m_material; }

    void setOpaqueMaterial(QSGMaterial *material);
    QSGMaterial *opaqueMaterial() const { return m_opaque_material; }

    QSGMaterial *activeMaterial() const;

    void setRenderOrder(int order);
    int renderOrder() const { return m_render_order; }

    void setInheritedOpacity(qreal opacity);
    qreal inheritedOpacity() const { return m_opacity; }

protected:
    QSGGeometryNode(QSGGeometryNodePrivate &dd);

private:
    friend class QSGNodeUpdater;

    int m_render_order = 0;
    QSGMaterial *m_material = nullptr;
    QSGMaterial *m_opaque_material = nullptr;

    qreal m_opacity = 1;
};

class Q_QUICK_EXPORT QSGClipNode : public QSGBasicGeometryNode
{
public:
    QSGClipNode();
    ~QSGClipNode() override;

    void setIsRectangular(bool rectHint);
    bool isRectangular() const { return m_is_rectangular; }

    void setClipRect(const QRectF &);
    QRectF clipRect() const { return m_clip_rect; }

private:
    uint m_is_rectangular : 1;
    uint m_reserved : 31;

    QRectF m_clip_rect;
};


class Q_QUICK_EXPORT QSGTransformNode : public QSGNode
{
public:
    QSGTransformNode();
    ~QSGTransformNode() override;

    void setMatrix(const QMatrix4x4 &matrix);
    const QMatrix4x4 &matrix() const { return m_matrix; }

    void setCombinedMatrix(const QMatrix4x4 &matrix);
    const QMatrix4x4 &combinedMatrix() const { return m_combined_matrix; }

private:
    QMatrix4x4 m_matrix;
    QMatrix4x4 m_combined_matrix;
};


class Q_QUICK_EXPORT QSGRootNode : public QSGNode
{
public:
    QSGRootNode();
    ~QSGRootNode() override;

private:
    void notifyNodeChange(QSGNode *node, DirtyState state);

    friend class QSGAbstractRenderer;
    friend class QSGNode;
    friend class QSGGeometryNode;

    QList<QSGAbstractRenderer *> m_renderers;
};


class Q_QUICK_EXPORT QSGOpacityNode : public QSGNode
{
public:
    QSGOpacityNode();
    ~QSGOpacityNode() override;

    void setOpacity(qreal opacity);
    qreal opacity() const { return m_opacity; }

    void setCombinedOpacity(qreal opacity);
    qreal combinedOpacity() const { return m_combined_opacity; }

    bool isSubtreeBlocked() const override;

private:
    qreal m_opacity = 1;
    qreal m_combined_opacity = 1;
};

class Q_QUICK_EXPORT QSGNodeVisitor {
public:
    virtual ~QSGNodeVisitor();

protected:
    virtual void enterTransformNode(QSGTransformNode *) {}
    virtual void leaveTransformNode(QSGTransformNode *) {}
    virtual void enterClipNode(QSGClipNode *) {}
    virtual void leaveClipNode(QSGClipNode *) {}
    virtual void enterGeometryNode(QSGGeometryNode *) {}
    virtual void leaveGeometryNode(QSGGeometryNode *) {}
    virtual void enterOpacityNode(QSGOpacityNode *) {}
    virtual void leaveOpacityNode(QSGOpacityNode *) {}
    virtual void visitNode(QSGNode *n);
    virtual void visitChildren(QSGNode *n);
};

#ifndef QT_NO_DEBUG_STREAM
Q_QUICK_EXPORT QDebug operator<<(QDebug, const QSGNode *n);
Q_QUICK_EXPORT QDebug operator<<(QDebug, const QSGGeometryNode *n);
Q_QUICK_EXPORT QDebug operator<<(QDebug, const QSGTransformNode *n);
Q_QUICK_EXPORT QDebug operator<<(QDebug, const QSGOpacityNode *n);
Q_QUICK_EXPORT QDebug operator<<(QDebug, const QSGRootNode *n);

#endif

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGNode::DirtyState)
Q_DECLARE_OPERATORS_FOR_FLAGS(QSGNode::Flags)

QT_END_NAMESPACE

#endif // QSGNODE_H
