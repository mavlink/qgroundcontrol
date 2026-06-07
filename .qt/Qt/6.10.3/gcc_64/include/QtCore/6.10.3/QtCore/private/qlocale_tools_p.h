// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QLOCALE_TOOLS_P_H
#define QLOCALE_TOOLS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include "qlocale_p.h"
#include "qstring.h"

#if !defined(QT_SUPPORTS_INT128) && (defined(Q_CC_MSVC) && (_MSC_VER >= 1930) && __has_include(<__msvc_int128.hpp>))
#include <__msvc_int128.hpp>
#define QT_USE_MSVC_INT128
#endif

QT_BEGIN_NAMESPACE

#if defined(QT_SUPPORTS_INT128)
using qinternalint128 = qint128;
using qinternaluint128 = quint128;
#elif defined(QT_USE_MSVC_INT128)
using qinternalint128 = std::_Signed128;
using qinternaluint128 = std::_Unsigned128;
#endif

enum StrayCharacterMode {
    TrailingJunkProhibited,
    TrailingJunkAllowed,
};

// API note: this function can't process a number with more than 2.1 billion digits
[[nodiscard]] QSimpleParsedNumber<double>
qt_asciiToDouble(const char *num, qsizetype numLen,
                 StrayCharacterMode strayCharMode = TrailingJunkProhibited);
void qt_doubleToAscii(double d, QLocaleData::DoubleForm form, int precision,
                      char *buf, qsizetype bufSize,
                      bool &sign, int &length, int &decpt);

[[nodiscard]] QString qulltoBasicLatin(qulonglong l, int base, bool negative);
[[nodiscard]] QString qulltoa(qulonglong l, int base, const QStringView zero);
[[nodiscard]] char *qulltoa2(char *p, qulonglong n, int base);
[[nodiscard]] Q_CORE_EXPORT QString qdtoa(qreal d, int *decpt, int *sign);
[[nodiscard]] QString qdtoBasicLatin(double d, QLocaleData::DoubleForm form,
                                     int precision, bool uppercase);
[[nodiscard]] QByteArray qdtoAscii(double d, QLocaleData::DoubleForm form,
                                   int precision, bool uppercase);

#if defined(QT_SUPPORTS_INT128) || defined(QT_USE_MSVC_INT128)
[[nodiscard]] Q_CORE_EXPORT QString quint128toBasicLatin(qinternaluint128 number,
                                                         int base = 10);
[[nodiscard]] Q_CORE_EXPORT QString qint128toBasicLatin(qinternalint128 number,
                                                        int base = 10);
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 12, 0)
[[deprecated("Use qIsNull(double) instead.")]]
[[nodiscard]] constexpr inline bool isZero(double d)
{
    return qIsNull(d);
}
#endif

// Enough space for the digits before the decimal separator:
[[nodiscard]] inline int wholePartSpace(double d)
{
    Q_ASSERT(d >= 0); // caller should call qAbs() if needed
    // Optimize for numbers between -512k and 512k - otherwise, use the
    // maximum number of digits in the whole number part of a double:
    return d > (1 << 19) ? std::numeric_limits<double>::max_exponent10 + 1 : 6;
}

// Returns code-point of same kind (UCS2 or UCS4) as zero; digit is 0 through 9
template <typename UcsInt>
[[nodiscard]] inline UcsInt unicodeForDigit(uint digit, UcsInt zero)
{
    // Must match qlocale.cpp's NumericTokenizer's digit-digestion.
    Q_ASSERT(digit < 10);
    if (!digit)
        return zero;

    // See QTBUG-85409: Suzhou's digits are U+3007, U+3021, ..., U+3029
    if (zero == u'\u3007')
        return u'\u3020' + digit;
    // In util/locale_database/ldml.py, LocaleScanner.numericData() asserts no
    // other number system in CLDR has discontinuous digits.

    return zero + digit;
}

[[nodiscard]] Q_CORE_EXPORT double qstrntod(const char *s00, qsizetype len,
                                            char const **se, bool *ok);
[[nodiscard]] inline double qstrtod(const char *s00, char const **se, bool *ok)
{
    qsizetype len = qsizetype(strlen(s00));
    return qstrntod(s00, len, se, ok);
}

[[nodiscard]] Q_AUTOTEST_EXPORT
QSimpleParsedNumber<qlonglong> qstrntoll(const char *nptr, qsizetype size, int base);
[[nodiscard]] QSimpleParsedNumber<qulonglong> qstrntoull(const char *nptr, qsizetype size, int base);

QT_END_NAMESPACE

#endif
