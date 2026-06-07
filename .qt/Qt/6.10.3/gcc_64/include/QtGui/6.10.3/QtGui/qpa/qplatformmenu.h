// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLATFORMMENU_H
#define QPLATFORMMENU_H
//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/qobject.h>
#include <QtGui/qtguiglobal.h>
#include <QtCore/qpointer.h>
#include <QtGui/qfont.h>
#if QT_CONFIG(shortcut)
#  include <QtGui/qkeysequence.h>
#endif
#include <QtGui/qicon.h>

QT_BEGIN_NAMESPACE

class QPlatformMenu;
class Q_GUI_EXPORT QPlatformMenuItem : public QObject
{
Q_OBJECT
public:
    QPlatformMenuItem();

    // copied from, and must stay in sync with, QAction menu roles.
    enum MenuRole { NoRole = 0, TextHeuristicRole, ApplicationSpecificRole, AboutQtRole,
                    AboutRole, PreferencesRole, QuitRole,
                    // However these roles are private, perhaps temporarily.
                    // They could be added as public QAction roles if necessary.
                    CutRole, CopyRole, PasteRole, SelectAllRole,
                    RoleCount };
    Q_ENUM(MenuRole)

    virtual void setTag(quintptr tag);
    virtual quintptr tag() const;

    virtual void setText(const QString &text) = 0;
    virtual void setIcon(const QIcon &icon) = 0;
    virtual void setMenu(QPlatformMenu *menu) = 0;
    virtual void setVisible(bool isVisible) = 0;
    virtual void setIsSeparator(bool isSeparator) = 0;
    virtual void setFont(const QFont &font) = 0;
    virtual void setRole(MenuRole role) = 0;
    virtual void setCheckable(bool checkable) = 0;
    virtual void setChecked(bool isChecked) = 0;
#if QT_CONFIG(shortcut)
    virtual void setShortcut(const QKeySequence& shortcut) = 0;
#endif
    virtual void setEnabled(bool enabled) = 0;
    virtual void setIconSize(int size) = 0;
    virtual void setNativeContents(WId item) { Q_UNUSED(item); }
    virtual void setHasExclusiveGroup(bool hasExclusiveGroup) { Q_UNUSED(hasExclusiveGroup); }

Q_SIGNALS:
    void activated();
    void hovered();

private:
    quintptr m_tag;
};

class Q_GUI_EXPORT QPlatformMenu : public QObject
{
Q_OBJECT
public:
    QPlatformMenu();

    enum MenuType { DefaultMenu = 0, EditMenu };
    Q_ENUM(MenuType)

    virtual void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before) = 0;
    virtual void removeMenuItem(QPlatformMenuItem *menuItem) = 0;
    virtual void syncMenuItem(QPlatformMenuItem *menuItem) = 0;
    virtual void syncSeparatorsCollapsible(bool enable) = 0;

    virtual void setTag(quintptr tag);
    virtual quintptr tag() const;

    virtual void setText(const QString &text) = 0;
    virtual void setIcon(const QIcon &icon) = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isEnabled() const { return true; }
    virtual void setVisible(bool visible) = 0;
    virtual void setMinimumWidth(int width) { Q_UNUSED(width); }
    virtual void setFont(const QFont &font) { Q_UNUSED(font); }
    virtual void setMenuType(MenuType type) { Q_UNUSED(type); }

    virtual void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
    {
        Q_UNUSED(parentWindow);
        Q_UNUSED(targetRect);
        Q_UNUSED(item);
        setVisible(true);
    }

    virtual void dismiss() { } // Closes this and all its related menu popups

    virtual QPlatformMenuItem *menuItemAt(int position) const = 0;
    virtual QPlatformMenuItem *menuItemForTag(quintptr tag) const = 0;

    virtual QPlatformMenuItem *createMenuItem() const;
    virtual QPlatformMenu *createSubMenu() const;
Q_SIGNALS:
    void aboutToShow();
    void aboutToHide();

private:
    quintptr m_tag;
};

class Q_GUI_EXPORT QPlatformMenuBar : public QObject
{
Q_OBJECT
public:
    virtual void insertMenu(QPlatformMenu *menu, QPlatformMenu *before) = 0;
    virtual void removeMenu(QPlatformMenu *menu) = 0;
    virtual void syncMenu(QPlatformMenu *menuItem) = 0;
    virtual void handleReparent(QWindow *newParentWindow) = 0;
    virtual QWindow *parentWindow() const { return nullptr; }

    virtual QPlatformMenu *menuForTag(quintptr tag) const = 0;
    virtual QPlatformMenu *createMenu() const;
};

QT_END_NAMESPACE

#endif

