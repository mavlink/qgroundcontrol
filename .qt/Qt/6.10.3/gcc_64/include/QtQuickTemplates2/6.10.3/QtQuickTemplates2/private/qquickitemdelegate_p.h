// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKITEMDELEGATE_P_H
#define QQUICKITEMDELEGATE_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_BEGIN_NAMESPACE

class QQuickItemDelegatePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickItemDelegate : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
    QML_NAMED_ELEMENT(ItemDelegate)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickItemDelegate(QQuickItem *parent = nullptr);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

Q_SIGNALS:
    void highlightedChanged();

protected:
    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

protected:
    QQuickItemDelegate(QQuickItemDelegatePrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickItemDelegate)
    Q_DECLARE_PRIVATE(QQuickItemDelegate)
};

QT_END_NAMESPACE

#endif // QQUICKITEMDELEGATE_P_H
