// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef COMPLEXWIDGETS_H
#define COMPLEXWIDGETS_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtCore/qpointer.h>
#include <QtWidgets/qaccessiblewidget.h>
#if QT_CONFIG(itemviews)
#include <QtWidgets/qabstractitemview.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class QAbstractButton;
class QHeaderView;
class QTabBar;
class QComboBox;
class QTitleBar;
class QAbstractScrollArea;
class QScrollArea;

#if QT_CONFIG(scrollarea)
class QAccessibleAbstractScrollArea : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleAbstractScrollArea(QWidget *widget);

    enum AbstractScrollAreaElement {
        Self = 0,
        Viewport,
        HorizontalContainer,
        VerticalContainer,
        CornerWidget,
        Undefined
    };

    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *child) const override;
    bool isValid() const override;
    QAccessibleInterface *childAt(int x, int y) const override;
    QAbstractScrollArea *abstractScrollArea() const;

private:
    QWidgetList accessibleChildren() const;
    AbstractScrollAreaElement elementType(QWidget *widget) const;
    bool isLeftToRight() const;
};

class QAccessibleScrollArea : public QAccessibleAbstractScrollArea
{
public:
    explicit QAccessibleScrollArea(QWidget *widget);
};
#endif // QT_CONFIG(scrollarea)

#if QT_CONFIG(tabbar)
class QAccessibleTabBar : public QAccessibleWidgetV2, public QAccessibleSelectionInterface
{
public:
    explicit QAccessibleTabBar(QWidget *w);
    ~QAccessibleTabBar();

    void *interface_cast(QAccessible::InterfaceType t) override;

    QAccessibleInterface *focusChild() const override;
    int childCount() const override;
    QString text(QAccessible::Text t) const override;

    QAccessibleInterface* child(int index) const override;
    int indexOfChild(const QAccessibleInterface *child) const override;

    // QAccessibleSelectionInterface
    int selectedItemCount() const override;
    QList<QAccessibleInterface*> selectedItems() const override;
    QAccessibleInterface* selectedItem(int selectionIndex) const override;
    bool isSelected(QAccessibleInterface *childItem) const override;
    bool select(QAccessibleInterface *childItem) override;
    bool unselect(QAccessibleInterface *childItem) override;
    bool selectAll() override;
    bool clear() override;

protected:
    QTabBar *tabBar() const;
    mutable QHash<int, QAccessible::Id> m_childInterfaces;
};
#endif // QT_CONFIG(tabbar)

#if QT_CONFIG(combobox)
class QAccessibleComboBox : public QAccessibleWidgetV2
{
public:
    explicit QAccessibleComboBox(QWidget *w);

    int childCount() const override;
    QAccessibleInterface *childAt(int x, int y) const override;
    int indexOfChild(const QAccessibleInterface *child) const override;
    QAccessibleInterface* child(int index) const override;
    QAccessibleInterface* focusChild() const override;

    QString text(QAccessible::Text t) const override;

    QAccessible::State state() const override;

    // QAccessibleActionInterface
    QStringList actionNames() const override;
    QString localizedActionDescription(const QString &actionName) const override;
    void doAction(const QString &actionName) override;
    QStringList keyBindingsForAction(const QString &actionName) const override;

protected:
    QComboBox *comboBox() const;
};
#endif // QT_CONFIG(combobox)

#endif // QT_CONFIG(accessibility)

QT_END_NAMESPACE

#endif // COMPLEXWIDGETS_H
