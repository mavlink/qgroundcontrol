// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENUITEM_P_P_H
#define QQUICKMENUITEM_P_P_H

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

#include <QtQuickTemplates2/private/qquickmenuitem_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickMenu;

class QQuickMenuItemPrivate : public QQuickAbstractButtonPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickMenuItem)

    static QQuickMenuItemPrivate *get(QQuickMenuItem *item)
    {
        return item->d_func();
    }

    void setMenu(QQuickMenu *menu);
    void setSubMenu(QQuickMenu *subMenu);

    void updateEnabled();

    void cancelArrow();
    void executeArrow(bool complete = false);

    bool acceptKeyClick(Qt::Key key) const override;

    QPalette defaultPalette() const override;

    bool highlighted = false;
    QQuickDeferredPointer<QQuickItem> arrow;
    QQuickMenu *menu = nullptr;
    QPointer<QQuickMenu> subMenu;
    qreal implicitTextPadding = 0;
};

QT_END_NAMESPACE

#endif // QQUICKMENUITEM_P_P_H
