// Copyright (C) 2024 Ahmad Samir <a.samirh78@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef WCHARHELPERS_WIN_P_H
#define WCHARHELPERS_WIN_P_H

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


#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

static inline const wchar_t *qt_castToWchar(const QString &s)
{
    return reinterpret_cast<const wchar_t *>(s.constData());
}

QT_END_NAMESPACE

#endif // WCHARHELPERS_WIN_P_H
