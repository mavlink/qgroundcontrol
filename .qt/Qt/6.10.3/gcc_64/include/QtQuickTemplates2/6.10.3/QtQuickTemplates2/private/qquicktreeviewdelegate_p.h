// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTREEVIEWDELEGATE_P_H
#define QQUICKTREEVIEWDELEGATE_P_H

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

#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquicktreeview_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>
#include <QtQuickTemplates2/private/qquickitemdelegate_p.h>

QT_BEGIN_NAMESPACE

class QQuickTreeViewDelegatePrivate;

class Q_QUICKTEMPLATES2_EXPORT QQuickTreeViewDelegate : public QQuickItemDelegate
{
    Q_OBJECT
    Q_PROPERTY(qreal indentation READ indentation WRITE setIndentation NOTIFY indentationChanged FINAL)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged FINAL)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin NOTIFY rightMarginChanged FINAL)

    // Required properties
    Q_PROPERTY(QQuickTreeView *treeView READ treeView WRITE setTreeView NOTIFY treeviewChanged REQUIRED FINAL)
    Q_PROPERTY(bool isTreeNode READ isTreeNode WRITE setIsTreeNode NOTIFY isTreeNodeChanged REQUIRED FINAL)
    Q_PROPERTY(bool hasChildren READ hasChildren WRITE setHasChildren NOTIFY hasChildrenChanged REQUIRED FINAL)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged REQUIRED FINAL)
    Q_PROPERTY(int depth READ depth WRITE setDepth NOTIFY depthChanged REQUIRED FINAL)
    Q_PROPERTY(bool current READ current WRITE setCurrent NOTIFY currentChanged REQUIRED FINAL REVISION(6, 4))
    Q_PROPERTY(bool selected READ selected WRITE setSelected NOTIFY selectedChanged REQUIRED FINAL REVISION(6, 4))
    Q_PROPERTY(bool editing READ editing WRITE setEditing NOTIFY editingChanged REQUIRED FINAL REVISION(6, 5))

    QML_NAMED_ELEMENT(TreeViewDelegate)
    QML_ADDED_IN_VERSION(6, 3)

public:
    explicit QQuickTreeViewDelegate(QQuickItem *parent = nullptr);

    qreal indentation() const;
    void setIndentation(qreal indentation);

    bool isTreeNode() const;
    void setIsTreeNode(bool isTreeNode);

    bool hasChildren() const;
    void setHasChildren(bool hasChildren);

    bool expanded() const;
    void setExpanded(bool expanded);

    bool current() const;
    void setCurrent(bool current);

    bool selected() const;
    void setSelected(bool selected);

    bool editing() const;
    void setEditing(bool editing);

    int depth() const;
    void setDepth(int depth);

    QQuickTreeView *treeView() const;
    void setTreeView(QQuickTreeView *treeView);

    qreal leftMargin() const;
    void setLeftMargin(qreal leftMargin);

    qreal rightMargin() const;
    void setRightMargin(qreal rightMargin);

Q_SIGNALS:
    void indicatorChanged();
    void indentationChanged();
    void isTreeNodeChanged();
    void hasChildrenChanged();
    void expandedChanged();
    void depthChanged();
    void treeviewChanged();
    void leftMarginChanged();
    void rightMarginChanged();
    Q_REVISION(6, 4) void currentChanged();
    Q_REVISION(6, 4) void selectedChanged();
    Q_REVISION(6, 5) void editingChanged();

protected:
    QFont defaultFont() const override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickTreeViewDelegate)
    Q_DECLARE_PRIVATE(QQuickTreeViewDelegate)
};

QT_END_NAMESPACE

#endif // QQUICKTREEVIEWDELEGATE_P_H
