// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTEXTTABLE_P_H
#define QTEXTTABLE_P_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include "private/qtextobject_p.h"
#include "private/qtextdocument_p.h"

#include <vector>

QT_BEGIN_NAMESPACE

class QTextTablePrivate : public QTextFramePrivate
{
    Q_DECLARE_PUBLIC(QTextTable)
public:
    QTextTablePrivate(QTextDocument *document) : QTextFramePrivate(document), nRows(0), nCols(0), dirty(true), blockFragmentUpdates(false) {}

    static QTextTable *createTable(QTextDocumentPrivate *, int pos, int rows, int cols, const QTextTableFormat &tableFormat);
    void fragmentAdded(QChar type, uint fragment) override;
    void fragmentRemoved(QChar type, uint fragment) override;

    void update() const;

    int findCellIndex(int fragment) const;

    QList<int> cells;
    // symmetric to cells array and maps to indecs in grid,
    // used for fast-lookup for row/column by fragment
    mutable QList<int> cellIndices;
    mutable std::vector<int> grid;
    mutable int nRows;
    mutable int nCols;
    mutable bool dirty;
    bool blockFragmentUpdates;
};

QT_END_NAMESPACE

#endif // QTEXTTABLE_P_H
