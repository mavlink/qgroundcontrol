// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKPAINTEDITEM_P_H
#define QQUICKPAINTEDITEM_P_H

#include <QtQuick/qquickitem.h>
#include <QtGui/qcolor.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE

class QQuickPaintedItemPrivate;
class QPainter;
class Q_QUICK_EXPORT QQuickPaintedItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QSize contentsSize READ contentsSize WRITE setContentsSize NOTIFY contentsSizeChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(qreal contentsScale READ contentsScale WRITE setContentsScale NOTIFY contentsScaleChanged)
    Q_PROPERTY(RenderTarget renderTarget READ renderTarget WRITE setRenderTarget NOTIFY renderTargetChanged)
    Q_PROPERTY(QSize textureSize READ textureSize WRITE setTextureSize NOTIFY textureSizeChanged)

    QML_NAMED_ELEMENT(PaintedItem)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Cannot create instance of abstract class PaintedItem.")

public:
    explicit QQuickPaintedItem(QQuickItem *parent = nullptr);
    ~QQuickPaintedItem() override;

    enum RenderTarget {
        Image,
        FramebufferObject,
        InvertedYFramebufferObject
    };
    Q_ENUM(RenderTarget)

    enum PerformanceHint {
        FastFBOResizing = 0x1
    };
    Q_DECLARE_FLAGS(PerformanceHints, PerformanceHint)
    Q_FLAG(PerformanceHints)

    void update(const QRect &rect = QRect());

    bool opaquePainting() const;
    void setOpaquePainting(bool opaque);

    bool antialiasing() const;
    void setAntialiasing(bool enable);

    bool mipmap() const;
    void setMipmap(bool enable);

    PerformanceHints performanceHints() const;
    void setPerformanceHint(PerformanceHint hint, bool enabled = true);
    void setPerformanceHints(PerformanceHints hints);

    QRectF contentsBoundingRect() const;

    QSize contentsSize() const;
    void setContentsSize(const QSize &);
    void resetContentsSize();

    qreal contentsScale() const;
    void setContentsScale(qreal);

    QSize textureSize() const;
    void setTextureSize(const QSize &size);

    QColor fillColor() const;
    void setFillColor(const QColor&);

    RenderTarget renderTarget() const;
    void setRenderTarget(RenderTarget target);

    virtual void paint(QPainter *painter) = 0;

    bool isTextureProvider() const override;
    QSGTextureProvider *textureProvider() const override;

Q_SIGNALS:
    void fillColorChanged();
    void contentsSizeChanged();
    void contentsScaleChanged();
    void renderTargetChanged();
    void textureSizeChanged();

protected:
    QQuickPaintedItem(QQuickPaintedItemPrivate &dd, QQuickItem *parent = nullptr);
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void releaseResources() override;
    void itemChange(ItemChange, const ItemChangeData &) override;

private Q_SLOTS:
    void invalidateSceneGraph();

private:
    Q_DISABLE_COPY(QQuickPaintedItem)
    Q_DECLARE_PRIVATE(QQuickPaintedItem)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickPaintedItem::PerformanceHints)

QT_END_NAMESPACE

#endif // QQUICKPAINTEDITEM_P_H
