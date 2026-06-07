// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENU_P_H
#define QQUICKMENU_P_H

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

#include <QtQml/qqmllist.h>
#include <QtQml/qqml.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>

#include "qquickpopup_p.h"
#include <QtQuickTemplates2/private/qquickicon_p.h>

QT_REQUIRE_CONFIG(qml_object_model);

QT_BEGIN_NAMESPACE

class QQuickAction;
class QQmlComponent;
class QQuickMenuItem;
class QQuickMenuPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickMenu : public QQuickPopup
{
    Q_OBJECT
    Q_PROPERTY(QVariant contentModel READ contentModel CONSTANT FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    // 2.3 (Qt 5.10)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL REVISION(2, 3))
    Q_PROPERTY(bool cascade READ cascade WRITE setCascade RESET resetCascade NOTIFY cascadeChanged FINAL REVISION(2, 3))
    Q_PROPERTY(qreal overlap READ overlap WRITE setOverlap NOTIFY overlapChanged FINAL REVISION(2, 3))
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged FINAL REVISION(2, 3))
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged FINAL REVISION(2, 3))
    // 6.5 (Qt 6.5)
    Q_PROPERTY(QQuickIcon icon READ icon WRITE setIcon NOTIFY iconChanged FINAL REVISION(6, 5))
    Q_CLASSINFO("DefaultProperty", "contentData")
    QML_NAMED_ELEMENT(Menu)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickMenu(QObject *parent = nullptr);
    ~QQuickMenu();

    Q_INVOKABLE QQuickItem *itemAt(int index) const;
    Q_INVOKABLE void addItem(QQuickItem *item);
    Q_INVOKABLE void insertItem(int index, QQuickItem *item);
    Q_INVOKABLE void moveItem(int from, int to);
    Q_INVOKABLE void removeItem(QQuickItem *item);

    QVariant contentModel() const;
    QQmlListProperty<QObject> contentData();

    QString title() const;
    void setTitle(const QString &title);

    QQuickIcon icon() const;
    void setIcon(const QQuickIcon &icon);

    bool cascade() const;
    void setCascade(bool cascade);
    void resetCascade();

    qreal overlap() const;
    void setOverlap(qreal overlap);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    int currentIndex() const;
    void setCurrentIndex(int index);

    // 2.3 (Qt 5.10)
    int count() const;
    Q_REVISION(2, 3) Q_INVOKABLE QQuickItem *takeItem(int index);

    Q_REVISION(2, 3) Q_INVOKABLE QQuickMenu *menuAt(int index) const;
    Q_REVISION(2, 3) Q_INVOKABLE void addMenu(QQuickMenu *menu);
    Q_REVISION(2, 3) Q_INVOKABLE void insertMenu(int index, QQuickMenu *menu);
    Q_REVISION(2, 3) Q_INVOKABLE void removeMenu(QQuickMenu *menu);
    Q_REVISION(2, 3) Q_INVOKABLE QQuickMenu *takeMenu(int index);

    Q_REVISION(2, 3) Q_INVOKABLE QQuickAction *actionAt(int index) const;
    Q_REVISION(2, 3) Q_INVOKABLE void addAction(QQuickAction *action);
    Q_REVISION(2, 3) Q_INVOKABLE void insertAction(int index, QQuickAction *action);
    Q_REVISION(2, 3) Q_INVOKABLE void removeAction(QQuickAction *action);
    Q_REVISION(2, 3) Q_INVOKABLE QQuickAction *takeAction(int index);

    bool isVisible() const override;
    void setVisible(bool visible) override;

    // overload set with parent item
    Q_REVISION(2, 3) Q_INVOKABLE void popup(QQuickItem *parent, qreal x, qreal y, QQuickItem *menuItem = nullptr);
    Q_REVISION(2, 3) Q_INVOKABLE void popup(QQuickItem *parent, const QPointF &position, QQuickItem *menuItem = nullptr);
    Q_REVISION(2, 3) Q_INVOKABLE void popup(QQuickItem *parent, QQuickItem *menuItem);
    Q_REVISION(2, 3) Q_INVOKABLE void popup(QQuickItem *parent = nullptr);
    // overload set without parent item
    Q_REVISION(2, 3) Q_INVOKABLE void popup(qreal x, qreal y, QQuickItem *menuItem = nullptr);
    Q_REVISION(2, 3) Q_INVOKABLE void popup(const QPointF &position, QQuickItem *menuItem = nullptr);
    Q_REVISION(2, 3) Q_INVOKABLE void dismiss();

protected:
    void componentComplete() override;
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data) override;
    void keyPressEvent(QKeyEvent *event) override;

Q_SIGNALS:
    void titleChanged(const QString &title);
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void countChanged();
    Q_REVISION(2, 3) void cascadeChanged(bool cascade);
    Q_REVISION(2, 3) void overlapChanged();
    Q_REVISION(2, 3) void delegateChanged();
    Q_REVISION(2, 3) void currentIndexChanged();
    // 6.5 (Qt 6.5)
    Q_REVISION(6, 5) void iconChanged(const QQuickIcon &icon);

protected:
    void timerEvent(QTimerEvent *event) override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickMenu)
    Q_DECLARE_PRIVATE(QQuickMenu)
};

QT_END_NAMESPACE

#endif // QQUICKMENU_P_H
