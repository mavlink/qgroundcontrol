// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSWIPEVIEW_P_H
#define QQUICKSWIPEVIEW_P_H

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

#include <QtQuickTemplates2/private/qquickcontainer_p.h>

QT_REQUIRE_CONFIG(quicktemplates2_container);

QT_BEGIN_NAMESPACE

class QQuickSwipeViewAttached;
class QQuickSwipeViewPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickSwipeView : public QQuickContainer
{
    Q_OBJECT
    // 2.1 (Qt 5.8)
    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive NOTIFY interactiveChanged FINAL REVISION(2, 1))
    // 2.2 (Qt 5.9)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged FINAL REVISION(2, 2))
    // 2.3 (Qt 5.10)
    Q_PROPERTY(bool horizontal READ isHorizontal NOTIFY orientationChanged FINAL REVISION(2, 3))
    Q_PROPERTY(bool vertical READ isVertical NOTIFY orientationChanged FINAL REVISION(2, 3))
    QML_NAMED_ELEMENT(SwipeView)
    QML_ATTACHED(QQuickSwipeViewAttached)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSwipeView(QQuickItem *parent = nullptr);

    static QQuickSwipeViewAttached *qmlAttachedProperties(QObject *object);

    // 2.1 (Qt 5.8)
    bool isInteractive() const;
    void setInteractive(bool interactive);

    // 2.2 (Qt 5.9)
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    // 2.3 (Qt 5.10)
    bool isHorizontal() const;
    bool isVertical() const;

Q_SIGNALS:
    // 2.1 (Qt 5.8)
    Q_REVISION(2, 1) void interactiveChanged();
    // 2.2 (Qt 5.9)
    Q_REVISION(2, 2) void orientationChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemAdded(int index, QQuickItem *item) override;
    void itemMoved(int index, QQuickItem *item) override;
    void itemRemoved(int index, QQuickItem *item) override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickSwipeView)
    Q_DECLARE_PRIVATE(QQuickSwipeView)
};

class QQuickSwipeViewAttachedPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickSwipeViewAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index NOTIFY indexChanged FINAL)
    Q_PROPERTY(bool isCurrentItem READ isCurrentItem NOTIFY isCurrentItemChanged FINAL)
    Q_PROPERTY(QQuickSwipeView *view READ view NOTIFY viewChanged FINAL)
    // 2.1 (Qt 5.8)
    Q_PROPERTY(bool isNextItem READ isNextItem NOTIFY isNextItemChanged FINAL REVISION(2, 1))
    Q_PROPERTY(bool isPreviousItem READ isPreviousItem NOTIFY isPreviousItemChanged FINAL REVISION(2, 1))

public:
    explicit QQuickSwipeViewAttached(QObject *parent = nullptr);

    int index() const;
    bool isCurrentItem() const;
    QQuickSwipeView *view() const;

    // 2.1 (Qt 5.8)
    bool isNextItem() const;
    bool isPreviousItem() const;

Q_SIGNALS:
    void indexChanged();
    void isCurrentItemChanged();
    void viewChanged();
    // 2.1 (Qt 5.8)
    /*Q_REVISION(2, 1)*/ void isNextItemChanged();
    /*Q_REVISION(2, 1)*/ void isPreviousItemChanged();

private:
    Q_DISABLE_COPY(QQuickSwipeViewAttached)
    Q_DECLARE_PRIVATE(QQuickSwipeViewAttached)
};

QT_END_NAMESPACE

#endif // QQUICKSWIPEVIEW_P_H
