// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKTREEVIEW_P_P_H
#define QQUICKTREEVIEW_P_P_H

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

#include "qquicktreeview_p.h"
#include "qquicktableview_p_p.h"

#include <QtQmlModels/private/qqmltreemodeltotablemodel_p_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickTreeViewPrivate : public QQuickTableViewPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickTreeView)

    QQuickTreeViewPrivate();
    ~QQuickTreeViewPrivate() override;

    static inline QQuickTreeViewPrivate *get(QQuickTreeView *q) { return q->d_func(); }

    QVariant modelImpl() const override;
    void setModelImpl(const QVariant &newModel) override;

    void initItemCallback(int serializedModelIndex, QObject *object) override;
    void itemReusedCallback(int serializedModelIndex, QObject *object) override;
    void dataChangedCallback(const QModelIndex &topLeft,
                             const QModelIndex &bottomRight,
                             const QVector<int> &roles);

    void updateRequiredProperties(int serializedModelIndex, QObject *object, bool init);
    void updateSelection(const QRect &oldSelection, const QRect &newSelection) override;

public:
    QQmlTreeModelToTableModel m_treeModelToTableModel;
    QVariant m_assignedModel;
};

QT_END_NAMESPACE

#endif // QQUICKTREEVIEW_P_P_H
