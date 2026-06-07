// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QPRINTERINFO_P_H
#define QPRINTERINFO_P_H

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

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>

#ifndef QT_NO_PRINTER

#include "qprintdevice_p.h"

QT_BEGIN_NAMESPACE

class QPrinterInfoPrivate
{
public:
    explicit QPrinterInfoPrivate(const QString& id = QString());

    QPrintDevice m_printDevice;
};

QT_END_NAMESPACE

#endif // QT_NO_PRINTER

#endif // QPRINTERINFO_P_H
