// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKMENUBARITEM_P_H
#define QQUICKMENUBARITEM_P_H

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

#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>

QT_REQUIRE_CONFIG(quicktemplates2_container);

QT_BEGIN_NAMESPACE

class QQuickMenu;
class QQuickMenuBar;
class QQuickMenuBarItemPrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickMenuBarItem : public QQuickAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(QQuickMenuBar *menuBar READ menuBar NOTIFY menuBarChanged FINAL)
    Q_PROPERTY(QQuickMenu *menu READ menu WRITE setMenu NOTIFY menuChanged FINAL)
    Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted NOTIFY highlightedChanged FINAL)
    QML_NAMED_ELEMENT(MenuBarItem)
    QML_ADDED_IN_VERSION(2, 3)

public:
    explicit QQuickMenuBarItem(QQuickItem *parent = nullptr);

    QQuickMenuBar *menuBar() const;

    QQuickMenu *menu() const;
    void setMenu(QQuickMenu *menu);

    bool isHighlighted() const;
    void setHighlighted(bool highlighted);

Q_SIGNALS:
    void triggered();
    void menuBarChanged();
    void menuChanged();
    void highlightedChanged();

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QFont defaultFont() const override;

#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override;
#endif

private:
    Q_DISABLE_COPY(QQuickMenuBarItem)
    Q_DECLARE_PRIVATE(QQuickMenuBarItem)
};

QT_END_NAMESPACE

#endif // QQUICKMENUBARITEM_P_H
