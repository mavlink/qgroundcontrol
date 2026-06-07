// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENUBAR_P_H
#define QQUICKMENUBAR_P_H

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

#include <QtQuickTemplates2/private/qquickcontainer_p.h>

QT_REQUIRE_CONFIG(quicktemplates2_container);

QT_BEGIN_NAMESPACE

class QQuickMenu;
class QQuickMenuBarPrivate;
class QQmlComponent;

class Q_QUICKTEMPLATES2_EXPORT QQuickMenuBar : public QQuickContainer
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL)
    Q_PRIVATE_PROPERTY(QQuickMenuBar::d_func(), QQmlListProperty<QQuickMenu> menus READ menus NOTIFY menusChanged FINAL)
    Q_PRIVATE_PROPERTY(QQuickMenuBar::d_func(), QQmlListProperty<QObject> contentData READ contentData FINAL)
    QML_NAMED_ELEMENT(MenuBar)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickMenuBar(QQuickItem *parent = nullptr);
    ~QQuickMenuBar() override;

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    Q_INVOKABLE QQuickMenu *menuAt(int index) const;
    Q_INVOKABLE void addMenu(QQuickMenu *menu);
    Q_INVOKABLE void insertMenu(int index, QQuickMenu *menu);
    Q_INVOKABLE void removeMenu(QQuickMenu *menu);
    Q_INVOKABLE QQuickMenu *takeMenu(int index);

Q_SIGNALS:
    void delegateChanged();
    void menusChanged();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

    bool isContent(QQuickItem *item) const override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value) override;
    void itemAdded(int index, QQuickItem *item) override;
    void itemMoved(int index, QQuickItem *item) override;
    void itemRemoved(int index, QQuickItem *item) override;

    void componentComplete() override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickMenuBar)
    Q_DECLARE_PRIVATE(QQuickMenuBar)
};

QT_END_NAMESPACE

#endif // QQUICKMENUBAR_P_H
