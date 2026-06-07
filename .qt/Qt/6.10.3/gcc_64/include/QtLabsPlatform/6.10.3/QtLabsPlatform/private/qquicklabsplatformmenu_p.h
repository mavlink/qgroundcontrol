// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMMENU_P_H
#define QQUICKLABSPLATFORMMENU_P_H

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

#include <QtCore/qloggingcategory.h>
#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtGui/qfont.h>
#include <QtGui/qpa/qplatformmenu.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqml.h>

#include "qquicklabsplatformicon_p.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qtLabsPlatformMenus)

class QIcon;
class QWindow;
class QQuickItem;
class QPlatformMenu;
class QQuickLabsPlatformMenuBar;
class QQuickLabsPlatformMenuItem;
class QQuickLabsPlatformIconLoader;
class QQuickLabsPlatformSystemTrayIcon;

class QQuickLabsPlatformMenu : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Menu)
    QML_EXTENDED_NAMESPACE(QPlatformMenu)
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickLabsPlatformMenuItem> items READ items NOTIFY itemsChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenuBar *menuBar READ menuBar NOTIFY menuBarChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformMenu *parentMenu READ parentMenu NOTIFY parentMenuChanged FINAL)
#if QT_CONFIG(systemtrayicon)
    Q_PROPERTY(QQuickLabsPlatformSystemTrayIcon *systemTrayIcon READ systemTrayIcon NOTIFY systemTrayIconChanged FINAL)
#endif
    Q_PROPERTY(QQuickLabsPlatformMenuItem *menuItem READ menuItem CONSTANT FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged FINAL)
    Q_PROPERTY(int minimumWidth READ minimumWidth WRITE setMinimumWidth NOTIFY minimumWidthChanged FINAL)
    Q_PROPERTY(QPlatformMenu::MenuType type READ type WRITE setType NOTIFY typeChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged FINAL)
    Q_PROPERTY(QQuickLabsPlatformIcon icon READ icon WRITE setIcon NOTIFY iconChanged FINAL REVISION(1, 1))
    Q_CLASSINFO("DefaultProperty", "data")

public:
    explicit QQuickLabsPlatformMenu(QObject *parent = nullptr);
    ~QQuickLabsPlatformMenu();

    QPlatformMenu *handle() const;
    QPlatformMenu *create();
    void destroy();
    void sync();

    QQmlListProperty<QObject> data();
    QQmlListProperty<QQuickLabsPlatformMenuItem> items();

    QQuickLabsPlatformMenuBar *menuBar() const;
    void setMenuBar(QQuickLabsPlatformMenuBar *menuBar);

    QQuickLabsPlatformMenu *parentMenu() const;
    void setParentMenu(QQuickLabsPlatformMenu *menu);

#if QT_CONFIG(systemtrayicon)
    QQuickLabsPlatformSystemTrayIcon *systemTrayIcon() const;
    void setSystemTrayIcon(QQuickLabsPlatformSystemTrayIcon *icon);
#endif

    QQuickLabsPlatformMenuItem *menuItem() const;

    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool isVisible() const;
    void setVisible(bool visible);

    int minimumWidth() const;
    void setMinimumWidth(int width);

    QPlatformMenu::MenuType type() const;
    void setType(QPlatformMenu::MenuType type);

    QString title() const;
    void setTitle(const QString &title);

    QFont font() const;
    void setFont(const QFont &font);

    QQuickLabsPlatformIcon icon() const;
    void setIcon(const QQuickLabsPlatformIcon &icon);

    Q_INVOKABLE void addItem(QQuickLabsPlatformMenuItem *item);
    Q_INVOKABLE void insertItem(int index, QQuickLabsPlatformMenuItem *item);
    Q_INVOKABLE void removeItem(QQuickLabsPlatformMenuItem *item);

    Q_INVOKABLE void addMenu(QQuickLabsPlatformMenu *menu);
    Q_INVOKABLE void insertMenu(int index, QQuickLabsPlatformMenu *menu);
    Q_INVOKABLE void removeMenu(QQuickLabsPlatformMenu *menu);

    Q_INVOKABLE void clear();

public Q_SLOTS:
    void open(QQmlV4FunctionPtr args);
    void close();

Q_SIGNALS:
    void aboutToShow();
    void aboutToHide();

    void itemsChanged();
    void menuBarChanged();
    void parentMenuChanged();
    void systemTrayIconChanged();
    void titleChanged();
    void enabledChanged();
    void visibleChanged();
    void minimumWidthChanged();
    void fontChanged();
    void typeChanged();
    Q_REVISION(1, 1) void iconChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    QQuickLabsPlatformIconLoader *iconLoader() const;

    QWindow *findWindow(QQuickItem *target, QPoint *offset) const;

    static void data_append(QQmlListProperty<QObject> *property, QObject *object);
    static qsizetype data_count(QQmlListProperty<QObject> *property);
    static QObject *data_at(QQmlListProperty<QObject> *property, qsizetype index);
    static void data_clear(QQmlListProperty<QObject> *property);

    static void items_append(QQmlListProperty<QQuickLabsPlatformMenuItem> *property, QQuickLabsPlatformMenuItem *item);
    static qsizetype items_count(QQmlListProperty<QQuickLabsPlatformMenuItem> *property);
    static QQuickLabsPlatformMenuItem *items_at(QQmlListProperty<QQuickLabsPlatformMenuItem> *property, qsizetype index);
    static void items_clear(QQmlListProperty<QQuickLabsPlatformMenuItem> *property);

private Q_SLOTS:
    void updateIcon();

private:
    void unparentSubmenus();

    bool m_complete;
    bool m_enabled;
    bool m_visible;
    int m_minimumWidth;
    QPlatformMenu::MenuType m_type;
    QString m_title;
    QFont m_font;
    QList<QObject *> m_data;
    QList<QQuickLabsPlatformMenuItem *> m_items;
    QQuickLabsPlatformMenuBar *m_menuBar;
    QQuickLabsPlatformMenu *m_parentMenu;
    QQuickLabsPlatformSystemTrayIcon *m_systemTrayIcon;
    mutable QQuickLabsPlatformMenuItem *m_menuItem;
    mutable QQuickLabsPlatformIconLoader *m_iconLoader;
    QPlatformMenu *m_handle;
};

QT_END_NAMESPACE

#endif // QQUICKLABSPLATFORMMENU_P_H
