// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4STRINGTOARRAYINDEX_P_H
#define QV4STRINGTOARRAYINDEX_P_H

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

#include <QtCore/private/qnumeric_p.h>
#include <QtCore/qstring.h>
#include <limits>

QT_BEGIN_NAMESPACE

namespace QV4 {

inline uint charToUInt(const QChar *ch) { return ch->unicode(); }
inline uint charToUInt(const char *ch) { return static_cast<unsigned char>(*ch); }

template <typename T>
uint stringToArrayIndex(const T *ch, const T *end)
{
    if (ch == end)
        return std::numeric_limits<uint>::max();
    uint i = charToUInt(ch) - '0';
    if (i > 9)
        return std::numeric_limits<uint>::max();
    ++ch;
    // reject "01", "001", ...
    if (i == 0 && ch != end)
        return std::numeric_limits<uint>::max();

    while (ch < end) {
        uint x = charToUInt(ch) - '0';
        if (x > 9)
            return std::numeric_limits<uint>::max();
        if (qMulOverflow(i, uint(10), &i) || qAddOverflow(i, x, &i)) // i = i * 10 + x
            return std::numeric_limits<uint>::max();
        ++ch;
    }
    return i;
}

inline uint stringToArrayIndex(const QString &str)
{
    return stringToArrayIndex(str.constData(), str.constData() + str.size());
}

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4STRINGTOARRAYINDEX_P_H
