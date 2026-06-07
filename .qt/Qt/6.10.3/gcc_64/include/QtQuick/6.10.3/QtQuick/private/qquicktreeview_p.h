// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTREEVIEW_P_H
#define QQUICKTREEVIEW_P_H

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

#include <QtCore/qabstractitemmodel.h>
#include "qquicktableview_p.h"

QT_BEGIN_NAMESPACE

class QQuickTreeViewPrivate;

class Q_QUICK_EXPORT QQuickTreeView : public QQuickTableView
{
    Q_OBJECT
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex RESET resetRootIndex NOTIFY rootIndexChanged REVISION(6, 6) FINAL)
    QML_NAMED_ELEMENT(TreeView)
    QML_ADDED_IN_VERSION(6, 3)

public:
    QQuickTreeView(QQuickItem *parent = nullptr);
    ~QQuickTreeView() override;

    QModelIndex rootIndex() const;
    void setRootIndex(const QModelIndex &index);
    void resetRootIndex();

    Q_INVOKABLE int depth(int row) const;

    Q_INVOKABLE bool isExpanded(int row) const;
    Q_INVOKABLE void expand(int row);
    Q_INVOKABLE void collapse(int row);
    Q_INVOKABLE void toggleExpanded(int row);

    Q_REVISION(6, 4) Q_INVOKABLE void expandRecursively(int row = -1, int depth = -1);
    Q_REVISION(6, 4) Q_INVOKABLE void collapseRecursively(int row = -1);
    Q_REVISION(6, 4) Q_INVOKABLE void expandToIndex(const QModelIndex &index);

    Q_INVOKABLE QModelIndex modelIndex(const QPoint &cell) const override;
    Q_INVOKABLE QPoint cellAtIndex(const QModelIndex &index) const override;

#if QT_DEPRECATED_SINCE(6, 4)
    QT_DEPRECATED_VERSION_X_6_4("Use index(row, column) instead")
    Q_REVISION(6, 4) Q_INVOKABLE QModelIndex modelIndex(int row, int column) const override;
#endif

Q_SIGNALS:
    void expanded(int row, int depth);
    void collapsed(int row, bool recursively);
    Q_REVISION(6, 6) void rootIndexChanged();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickTreeView)
    Q_DECLARE_PRIVATE(QQuickTreeView)
};

QT_END_NAMESPACE

#endif // QQUICKTREEVIEW_P_H
