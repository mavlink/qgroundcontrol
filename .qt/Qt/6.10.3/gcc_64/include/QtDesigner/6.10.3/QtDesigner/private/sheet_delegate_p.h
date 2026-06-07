// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//


#ifndef SHEET_DELEGATE_H
#define SHEET_DELEGATE_H

#include "shared_global_p.h"

#include <QtWidgets/qstyleditemdelegate.h>
#include <QtWidgets/qtreeview.h>

QT_BEGIN_NAMESPACE

class QTreeView;

namespace qdesigner_internal {

class QDESIGNER_SHARED_EXPORT SheetDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    SheetDelegate(QTreeView *view, QWidget *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const override;

private:
    QTreeView *m_view;
};

} // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // SHEET_DELEGATE_H
