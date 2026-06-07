// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSWIPEDELEGATE_P_H
#define QQUICKSWIPEDELEGATE_P_H

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

#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>

QT_BEGIN_NAMESPACE

class QQuickSwipe;
class QQuickSwipeDelegatePrivate;
class QQuickSwipeDelegateAttached;
class QQuickSwipeDelegateAttachedPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickSwipeDelegate : public QQuickItemDelegate
{
    Q_OBJECT
    Q_PROPERTY(QQuickSwipe *swipe READ swipe CONSTANT FINAL)
    QML_NAMED_ELEMENT(SwipeDelegate)
    QML_ATTACHED(QQuickSwipeDelegateAttached)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSwipeDelegate(QQuickItem *parent = nullptr);

    QQuickSwipe *swipe() const;

    enum Side { Left = 1, Right = -1 };
    Q_ENUM(Side)

    static QQuickSwipeDelegateAttached *qmlAttachedProperties(QObject *object);

protected:
    bool childMouseEventFilter(QQuickItem *child, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
    void touchEvent(QTouchEvent *event) override;

    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickSwipeDelegate)
    Q_DECLARE_PRIVATE(QQuickSwipeDelegate)
};

class Q_QUICKTEMPLATES2_EXPORT QQuickSwipeDelegateAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged FINAL)

public:
    explicit QQuickSwipeDelegateAttached(QObject *object = nullptr);

    bool isPressed() const;
    void setPressed(bool pressed);

Q_SIGNALS:
    void pressedChanged();
    void clicked();

private:
    Q_DISABLE_COPY(QQuickSwipeDelegateAttached)
    Q_DECLARE_PRIVATE(QQuickSwipeDelegateAttached)
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuickSwipeDelegate::Side)

#endif // QQUICKSWIPEDELEGATE_P_H
