// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACTION_WIDGETS_P_H
#define QACTION_WIDGETS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qaction_p.h>
#if QT_CONFIG(menu)
#include <QtWidgets/qmenu.h>
#endif

#include <QtCore/qpointer.h>

QT_REQUIRE_CONFIG(action);

QT_BEGIN_NAMESPACE

class QShortcutMap;

class Q_WIDGETS_EXPORT QtWidgetsActionPrivate : public QActionPrivate
{
    Q_DECLARE_PUBLIC(QAction)
public:
    QtWidgetsActionPrivate() = default;
    ~QtWidgetsActionPrivate();

    void destroy() override;

#if QT_CONFIG(shortcut)
    QShortcutMap::ContextMatcher contextMatcher() const override;
#endif

#if QT_CONFIG(menu)
    QPointer<QMenu> m_menu;

    QObject *menu() const override;
    void setMenu(QObject *menu) override;
#endif
};

QT_END_NAMESPACE

#endif // QACTION_WIDGETS_P_H
