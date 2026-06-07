// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QGRAPHICSSVGITEM_H
#define QGRAPHICSSVGITEM_H

#include <QtSvgWidgets/qtsvgwidgetsglobal.h>

#if !defined(QT_NO_GRAPHICSVIEW)

#include <QtWidgets/qgraphicsitem.h>

QT_BEGIN_NAMESPACE


class QSvgRenderer;
class QGraphicsSvgItemPrivate;

class Q_SVGWIDGETS_EXPORT QGraphicsSvgItem : public QGraphicsObject
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    Q_PROPERTY(QString elementId READ elementId WRITE setElementId)
    Q_PROPERTY(QSize maximumCacheSize READ maximumCacheSize WRITE setMaximumCacheSize)

public:
    QGraphicsSvgItem(QGraphicsItem *parentItem = nullptr);
    QGraphicsSvgItem(const QString &fileName, QGraphicsItem *parentItem = nullptr);

    void setSharedRenderer(QSvgRenderer *renderer);
    QSvgRenderer *renderer() const;

    void setElementId(const QString &id);
    QString elementId() const;

    void setCachingEnabled(bool);
    bool isCachingEnabled() const;

    void setMaximumCacheSize(const QSize &size);
    QSize maximumCacheSize() const;

    QRectF boundingRect() const override;

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    enum { Type = 13 };
    int type() const override;

private:
    Q_DISABLE_COPY(QGraphicsSvgItem)
    Q_DECLARE_PRIVATE_D(QGraphicsItem::d_ptr.data(), QGraphicsSvgItem)

    Q_PRIVATE_SLOT(d_func(), void _q_repaintItem())
};

QT_END_NAMESPACE

#endif // QT_NO_GRAPHICSVIEW

#endif // QGRAPHICSSVGITEM_H
