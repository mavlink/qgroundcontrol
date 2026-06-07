// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRECTANGULARSHADOW_P_H
#define QQUICKRECTANGULARSHADOW_P_H

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

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_shadereffect);

#include "qtquickeffectsglobal_p.h"
#include <QtQuick/qquickitem.h>
#include <QtGui/qcolor.h>
#include <QtGui/qvector2d.h>
#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE

class QQuickRectangularShadowPrivate;

class Q_QUICKEFFECTS_EXPORT QQuickRectangularShadow : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVector2D offset READ offset WRITE setOffset NOTIFY offsetChanged FINAL)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(qreal blur READ blur WRITE setBlur NOTIFY blurChanged FINAL)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(qreal spread READ spread WRITE setSpread NOTIFY spreadChanged FINAL)
    Q_PROPERTY(bool cached READ isCached WRITE setCached NOTIFY cachedChanged FINAL)
    Q_PROPERTY(QQuickItem *material READ material WRITE setMaterial NOTIFY materialChanged FINAL)
    QML_NAMED_ELEMENT(RectangularShadow)
    QML_ADDED_IN_VERSION(6, 9)
public:
    QQuickRectangularShadow(QQuickItem *parent = nullptr);

    QVector2D offset() const;
    void setOffset(const QVector2D &offset);
    QColor color() const;
    void setColor(const QColor &color);
    qreal blur() const;
    void setBlur(qreal blur);
    qreal radius() const;
    void setRadius(qreal radius);
    qreal spread() const;
    void setSpread(qreal spread);
    bool isCached() const;
    void setCached(bool cached);
    QQuickItem *material() const;
    void setMaterial(QQuickItem *item);

Q_SIGNALS:
    void offsetChanged();
    void colorChanged();
    void blurChanged();
    void radiusChanged();
    void spreadChanged();
    void cachedChanged();
    void materialChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    Q_DECLARE_PRIVATE(QQuickRectangularShadow)
};

QT_END_NAMESPACE

#endif // QQUICKRECTANGULARSHADOW_P_H
