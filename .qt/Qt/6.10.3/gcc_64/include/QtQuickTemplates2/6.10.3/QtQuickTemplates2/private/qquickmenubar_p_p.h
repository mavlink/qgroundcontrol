// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENUBAR_P_P_H
#define QQUICKMENUBAR_P_P_H

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

#include <QtQuickTemplates2/private/qquickmenubar_p.h>
#include <QtQuickTemplates2/private/qquickcontainer_p_p.h>

#include <QtCore/qpointer.h>
#include <QtGui/qpa/qplatformmenu.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQuickMenuBarItem;

class Q_QUICKTEMPLATES2_EXPORT QQuickMenuBarPrivate : public QQuickContainerPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickMenuBar)

    static QQuickMenuBarPrivate *get(QQuickMenuBar *menuBar)
    {
        return menuBar->d_func();
    }

    QQmlListProperty<QQuickMenu> menus();
    QQmlListProperty<QObject> contentData();

    QQuickItem *createItemFromDelegate();
    QQuickMenuBarItem *createMenuBarItem(QQuickMenu *menu);

    void openCurrentMenu();
    void closeCurrentMenu();
    void activateMenuItem(int index);

    void activateItem(QQuickMenuBarItem *item);
    void activateNextItem();
    void activatePreviousItem();

    void onItemHovered();
    void onItemTriggered();
    void onMenuAboutToHide(QQuickMenu *menu);

    void insertMenu(int index, QQuickMenu *menu, QQuickMenuBarItem *delegateItem);
    QQuickMenu *takeMenu(int index);
    void insertNativeMenu(QQuickMenu *menu);
    void removeNativeMenu(QQuickMenu *menu);
    void syncMenuBarItemVisibilty(QQuickMenuBarItem *menuBarItem);

    QWindow *window() const;
    int menuIndex(QQuickMenu *menu) const;

    QPlatformMenuBar *nativeHandle() const;
    bool useNativeMenuBar() const;
    bool useNativeMenu(const QQuickMenu *menu) const;
    void syncNativeMenuBarVisible();
    void createNativeMenuBar();
    void removeNativeMenuBar();

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);

    static void menus_append(QQmlListProperty<QQuickMenu> *prop, QQuickMenu *obj);
    static qsizetype menus_count(QQmlListProperty<QQuickMenu> *prop);
    static QQuickMenu *menus_at(QQmlListProperty<QQuickMenu> *prop, qsizetype index);
    static void menus_clear(QQmlListProperty<QQuickMenu> *prop);

    QPalette defaultPalette() const override;

    bool closingCurrentMenu = false;
    bool altPressed = false;
    bool currentMenuOpen = false;
    QQmlComponent *delegate = nullptr;
    QPointer<QQuickMenuBarItem> currentItem;
    QPointer<QQuickItem> windowContentItem;

private:
    std::unique_ptr<QPlatformMenuBar> handle;
};

QT_END_NAMESPACE

#endif // QQUICKMENUBAR_P_P_H
