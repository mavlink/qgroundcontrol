// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKFRAMEBUFFEROBJECT_H
#define QQUICKFRAMEBUFFEROBJECT_H

#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObject;
class QQuickFramebufferObjectPrivate;
class QSGFramebufferObjectNode;

class Q_QUICK_EXPORT QQuickFramebufferObject : public QQuickItem
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickFramebufferObject)

    Q_PROPERTY(bool textureFollowsItemSize READ textureFollowsItemSize WRITE setTextureFollowsItemSize NOTIFY textureFollowsItemSizeChanged)
    Q_PROPERTY(bool mirrorVertically READ mirrorVertically WRITE setMirrorVertically NOTIFY mirrorVerticallyChanged)

public:

    class Q_QUICK_EXPORT Renderer {
    protected:
        Renderer();
        virtual ~Renderer();
        virtual void render() = 0;
        virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize &size);
        virtual void synchronize(QQuickFramebufferObject *);
        QOpenGLFramebufferObject *framebufferObject() const;
        void update();
        void invalidateFramebufferObject();
    private:
        friend class QSGFramebufferObjectNode;
        friend class QQuickFramebufferObject;
        void *data;
    };

    QQuickFramebufferObject(QQuickItem *parent = nullptr);

    bool textureFollowsItemSize() const;
    void setTextureFollowsItemSize(bool follows);

    bool mirrorVertically() const;
    void setMirrorVertically(bool enable);

    virtual Renderer *createRenderer() const = 0;

    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;
    void releaseResources() override;

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

Q_SIGNALS:
    void textureFollowsItemSizeChanged(bool);
    void mirrorVerticallyChanged(bool);

private Q_SLOTS:
    void invalidateSceneGraph();
};

QT_END_NAMESPACE

#endif // QQUICKFRAMEBUFFEROBJECT_H
