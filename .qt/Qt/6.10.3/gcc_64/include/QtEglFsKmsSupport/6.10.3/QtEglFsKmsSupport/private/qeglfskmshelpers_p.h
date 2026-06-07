// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QEGLFSKMSHELPERS_H
#define QEGLFSKMSHELPERS_H

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

#include <QString>

QT_BEGIN_NAMESPACE

inline QString q_fourccToString(uint code)
{
    QString s;
    s.reserve(4);
    s.append(code & 0xff);
    s.append((code >> 8) & 0xff);
    s.append((code >> 16) & 0xff);
    s.append((code >> 24) & 0xff);
    return s;
}

QT_END_NAMESPACE

#endif // QEGLFSKMSHELPERS_H
