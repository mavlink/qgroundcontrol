// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMMENUITEM_P_H
#define QQUICKLABSPLATFORMMENUITEM_P_H

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
#include <QtCore/qurl.h>
#include <QtGui/qfont.h>
#include <QtGui/qpa/qplatformmenu.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqml.h>

#include "qquicklabsplatformicon_p.h"

QT_BEGIN_NAMESPACE

class QPlatformMenuItem;
class QQuickLabsPlatformMenu;
class QQuickLabsPlatformIconLoader;
class QQuickLabsPlatformMenuItemGroup;

class QQuickLabsPlatformMenuItem : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuItem)
    QML_EXTENDED_NAMESPACE(QPlatformMenuItem)
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQuickLabsPlatformMenu *menu READ menu NOTIFY menuChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenu *subMenu READ subMenu NOTIFY subMenuChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenuItemGroup *group READ group WRITE setGroup NOTIFY groupChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(bool separator READ isSeparator WRITE setSeparator NOTIFY separatorChanged FINAL)
    Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable NOTIFY checkableChanged FINAL)
    Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY checkedChanged FINAL)
    Q_PROPERTY(QPlatformMenuItem::MenuRole role READ role WRITE setRole NOTIFY roleChanged FINAL)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged FINAL)
    Q_PROPERTY(QVariant shortcut READ shortcut WRITE setShortcut NOTIFY shortcutChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformIcon icon READ icon WRITE setIcon NOTIFY iconChanged FINAL REVISION(1, 1))

public:
    explicit QQuickLabsPlatformMenuItem(QObject *parent = nullptr);
    ~QQuickLabsPlatformMenuItem();

    QPlatformMenuItem *handle() const;
    QPlatformMenuItem *create();
    void sync();

    QQuickLabsPlatformMenu *menu() const;
    void setMenu(QQuickLabsPlatformMenu* menu);

    QQuickLabsPlatformMenu *subMenu() const;
    void setSubMenu(QQuickLabsPlatformMenu *menu);

    QQuickLabsPlatformMenuItemGroup *group() const;
    void setGroup(QQuickLabsPlatformMenuItemGroup *group);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool isVisible() const;
    void setVisible(bool visible);

    bool isSeparator() const;
    void setSeparator(bool separator);

    bool isCheckable() const;
    void setCheckable(bool checkable);

    bool isChecked() const;
    void setChecked(bool checked);

    QPlatformMenuItem::MenuRole role() const;
    void setRole(QPlatformMenuItem::MenuRole role);

    QString text() const;
    void setText(const QString &text);

    QVariant shortcut() const;
    void setShortcut(const QVariant& shortcut);

    QFont font() const;
    void setFont(const QFont &font);

    QQuickLabsPlatformIcon icon() const;
    void setIcon(const QQuickLabsPlatformIcon &icon);

public Q_SLOTS:
    void toggle();

Q_SIGNALS:
    void triggered();
    void hovered();

    void menuChanged();
    void subMenuChanged();
    void groupChanged();
    void enabledChanged();
    void visibleChanged();
    void separatorChanged();
    void checkableChanged();
    void checkedChanged();
    void roleChanged();
    void textChanged();
    void shortcutChanged();
    void fontChanged();
    Q_REVISION(1, 1) void iconChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    QQuickLabsPlatformIconLoader *iconLoader() const;

    bool event(QEvent *e) override;
private Q_SLOTS:
    void activate();
    void updateIcon();

private:
    void addShortcut();
    void removeShortcut();

    bool m_complete;
    bool m_enabled;
    bool m_visible;
    bool m_separator;
    bool m_checkable;
    bool m_checked;
    QPlatformMenuItem::MenuRole m_role;
    QString m_text;
    QVariant m_shortcut;
    QFont m_font;
    QQuickLabsPlatformMenu *m_menu;
    QQuickLabsPlatformMenu *m_subMenu;
    QQuickLabsPlatformMenuItemGroup *m_group;
    mutable QQuickLabsPlatformIconLoader *m_iconLoader;
    QPlatformMenuItem *m_handle;
    int m_shortcutId = -1;

    friend class QQuickLabsPlatformMenu;
    friend class QQuickLabsPlatformMenuItemGroup;
};

QT_END_NAMESPACE

#endif // QQUICKLABSPLATFORMMENUITEM_P_H
