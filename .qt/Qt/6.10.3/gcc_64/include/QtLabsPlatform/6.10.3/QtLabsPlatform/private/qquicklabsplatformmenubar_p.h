// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKLABSPLATFORMMENUBAR_P_H
#define QQUICKLABSPLATFORMMENUBAR_P_H

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
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqml.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QWindow;
class QPlatformMenuBar;
class QQuickLabsPlatformMenu;

class QQuickLabsPlatformMenuBar : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MenuBar)
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data FINAL)
    Q_PROPERTY(QQmlListProperty<QQuickLabsPlatformMenu> menus READ menus NOTIFY menusChanged FINAL)
    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    explicit QQuickLabsPlatformMenuBar(QObject *parent = nullptr);
    ~QQuickLabsPlatformMenuBar();

    QPlatformMenuBar *handle() const;

    QQmlListProperty<QObject> data();
    QQmlListProperty<QQuickLabsPlatformMenu> menus();

    QWindow *window() const;
    void setWindow(QWindow *window);

    Q_INVOKABLE void addMenu(QQuickLabsPlatformMenu *menu);
    Q_INVOKABLE void insertMenu(int index, QQuickLabsPlatformMenu *menu);
    Q_INVOKABLE void removeMenu(QQuickLabsPlatformMenu *menu);
    Q_INVOKABLE void clear();

Q_SIGNALS:
    void menusChanged();
    void windowChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

    QWindow *findWindow() const;

    static void data_append(QQmlListProperty<QObject> *property, QObject *object);
    static qsizetype data_count(QQmlListProperty<QObject> *property);
    static QObject *data_at(QQmlListProperty<QObject> *property, qsizetype index);
    static void data_clear(QQmlListProperty<QObject> *property);

    static void menus_append(QQmlListProperty<QQuickLabsPlatformMenu> *property, QQuickLabsPlatformMenu *menu);
    static qsizetype menus_count(QQmlListProperty<QQuickLabsPlatformMenu> *property);
    static QQuickLabsPlatformMenu *menus_at(QQmlListProperty<QQuickLabsPlatformMenu> *property, qsizetype index);
    static void menus_clear(QQmlListProperty<QQuickLabsPlatformMenu> *property);

private:
    bool m_complete;
    QWindow *m_window;
    QList<QObject *> m_data;
    QList<QQuickLabsPlatformMenu *> m_menus;
    QPlatformMenuBar *m_handle;
};

QT_END_NAMESPACE

#endif // QQUICKLABSPLATFORMMENUBAR_P_H
