// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QTOOLS_P_H
#define QTOOLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/private/qglobal_p.h"

#include <chrono>
#include <limits.h>
#include <time.h>

QT_BEGIN_NAMESPACE

namespace QtMiscUtils {
[[nodiscard]] constexpr inline char toHexUpper(char32_t value) noexcept
{
    return "0123456789ABCDEF"[value & 0xF];
}

[[nodiscard]] constexpr inline char toHexLower(char32_t value) noexcept
{
    return "0123456789abcdef"[value & 0xF];
}

[[nodiscard]] constexpr inline bool isHexDigit(char32_t c) noexcept
{
    return (c >= '0' && c <= '9')
        || (c >= 'A' && c <= 'F')
        || (c >= 'a' && c <= 'f');
}

[[nodiscard]] constexpr inline int fromHex(char32_t c) noexcept
{
    return ((c >= '0') && (c <= '9')) ? int(c - '0') :
           ((c >= 'A') && (c <= 'F')) ? int(c - 'A' + 10) :
           ((c >= 'a') && (c <= 'f')) ? int(c - 'a' + 10) :
           /* otherwise */              -1;
}

[[nodiscard]] constexpr inline char toOct(char32_t value) noexcept
{
    return char('0' + (value & 0x7));
}

[[nodiscard]] constexpr inline bool isOctalDigit(char32_t c) noexcept
{
    return c >= '0' && c <= '7';
}

[[nodiscard]] constexpr inline int fromOct(char32_t c) noexcept
{
    return isOctalDigit(c) ? int(c - '0') : -1;
}

[[nodiscard]] constexpr inline bool isAsciiDigit(char32_t c) noexcept
{
    return c >= '0' && c <= '9';
}

[[nodiscard]] constexpr inline bool isAsciiUpper(char32_t c) noexcept
{
    return c >= 'A' && c <= 'Z';
}

[[nodiscard]] constexpr inline bool isAsciiLower(char32_t c) noexcept
{
    return c >= 'a' && c <= 'z';
}

[[nodiscard]] constexpr inline bool isAsciiLetterOrNumber(char32_t c) noexcept
{
    return  isAsciiDigit(c) || isAsciiLower(c) || isAsciiUpper(c);
}

[[nodiscard]] constexpr inline char toAsciiLower(char ch) noexcept
{
    return isAsciiUpper(ch) ? ch - 'A' + 'a' : ch;
}

[[nodiscard]] constexpr inline char toAsciiUpper(char ch) noexcept
{
    return isAsciiLower(ch) ? ch - 'a' + 'A' : ch;
}

[[nodiscard]] constexpr inline int caseCompareAscii(char lhs, char rhs) noexcept
{
    const char lhsLower = QtMiscUtils::toAsciiLower(lhs);
    const char rhsLower = QtMiscUtils::toAsciiLower(rhs);
    return int(uchar(lhsLower)) - int(uchar(rhsLower));
}

[[nodiscard]] constexpr inline int isAsciiPrintable(char32_t ch) noexcept
{
    return ch >= ' ' && ch < 0x7f;
}

[[nodiscard]] constexpr inline int qt_lencmp(qsizetype lhs, qsizetype rhs) noexcept
{
    return lhs == rhs ? 0 :
           lhs >  rhs ? 1 :
           /* else */  -1 ;
}

} // namespace QtMiscUtils

struct CalculateGrowingBlockSizeResult
{
    qsizetype size;
    qsizetype elementCount;
};

// Implemented in qarraydata.cpp:
qsizetype Q_CORE_EXPORT Q_DECL_CONST_FUNCTION
qCalculateBlockSize(qsizetype elementCount, qsizetype elementSize, qsizetype headerSize = 0) noexcept;
CalculateGrowingBlockSizeResult Q_CORE_EXPORT Q_DECL_CONST_FUNCTION
qCalculateGrowingBlockSize(qsizetype elementCount, qsizetype elementSize, qsizetype headerSize = 0) noexcept ;

QT_END_NAMESPACE

#endif // QTOOLS_P_H
