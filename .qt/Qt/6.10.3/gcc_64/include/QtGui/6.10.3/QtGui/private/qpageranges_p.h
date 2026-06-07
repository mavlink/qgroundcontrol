// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAGERANGES_P_H
#define QPAGERANGES_P_H

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

QT_BEGIN_NAMESPACE

class QPageRangesPrivate : public QSharedData
{
public:
    void mergeIntervals();

    QList<QPageRanges::Range> intervals;
};

QT_END_NAMESPACE

#endif // QPAGERANGES_P_H
