// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKNATIVEMENUITEM_P_H
#define QQUICKNATIVEMENUITEM_P_H

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

#include <QtCore/qobject.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtQuickTemplates2/private/qquickicon_p.h>

QT_BEGIN_NAMESPACE

class QQuickAction;
class QQuickNativeIconLoader;
class QQuickMenu;
class QQuickMenuSeparator;
class QPlatformMenuItem;

class Q_QUICKTEMPLATES2_EXPORT QQuickNativeMenuItem : public QObject
{
    Q_OBJECT

public:
    static QQuickNativeMenuItem *createFromNonNativeItem(
        QQuickMenu *parentMenu, QQuickItem *nonNativeItem);
    ~QQuickNativeMenuItem();

    QQuickAction *action() const;
    QQuickMenu *subMenu() const;
    QQuickMenuSeparator *separator() const;
    QPlatformMenuItem *handle() const;
    void sync();

    QQuickIcon effectiveIcon() const;
    QQuickNativeIconLoader *iconLoader() const;
    void reloadIcon();

    QString debugText() const;

private Q_SLOTS:
    void updateIcon();

private:
    enum class Type {
        Unknown,
        // It's an Action or a MenuItem with an Action.
        Action,
        // It's a MenuItem without an Action.
        MenuItem,
        Separator,
        SubMenu
    };

    explicit QQuickNativeMenuItem(QQuickMenu *parentMenu, QQuickItem *nonNativeItem, Type type);

    void addShortcut();
    void removeShortcut();

    QQuickMenu *m_parentMenu = nullptr;
    QQuickItem *m_nonNativeItem = nullptr;
    Type m_type = Type::Unknown;
    mutable QQuickNativeIconLoader *m_iconLoader = nullptr;
    std::unique_ptr<QPlatformMenuItem> m_handle = nullptr;
    int m_shortcutId = -1;
    bool m_syncing = false;
};

QT_END_NAMESPACE

#endif // QQUICKNATIVEMENUITEM_P_H
