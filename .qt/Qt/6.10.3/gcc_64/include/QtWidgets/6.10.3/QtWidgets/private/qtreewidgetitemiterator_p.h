// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QTREEWIDGETITEMITERATOR_P_H
#define QTREEWIDGETITEMITERATOR_P_H

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

#include <QtCore/qstack.h>

#include "qtreewidgetitemiterator.h"
#include "private/qglobal_p.h"
#if QT_CONFIG(treewidget)

QT_BEGIN_NAMESPACE

class QTreeModel;
class QTreeWidgetItem;

class QTreeWidgetItemIteratorPrivate {
    Q_DECLARE_PUBLIC(QTreeWidgetItemIterator)
public:
    QTreeWidgetItemIteratorPrivate(QTreeWidgetItemIterator *q, QTreeModel *model)
        : m_currentIndex(0), m_model(model), q_ptr(q)
    {

    }

    QTreeWidgetItemIteratorPrivate(const QTreeWidgetItemIteratorPrivate& other)
        : m_currentIndex(other.m_currentIndex), m_model(other.m_model),
          m_parentIndex(other.m_parentIndex), q_ptr(other.q_ptr)
    {

    }

    QTreeWidgetItemIteratorPrivate &operator=(const QTreeWidgetItemIteratorPrivate& other)
    {
        m_currentIndex = other.m_currentIndex;
        m_parentIndex = other.m_parentIndex;
        m_model = other.m_model;
        return (*this);
    }

    ~QTreeWidgetItemIteratorPrivate()
    {
    }

    QTreeWidgetItem* nextSibling(const QTreeWidgetItem* item) const;
    void ensureValidIterator(const QTreeWidgetItem *itemToBeRemoved);

    QTreeWidgetItem *next(const QTreeWidgetItem *current);
    QTreeWidgetItem *previous(const QTreeWidgetItem *current);
private:
    int             m_currentIndex;
    QTreeModel     *m_model;        // This iterator class should not have ownership of the model.
    QStack<int>     m_parentIndex;
    QTreeWidgetItemIterator *q_ptr;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(treewidget)

#endif //QTREEWIDGETITEMITERATOR_P_H
