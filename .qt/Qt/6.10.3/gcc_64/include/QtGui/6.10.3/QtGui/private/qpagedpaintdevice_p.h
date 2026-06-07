// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPAGEDPAINTDEVICE_P_H
#define QPAGEDPAINTDEVICE_P_H

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
#include <qpagedpaintdevice.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QPagedPaintDevicePrivate
{
public:
    QPagedPaintDevicePrivate()
        : pageOrderAscending(true),
          printSelectionOnly(false)
    {
    }

    virtual ~QPagedPaintDevicePrivate();


    virtual bool setPageLayout(const QPageLayout &newPageLayout) = 0;

    virtual bool setPageSize(const QPageSize &pageSize) = 0;

    virtual bool setPageOrientation(QPageLayout::Orientation orientation) = 0;

    virtual bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units) = 0;

    virtual QPageLayout pageLayout() const = 0;

    static inline QPagedPaintDevicePrivate *get(QPagedPaintDevice *pd) { return pd->d; }

    // These are currently required to keep QPrinter functionality working in QTextDocument::print()
    QPageRanges pageRanges;
    bool pageOrderAscending;
    bool printSelectionOnly;
};

QT_END_NAMESPACE

#endif
