// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEWIDGET_H
#define QACCESSIBLEWIDGET_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtGui/qaccessibleobject.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE


#if QT_CONFIG(accessibility)

class QAccessibleWidgetPrivate;

class Q_WIDGETS_EXPORT QAccessibleWidget : public QAccessibleObject,
                                           public QAccessibleActionInterface
{
public:
    explicit QAccessibleWidget(QWidget *o, QAccessible::Role r = QAccessible::Client);
    explicit QAccessibleWidget(QWidget *o, QAccessible::Role r, const QString& name);
    bool isValid() const override;

    QWindow *window() const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *child) const override;
    QList<std::pair<QAccessibleInterface *, QAccessible::Relation>>
    relations(QAccessible::Relation match = QAccessible::AllRelations) const override;
    QAccessibleInterface *focusChild() const override;

    QRect rect() const override;

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;

    QString text(QAccessible::Text t) const override;
    QAccessible::Role role() const override;
    QAccessible::State state() const override;

    QColor foregroundColor() const override;
    QColor backgroundColor() const override;

    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleActionInterface
    QStringList actionNames() const override;
    void doAction(const QString &actionName) override;
    QStringList keyBindingsForAction(const QString &actionName) const override;
protected:
    ~QAccessibleWidget();
    QWidget *widget() const;
    QObject *parentObject() const;

    void addControllingSignal(const QString &signal);

private:
    QAccessibleWidgetPrivate *d;
    Q_DISABLE_COPY(QAccessibleWidget)
};

class Q_WIDGETS_EXPORT QAccessibleWidgetV2 : public QAccessibleWidget,
                                             public QAccessibleAttributesInterface
{
#ifdef Q_OS_INTEGRITY
    // force instantiation to avoid error #2045
    struct error2045 : QList<QAccessible::Attribute> {};
#endif
public:
    explicit QAccessibleWidgetV2(QWidget *object, QAccessible::Role role = QAccessible::Client);
    explicit QAccessibleWidgetV2(QWidget *object, QAccessible::Role role, const QString &name);

protected:
    ~QAccessibleWidgetV2() override;

public:
    void *interface_cast(QAccessible::InterfaceType t) override;

    // QAccessibleAttributesInterface
    QList<QAccessible::Attribute> attributeKeys() const override;
    QVariant attributeValue(QAccessible::Attribute key) const override;

private:
    Q_DISABLE_COPY(QAccessibleWidgetV2)
};

#endif // QT_CONFIG(accessibility)

QT_END_NAMESPACE

#endif // QACCESSIBLEWIDGET_H
