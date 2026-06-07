// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENU_P_P_H
#define QQUICKMENU_P_P_H

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

#include <QtCore/qlist.h>
#include <QtCore/qpointer.h>

#include <QtGui/qpa/qplatformmenu.h>

#include <QtQuickTemplates2/private/qquickmenu_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickAction;
class QQmlComponent;
class QQmlObjectModel;
class QQuickMenuItem;
class QQuickNativeMenuItem;
class QQuickMenuBar;

class Q_QUICKTEMPLATES2_EXPORT QQuickMenuPrivate : public QQuickPopupPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickMenu)

    QQuickMenuPrivate();

    static QQuickMenuPrivate *get(QQuickMenu *menu)
    {
        return menu->d_func();
    }

    void init();

    QPlatformMenu *nativeHandle();
    QPlatformMenu *maybeNativeHandle() const;
    QQuickMenu *rootMenu() const;
    bool useNativeMenu() const;
    bool createNativeMenu();
    void removeNativeMenu();
    void syncWithNativeMenu();
    void syncWithUseNativeMenu();
    void setNativeMenuVisible(bool visible);

    QQuickItem *itemAt(int index) const;
    void insertItem(int index, QQuickItem *item);
    void maybeCreateAndInsertNativeItem(int index, QQuickItem *item);
    void moveItem(int from, int to);
    enum class DestructionPolicy {
        Destroy,
        DoNotDestroy
    };
    void removeItem(int index, QQuickItem *item,
        DestructionPolicy destructionPolicy = DestructionPolicy::DoNotDestroy);
    enum class SyncPolicy {
        Sync,
        DoNotSync
    };
    void removeNativeItem(int index, SyncPolicy syncPolicy = SyncPolicy::Sync);
    void resetNativeData();

    static void recursivelyCreateNativeMenuItems(QQuickMenu *menu);

    void printContentModelItems() const;

    QQuickItem *beginCreateItem();
    void completeCreateItem();

    QQuickItem *createItem(QQuickMenu *menu);
    QQuickItem *createItem(QQuickAction *action);

    void resizeItem(QQuickItem *item);
    void resizeItems();

    void itemChildAdded(QQuickItem *item, QQuickItem *child) override;
    void itemSiblingOrderChanged(QQuickItem *item) override;
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) override;
    void itemDestroyed(QQuickItem *item) override;
    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange change, const QRectF &diff) override;

    QQuickPopupPositioner *getPositioner() override;
    bool prepareEnterTransition() override;
    bool prepareExitTransition() override;
    bool blockInput(QQuickItem *item, const QPointF &point) const override;
    bool handlePress(QQuickItem *item, const QPointF &point, ulong timestamp) override;
    bool handleReleaseWithoutGrab(const QEventPoint &eventPoint) override;

    void onItemHovered();
    void onItemTriggered();
    void onItemActiveFocusChanged();
    void updateTextPadding();

    QQuickMenu *currentSubMenu() const;
    void setParentMenu(QQuickMenu *parent);
    void resolveParentItem();

    void popup(QQuickItem *menuItem = nullptr);

    void propagateKeyEvent(QKeyEvent *event);

    void startHoverTimer();
    void stopHoverTimer();

    void setCurrentIndex(int index, Qt::FocusReason reason);
    bool activateNextItem();
    bool activatePreviousItem();

    QQuickMenuItem *firstEnabledMenuItem() const;

    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);
    static qsizetype contentData_count(QQmlListProperty<QObject> *prop);
    static QObject *contentData_at(QQmlListProperty<QObject> *prop, qsizetype index);
    static void contentData_clear(QQmlListProperty<QObject> *prop);

    QPalette defaultPalette() const override;
    virtual QQuickPopup::PopupType resolvedPopupType() const override;

    void resetContentItem();

    bool cascade = false;
    bool triedToCreateNativeMenu = false;
    int hoverTimer = 0;
    int currentIndex = -1;
    qreal overlap = 0;
    qreal textPadding = 0;
    QPointer<QQuickMenu> parentMenu;
    QPointer<QQuickMenuItem> currentItem;
    QPointer<QQuickItem> contentItem;
    QList<QObject *> contentData;
    QPointer<QQmlObjectModel> contentModel;
    QQmlComponent *delegate = nullptr;
    QString title;
    QQuickIcon icon;

    // For native menu support.
    std::unique_ptr<QPlatformMenu> handle = nullptr;
    QList<QQuickNativeMenuItem *> nativeItems;
    QPointer<QQuickMenuBar> menuBar;
    qreal lastDevicePixelRatio = 0;
};

QT_END_NAMESPACE

#endif // QQUICKMENU_P_P_H
