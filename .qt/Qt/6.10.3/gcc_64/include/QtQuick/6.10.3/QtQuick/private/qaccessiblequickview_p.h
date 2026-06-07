// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QAccessibleQuickView_H
#define QAccessibleQuickView_H

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

#include <QtGui/qaccessibleobject.h>
#include <QtQuick/qquickwindow.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class Q_QUICK_EXPORT QAccessibleQuickWindow : public QAccessibleObject
{
public:
    QAccessibleQuickWindow(QQuickWindow *object);

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;
    QAccessibleInterface *focusChild() const override;

    QAccessible::Role role() const override;
    QAccessible::State state() const override;
    QRect rect() const override;

    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QString text(QAccessible::Text text) const override;
    QAccessibleInterface *childAt(int x, int y) const override;

private:
    QQuickWindow *window() const override { return static_cast<QQuickWindow*>(object()); }
    QList<QQuickItem *> rootItems() const;
};

#endif // accessibility

QT_END_NAMESPACE

#endif // QAccessibleQuickView_H
