// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QACCESSIBLEQUICKITEM_H
#define QACCESSIBLEQUICKITEM_H

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

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include <QtGui/qaccessibleobject.h>
#include <QtQuick/private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

class QTextDocument;

class Q_QUICK_EXPORT QAccessibleQuickItem : public QAccessibleObject, public QAccessibleActionInterface, public QAccessibleValueInterface, public QAccessibleTextInterface
{
public:
    QAccessibleQuickItem(QQuickItem *item);

    QWindow *window() const override;

    QRect rect() const override;
    QRect viewRect() const;

    bool clipsChildren() const;
    QAccessibleInterface *childAt(int x, int y) const override;

    QAccessibleInterface *parent() const override;
    QAccessibleInterface *child(int index) const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *iface) const override;
    QList<QQuickItem *> childItems() const;

    QAccessible::State state() const override;
    QAccessible::Role role() const override;
    QString text(QAccessible::Text) const override;
    void setText(QAccessible::Text, const QString &text) override;

    bool isAccessible() const;

    QList<std::pair<QAccessibleInterface *, QAccessible::Relation>>
    relations(QAccessible::Relation match = QAccessible::AllRelations) const override;

    // Action Interface
    QStringList actionNames() const override;
    void doAction(const QString &actionName) override;
    QStringList keyBindingsForAction(const QString &actionName) const override;

    // Value Interface
    QVariant currentValue() const override;
    void setCurrentValue(const QVariant &value) override;
    QVariant maximumValue() const override;
    QVariant minimumValue() const override;
    QVariant minimumStepSize() const override;


    // Text Interface
    void selection(int selectionIndex, int *startOffset, int *endOffset) const override;
    int selectionCount() const override;
    void addSelection(int startOffset, int endOffset) override;
    void removeSelection(int selectionIndex) override;
    void setSelection(int selectionIndex, int startOffset, int endOffset) override;

    // cursor
    int cursorPosition() const override;
    void setCursorPosition(int position) override;

    // text
    QString text(int startOffset, int endOffset) const override;
    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                     int *startOffset, int *endOffset) const override;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                    int *startOffset, int *endOffset) const override;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                                 int *startOffset, int *endOffset) const override;
    int characterCount() const override;

    // character <-> geometry
    QRect characterRect(int /* offset */) const override { return QRect(); }
    int offsetAtPoint(const QPoint & /* point */) const override { return -1; }

    void scrollToSubstring(int /* startIndex */, int /* endIndex */) override {}
    QString attributes(int /* offset */, int *startOffset, int *endOffset) const override
    { *startOffset = 0; *endOffset = 0; return QString(); }

    QTextDocument *textDocument() const;

protected:
    QQuickItem *item() const { return static_cast<QQuickItem*>(object()); }
    void *interface_cast(QAccessible::InterfaceType t) override;

private:
    // for Text nodes:
    QTextDocument *m_doc;
    typedef QHash<int, QAccessible::Id> ChildCache;
    mutable ChildCache m_childToId;

};

QRect itemScreenRect(QQuickItem *item);
QList<QQuickItem *> accessibleUnignoredChildren(QQuickItem *item, bool paintOrder = false);

#endif // accessibility

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKITEM_H
