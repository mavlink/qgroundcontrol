// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEMROTATE_P_H
#define QMEMROTATE_P_H

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
#include <QtGui/private/qdrawhelper_p.h>

QT_BEGIN_NAMESPACE

#define QT_DECL_MEMROTATE(type)                            \
    void Q_GUI_EXPORT qt_memrotate90(const type*, int, int, int, type*, int); \
    void Q_GUI_EXPORT qt_memrotate180(const type*, int, int, int, type*, int); \
    void Q_GUI_EXPORT qt_memrotate270(const type*, int, int, int, type*, int)

QT_DECL_MEMROTATE(quint32);
QT_DECL_MEMROTATE(quint16);
QT_DECL_MEMROTATE(quint24);
QT_DECL_MEMROTATE(quint8);
QT_DECL_MEMROTATE(quint64);

#undef QT_DECL_MEMROTATE

QT_END_NAMESPACE

#endif // QMEMROTATE_P_H
