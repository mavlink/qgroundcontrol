// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLSOURCECOORDINATE_P_H
#define QQMLSOURCECOORDINATE_P_H

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

#include <QtCore/private/qglobal_p.h>

#include <limits>

QT_BEGIN_NAMESPACE

// These methods are needed because in some public methods we historically interpret -1 as the
// invalid line or column, even though all the lines and columns are 1-based. Also, the different
// integer ranges may turn certain large values into invalid ones on conversion.

template<typename From, typename To>
To qmlConvertSourceCoordinate(From n);

template<>
inline quint16 qmlConvertSourceCoordinate<int, quint16>(int n)
{
    return (n > 0 && n <= int(std::numeric_limits<quint16>::max())) ? quint16(n) : 0;
}

template<>
inline quint32 qmlConvertSourceCoordinate<int, quint32>(int n)
{
    return n > 0 ? quint32(n) : 0u;
}

// TODO: In Qt6, change behavior and make the invalid coordinate 0 for the following two methods.

template<>
inline int qmlConvertSourceCoordinate<quint16, int>(quint16 n)
{
    return (n == 0u) ? -1 : int(n);
}

template<>
inline int qmlConvertSourceCoordinate<quint32, int>(quint32 n)
{
    return (n == 0u || n > quint32(std::numeric_limits<int>::max())) ? -1 : int(n);
}

QT_END_NAMESPACE

#endif // QQMLSOURCECOORDINATE_P_H
