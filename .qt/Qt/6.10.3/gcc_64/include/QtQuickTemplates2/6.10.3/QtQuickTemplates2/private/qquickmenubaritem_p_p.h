// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENUBARITEM_P_P_H
#define QQUICKMENUBARITEM_P_P_H

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

#include <QtQuickTemplates2/private/qquickmenubaritem_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickMenu;
class QQuickMenuBar;

class QQuickMenuBarItemPrivate : public QQuickAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickMenuBarItem)

public:
    static QQuickMenuBarItemPrivate *get(QQuickMenuBarItem *item)
    {
        return item->d_func();
    }

    void setMenuBar(QQuickMenuBar *menuBar);

    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;

    QPalette defaultPalette() const override;

    void accessiblePressAction() override;

    bool highlighted = false;
    QQuickMenu *menu = nullptr;
    QQuickMenuBar *menuBar = nullptr;
};

QT_END_NAMESPACE

#endif // QQUICKMENUBARITEM_P_P_H
