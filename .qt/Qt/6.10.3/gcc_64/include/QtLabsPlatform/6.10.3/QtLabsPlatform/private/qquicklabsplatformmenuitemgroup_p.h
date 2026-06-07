// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMMENUITEMGROUP_P_H
#define QQUICKLABSPLATFORMMENUITEMGROUP_P_H

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
#include <QtCore/qlist.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickLabsPlatformMenuItem;
class QQuickLabsPlatformMenuItemGroupPrivate;

class QQuickLabsPlatformMenuItemGroup : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuItemGroup)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive NOTIFY exclusiveChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenuItem *checkedItem READ checkedItem WRITE setCheckedItem NOTIFY checkedItemChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickLabsPlatformMenuItem> items READ items NOTIFY itemsChanged FINAL)

public:
    explicit QQuickLabsPlatformMenuItemGroup(QObject *parent = nullptr);
    ~QQuickLabsPlatformMenuItemGroup();

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool isVisible() const;
    void setVisible(bool visible);

    bool isExclusive() const;
    void setExclusive(bool exclusive);

    QQuickLabsPlatformMenuItem *checkedItem() const;
    void setCheckedItem(QQuickLabsPlatformMenuItem *item);

    QQmlListProperty<QQuickLabsPlatformMenuItem> items();

    Q_INVOKABLE void addItem(QQuickLabsPlatformMenuItem *item);
    Q_INVOKABLE void removeItem(QQuickLabsPlatformMenuItem *item);
    Q_INVOKABLE void clear();

Q_SIGNALS:
    void triggered(QQuickLabsPlatformMenuItem *item);
    void hovered(QQuickLabsPlatformMenuItem *item);

    void enabledChanged();
    void visibleChanged();
    void exclusiveChanged();
    void checkedItemChanged();
    void itemsChanged();

private:
    QQuickLabsPlatformMenuItem *findCurrent() const;
    void updateCurrent();
    void activateItem();
    void hoverItem();

    static void items_append(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop, QQuickLabsPlatformMenuItem *obj);
    static qsizetype items_count(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop);
    static QQuickLabsPlatformMenuItem *items_at(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop, qsizetype index);
    static void items_clear(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop);

    bool m_enabled;
    bool m_visible;
    bool m_exclusive;
    QQuickLabsPlatformMenuItem *m_checkedItem;
    QList<QQuickLabsPlatformMenuItem*> m_items;
};

QT_END_NAMESPACE

#endif // QQUICKLABSPLATFORMMENUITEMGROUP_P_H
