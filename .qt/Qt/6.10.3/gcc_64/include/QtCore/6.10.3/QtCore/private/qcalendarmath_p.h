// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCALENDARMATH_P_H
#define QCALENDARMATH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of q*calendar.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/QtAlgorithms>

QT_BEGIN_NAMESPACE

namespace QRoundingDown {
// Note: qgregoriancalendar.cpp contains some static asserts to verify this all works.
namespace QRoundingDownPrivate {
#ifdef Q_CC_MSVC
// MSVC 2019 doesn't believe in the constexpr-ness of the #else clause's version :-(
#define QCALMATH_ISPOW2(b) ((b > 0) && !(b & (b - 1))) // See #else's comment.
#else
// Subtracting one toggles the least significant set bit and any unset bits less
// significant than it, leaving other bits unchanged. Thus the & of this with
// the original number preserves all more significant bits, clearing the least
// significant. If there are no such bits, either our number was 0 or it only
// had one bit set, hence is a power of two.
template <typename Int>
inline constexpr bool isPowerOfTwo(Int b) { return b > 0 && (b & (b - 1)) == 0; }
#define QCALMATH_ISPOW2(b) QRoundingDownPrivate::isPowerOfTwo(b)
#endif
}
/*
  Division, rounding down (rather than towards zero).

  From C++11 onwards, integer division is defined to round towards zero, so we
  can rely on that when implementing this. This is only used with denominator b
  > 0, so we only have to treat negative numerator, a, specially.

  If a is a multiple of b, adding 1 before and subtracting it after dividing by
  b gets us to where we should be (albeit by an eccentric path), since the
  adding caused rounding up, undone by the subtracting. Otherwise, adding 1
  doesn't change the result of dividing by b; and we want one less than that
  result. This is equivalent to subtracting b - 1 and simply dividing, except
  when that subtraction would underflow.

  For the remainder, with negative a, aside from having to add one and subtract
  it later to deal with the exact multiples, we can simply use the truncating
  remainder and then add b. When b is a power of two we can, of course, get the
  remainder correctly by the same masking that works for positive a.
*/

// Fall-back, to ensure intelligible error messages on mis-use:
template <unsigned b, typename Int, std::enable_if_t<(int(b) < 2), bool> = true>
constexpr auto qDivMod(Int)
{
    static_assert(b, "Division by 0 is undefined");
    // Use complement of earlier cases || new check, to ensure only one error:
    static_assert(!b || int(b) > 0, "Denominator is too big");
    static_assert(int(b) < 1 || b > 1, "Division by 1 is fautous");
    struct R { Int quotient; Int remainder; };
    return R { 0, 0 };
}

template <unsigned b, typename Int,
          std::enable_if_t<(b > 1) && !QCALMATH_ISPOW2(b) && (int(b) > 0),
                           bool> = true>
constexpr auto qDivMod(Int a)
{
    struct R { Int quotient; Int remainder; };
    if constexpr (std::is_signed_v<Int>) {
        if (a < 0) {
            ++a;    // can't overflow, it's negative
            return R { Int(a / int(b) - 1), Int(a % int(b) - 1 + int(b)) };
        }
    }
    return R { Int(a / int(b)), Int(a % int(b)) };
}

template <unsigned b, typename Int,
          std::enable_if_t<(b > 1) && QCALMATH_ISPOW2(b) && (int(b) > 0),
                           bool> = true>
constexpr auto qDivMod(Int a)
{
    constexpr unsigned w = QtPrivate::qConstexprCountTrailingZeroBits(b);
    struct R { Int quotient; Int remainder; };
    if constexpr (std::is_signed_v<Int>) {
        if (a < 0)
            return R { Int((a + 1) / int(b) - 1), Int(a & int(b - 1)) };
    }
    return R { Int(a >> w), Int(a & int(b - 1)) };
}

#undef QCALMATH_ISPOW2
// </kludge>

template <unsigned b, typename Int> constexpr Int qDiv(Int a) { return qDivMod<b>(a).quotient; }
template <unsigned b, typename Int> constexpr Int qMod(Int a) { return qDivMod<b>(a).remainder; }

} // QRoundingDown

namespace QRomanCalendrical {
// Julian Day number of Gregorian 1 BCE, February 29th:
inline constexpr qint64 LeapDayGregorian1Bce = 1721119;
// Aside from (maybe) some turns of centuries, one year in four is leap:
inline constexpr unsigned FourYears = 4 * 365 + 1;
inline constexpr unsigned FiveMonths = 31 + 30 + 31 + 30 + 31; // Mar-Jul or Aug-Dec.

constexpr auto yearMonthToYearDays(int year, int month)
{
    // Pre-digests year and month to (possibly denormal) year count and day-within-year.
    struct R { qint64 year; qint64 days; };
    if (year < 0) // Represent -N BCE as 1-N so year numbering is contiguous.
        ++year;
    month -= 3; // Adjust month numbering so March = 0, ...
    if (month < 0) { // and Jan = 10, Feb = 11, in the previous year.
        --year;
        month += 12;
    }
    return R { year, QRoundingDown::qDiv<5>(FiveMonths * month + 2) };
}

constexpr auto dayInYearToYmd(int dayInYear)
{
    // The year is an adjustment to the year for which dayInYear may be denormal.
    struct R { int year; int month; int day; };
    // Shared code for Julian and Milankovic (at least).
    using namespace QRoundingDown;
    const auto month5Day = qDivMod<FiveMonths>(5 * dayInYear + 2);
    // Its remainder changes by 5 per day, except at roughly monthly quotient steps.
    const auto yearMonth = qDivMod<12>(month5Day.quotient + 2);
    return R { yearMonth.quotient, yearMonth.remainder + 1, qDiv<5>(month5Day.remainder) + 1 };
}
}

QT_END_NAMESPACE

#endif // QCALENDARMATH_P_H
