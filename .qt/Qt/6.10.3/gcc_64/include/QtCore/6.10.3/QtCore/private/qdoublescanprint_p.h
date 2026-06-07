// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QDOUBLESCANPRINT_P_H
#define QDOUBLESCANPRINT_P_H

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

#include <private/qglobal_p.h>

#if defined(Q_CC_MSVC) && (defined(QT_BOOTSTRAPPED) || defined(QT_NO_DOUBLECONVERSION))
#  include <stdio.h>
#  include <locale.h>

QT_BEGIN_NAMESPACE

// We can always use _sscanf_l and _snprintf_l on MSVC as those were introduced in 2005.

// MSVC doesn't document what it will do with a NULL locale passed to _sscanf_l or _snprintf_l.
// The documentation for _create_locale() does not formally document "C" to be valid, but an example
// code snippet in the same documentation shows it.

struct QCLocaleT {
    QCLocaleT() : locale(_create_locale(LC_ALL, "C"))
    {
    }

    ~QCLocaleT()
    {
        _free_locale(locale);
    }

    const _locale_t locale;
};

#  define QT_CLOCALE_HOLDER Q_GLOBAL_STATIC(QCLocaleT, cLocaleT)
#  define QT_CLOCALE cLocaleT()->locale

inline int qDoubleSscanf(const char *buf, _locale_t locale, const char *format, double *d,
                         int *processed)
{
    return _sscanf_l(buf, format, locale, d, processed);
}

inline int qDoubleSnprintf(char *buf, size_t buflen, _locale_t locale, const char *format, double d)
{
    return _snprintf_l(buf, buflen, format, locale, d);
}

QT_END_NAMESPACE

#elif defined(QT_BOOTSTRAPPED)
#  include <stdio.h>

QT_BEGIN_NAMESPACE

// When bootstrapping we don't have libdouble-conversion available, yet. We can also not use locale
// aware snprintf and sscanf variants in the general case because those are only available on select
// platforms. We can use the regular snprintf and sscanf because we don't do setlocale(3) when
// bootstrapping and the locale is always "C" then.

#  define QT_CLOCALE_HOLDER
#  define QT_CLOCALE 0

inline int qDoubleSscanf(const char *buf, int, const char *format, double *d, int *processed)
{
    return sscanf(buf, format, d, processed);
}
inline int qDoubleSnprintf(char *buf, size_t buflen, int, const char *format, double d)
{
    return snprintf(buf, buflen, format, d);
}

QT_END_NAMESPACE

#else // !QT_BOOTSTRAPPED && (!Q_CC_MSVC || !QT_NO_DOUBLECONVERSION)
#  ifdef QT_NO_DOUBLECONVERSION
#    include <stdio.h>
#    include <xlocale.h>

QT_BEGIN_NAMESPACE

// OS X and FreeBSD both treat NULL as the "C" locale for snprintf_l and sscanf_l.
// When other implementations with different behavior show up, we'll have to do newlocale(3) and
// freelocale(3) here. The arguments to those will depend on what the other implementations will
// offer. OS X and FreeBSD again interpret a locale name of NULL as "C", but "C" itself is not
// documented as valid locale name. Mind that the names of the LC_* constants differ between e.g.
// BSD variants and linux.

#    define QT_CLOCALE_HOLDER
#    define QT_CLOCALE NULL

inline int qDoubleSscanf(const char *buf, locale_t locale, const char *format, double *d,
                         int *processed)
{
    return sscanf_l(buf, locale, format, d, processed);
}
inline int qDoubleSnprintf(char *buf, size_t buflen, locale_t locale, const char *format, double d)
{
    return snprintf_l(buf, buflen, locale, format, d);
}

QT_END_NAMESPACE

#  else // !QT_NO_DOUBLECONVERSION
#    include <double-conversion/double-conversion.h>
#    define QT_CLOCALE_HOLDER
#  endif // QT_NO_DOUBLECONVERSION
#endif // QT_BOOTSTRAPPED

#endif // QDOUBLESCANPRINT_P_H
