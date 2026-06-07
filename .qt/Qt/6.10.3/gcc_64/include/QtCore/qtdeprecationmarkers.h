// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTDEPRECATIONMARKERS_H
#define QTDEPRECATIONMARKERS_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qtdeprecationdefinitions.h>
#include <QtCore/qtversionchecks.h>
#include <QtCore/qcompilerdetection.h> // for Q_DECL_DEPRECATED

#if 0
#pragma qt_class(QtDeprecationMarkers)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

#if defined(QT_NO_DEPRECATED)
/* undef, so as to cause compile-errors when they're used outside #if QT_DEPRECATED_SINCE blocks */
#  undef QT_DEPRECATED
#  undef QT_DEPRECATED_X
#  undef QT_DEPRECATED_VARIABLE
#  undef QT_DEPRECATED_CONSTRUCTOR
#elif !defined(QT_NO_DEPRECATED_WARNINGS)
#  undef QT_DEPRECATED
#  define QT_DEPRECATED Q_DECL_DEPRECATED
#  undef QT_DEPRECATED_X
#  define QT_DEPRECATED_X(text) Q_DECL_DEPRECATED_X(text)
#  undef QT_DEPRECATED_VARIABLE
#  define QT_DEPRECATED_VARIABLE Q_DECL_VARIABLE_DEPRECATED
#  undef QT_DEPRECATED_CONSTRUCTOR
#  define QT_DEPRECATED_CONSTRUCTOR Q_DECL_CONSTRUCTOR_DEPRECATED explicit
#else
#  undef QT_DEPRECATED
#  define QT_DEPRECATED
#  undef QT_DEPRECATED_X
#  define QT_DEPRECATED_X(text)
#  undef QT_DEPRECATED_VARIABLE
#  define QT_DEPRECATED_VARIABLE
#  undef QT_DEPRECATED_CONSTRUCTOR
#  define QT_DEPRECATED_CONSTRUCTOR
#  undef Q_DECL_ENUMERATOR_DEPRECATED
#  define Q_DECL_ENUMERATOR_DEPRECATED
#  undef Q_DECL_ENUMERATOR_DEPRECATED_X
#  define Q_DECL_ENUMERATOR_DEPRECATED_X(ignored)
#endif

/*
    QT_DEPRECATED_SINCE(major, minor) evaluates as true if the Qt version is greater than
    the deprecation point specified.

    Use it to specify from which version of Qt a function or class has been deprecated

    Example:
        #if QT_DEPRECATED_SINCE(5,1)
            QT_DEPRECATED void deprecatedFunction(); //function deprecated since Qt 5.1
        #endif

*/
#ifdef QT_DEPRECATED
#define QT_DEPRECATED_SINCE(major, minor) (QT_VERSION_CHECK(major, minor, 0) > QT_DISABLE_DEPRECATED_UP_TO)
#else
#define QT_DEPRECATED_SINCE(major, minor) 0
#endif

/*
    QT_REMOVAL_QT{VER}_DEPRECATED_SINCE(major, minor)

    The macro should be used if the API is deprecated and should be removed
    in the {VER} major release.

    The \a major and \a minor parameters specify the deprecation version.

    For now, we provide the macros to remove the deprecated APIs in Qt 7
    and in Qt 8.

    Example:

    \code
    #if QT_REMOVAL_QT7_DEPRECATED_SINCE(6, 9)
        QT_DEPRECATED_VERSION_X_6_9("The reason for the deprecation")
        void deprecatedFunc();
    #endif
    \endcode

    The \c {deprecatedFunc()} function is deprecated since Qt 6.9, and will be
    completely removed in Qt 7.0.
*/
#define QT_DEPRECATED_TO_BE_REMOVED_HELPER(dep_major, dep_minor, rem_major) \
    (QT_DEPRECATED_SINCE(dep_major, dep_minor) && (QT_VERSION < QT_VERSION_CHECK(rem_major, 0, 0)))

// For APIs that should be removed in Qt 7
#define QT_REMOVAL_QT7_DEPRECATED_SINCE(major, minor) \
    QT_DEPRECATED_TO_BE_REMOVED_HELPER(major, minor, 7)

// For APIs that should be removed in Qt 8
#define QT_REMOVAL_QT8_DEPRECATED_SINCE(major, minor) \
    QT_DEPRECATED_TO_BE_REMOVED_HELPER(major, minor, 8)

/*
  QT_DEPRECATED_VERSION(major, minor) and QT_DEPRECATED_VERSION_X(major, minor, text)
  outputs a deprecation warning if QT_WARN_DEPRECATED_UP_TO is equal to or greater
  than the version specified as major, minor. This makes it possible to deprecate a
  function without annoying a user who needs to stay compatible with a specified minimum
  version and therefore can't use the new function.
*/
#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(5, 12, 0)
# define QT_DEPRECATED_VERSION_X_5_12(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_5_12         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_5_12(text)
# define QT_DEPRECATED_VERSION_5_12
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(5, 13, 0)
# define QT_DEPRECATED_VERSION_X_5_13(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_5_13         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_5_13(text)
# define QT_DEPRECATED_VERSION_5_13
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(5, 14, 0)
# define QT_DEPRECATED_VERSION_X_5_14(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_5_14         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_5_14(text)
# define QT_DEPRECATED_VERSION_5_14
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(5, 15, 0)
# define QT_DEPRECATED_VERSION_X_5_15(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_5_15         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_5_15(text)
# define QT_DEPRECATED_VERSION_5_15
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 0, 0)
# define QT_DEPRECATED_VERSION_X_6_0(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_0         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_0(text)
# define QT_DEPRECATED_VERSION_6_0
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 1, 0)
# define QT_DEPRECATED_VERSION_X_6_1(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_1         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_1(text)
# define QT_DEPRECATED_VERSION_6_1
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 2, 0)
# define QT_DEPRECATED_VERSION_X_6_2(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_2         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_2(text)
# define QT_DEPRECATED_VERSION_6_2
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 3, 0)
# define QT_DEPRECATED_VERSION_X_6_3(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_3         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_3(text)
# define QT_DEPRECATED_VERSION_6_3
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 4, 0)
# define QT_DEPRECATED_VERSION_X_6_4(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_4         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_4(text)
# define QT_DEPRECATED_VERSION_6_4
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 5, 0)
# define QT_DEPRECATED_VERSION_X_6_5(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_5         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_5(text)
# define QT_DEPRECATED_VERSION_6_5
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 6, 0)
# define QT_DEPRECATED_VERSION_X_6_6(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_6         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_6(text)
# define QT_DEPRECATED_VERSION_6_6
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 7, 0)
# define QT_DEPRECATED_VERSION_X_6_7(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_7         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_7(text)
# define QT_DEPRECATED_VERSION_6_7
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 8, 0)
# define QT_DEPRECATED_VERSION_X_6_8(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_8         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_8(text)
# define QT_DEPRECATED_VERSION_6_8
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 9, 0)
# define QT_DEPRECATED_VERSION_X_6_9(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_9         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_9(text)
# define QT_DEPRECATED_VERSION_6_9
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 10, 0)
# define QT_DEPRECATED_VERSION_X_6_10(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_10         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_10(text)
# define QT_DEPRECATED_VERSION_6_10
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 11, 0)
# define QT_DEPRECATED_VERSION_X_6_11(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_11         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_11(text)
# define QT_DEPRECATED_VERSION_6_11
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 12, 0)
# define QT_DEPRECATED_VERSION_X_6_12(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_12         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_12(text)
# define QT_DEPRECATED_VERSION_6_12
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 13, 0)
# define QT_DEPRECATED_VERSION_X_6_13(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_13         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_13(text)
# define QT_DEPRECATED_VERSION_6_13
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 14, 0)
# define QT_DEPRECATED_VERSION_X_6_14(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_14         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_14(text)
# define QT_DEPRECATED_VERSION_6_14
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 15, 0)
# define QT_DEPRECATED_VERSION_X_6_15(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_15         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_15(text)
# define QT_DEPRECATED_VERSION_6_15
#endif

#if QT_WARN_DEPRECATED_UP_TO >= QT_VERSION_CHECK(6, 16, 0)
# define QT_DEPRECATED_VERSION_X_6_16(text) QT_DEPRECATED_X(text)
# define QT_DEPRECATED_VERSION_6_16         QT_DEPRECATED
#else
# define QT_DEPRECATED_VERSION_X_6_16(text)
# define QT_DEPRECATED_VERSION_6_16
#endif

#define QT_DEPRECATED_VERSION_X_5(minor, text)      QT_DEPRECATED_VERSION_X_5_##minor(text)
#define QT_DEPRECATED_VERSION_X(major, minor, text) QT_DEPRECATED_VERSION_X_##major##_##minor(text)

#define QT_DEPRECATED_VERSION_5(minor)      QT_DEPRECATED_VERSION_5_##minor
#define QT_DEPRECATED_VERSION(major, minor) QT_DEPRECATED_VERSION_##major##_##minor

/*
    QT_IF_DEPRECATED_SINCE(major, minor, whenTrue, whenFalse) expands to
    \a whenTrue if the specified (\a major, \a minor) version is less than or
    equal to the deprecation version defined by QT_DISABLE_DEPRECATED_UP_TO,
    and to \a whenFalse otherwise.

    Currently used for QT_INLINE_SINCE(maj, min), but can also be helpful for
    other macros of that kind.

    The implementation uses QT_DEPRECATED_SINCE(maj, min) to define a bunch of
    helper QT_IF_DEPRECATED_SINCE_X_Y macros, which expand to \a whenTrue or
    \a whenFalse depending on the value of QT_DEPRECATED_SINCE.

    If you need to use QT_IF_DEPRECATED_SINCE() for a (major, minor) version,
    that is not yet covered by the list below, you need to copy the definition
    and change the major and minor versions accordingly. For example, for
    version (X, Y), you will need to add

    \code
    #if QT_DEPRECATED_SINCE(X, Y)
    # define QT_IF_DEPRECATED_SINCE_X_Y(whenTrue, whenFalse) whenFalse
    #else
    # define QT_IF_DEPRECATED_SINCE_X_Y(whenTrue, whenFalse) whenTrue
    #endif
    \endcode
*/

#define QT_IF_DEPRECATED_SINCE(major, minor, whenTrue, whenFalse) \
    QT_IF_DEPRECATED_SINCE_ ## major ## _ ## minor(whenTrue, whenFalse)

#if QT_DEPRECATED_SINCE(6, 0)
# define QT_IF_DEPRECATED_SINCE_6_0(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_0(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 1)
# define QT_IF_DEPRECATED_SINCE_6_1(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_1(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 2)
# define QT_IF_DEPRECATED_SINCE_6_2(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_2(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 3)
# define QT_IF_DEPRECATED_SINCE_6_3(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_3(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 4)
# define QT_IF_DEPRECATED_SINCE_6_4(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_4(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 5)
# define QT_IF_DEPRECATED_SINCE_6_5(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_5(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 6)
# define QT_IF_DEPRECATED_SINCE_6_6(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_6(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 7)
# define QT_IF_DEPRECATED_SINCE_6_7(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_7(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 8)
# define QT_IF_DEPRECATED_SINCE_6_8(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_8(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 9)
# define QT_IF_DEPRECATED_SINCE_6_9(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_9(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 10)
# define QT_IF_DEPRECATED_SINCE_6_10(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_10(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 11)
# define QT_IF_DEPRECATED_SINCE_6_11(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_11(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 12)
# define QT_IF_DEPRECATED_SINCE_6_12(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_12(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 13)
# define QT_IF_DEPRECATED_SINCE_6_13(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_13(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 14)
# define QT_IF_DEPRECATED_SINCE_6_14(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_14(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 15)
# define QT_IF_DEPRECATED_SINCE_6_15(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_15(whenTrue, whenFalse) whenTrue
#endif

#if QT_DEPRECATED_SINCE(6, 16)
# define QT_IF_DEPRECATED_SINCE_6_16(whenTrue, whenFalse) whenFalse
#else
# define QT_IF_DEPRECATED_SINCE_6_16(whenTrue, whenFalse) whenTrue
#endif

#ifdef __cplusplus
// A tag to help mark stuff deprecated (cf. QStringViewLiteral)
namespace QtPrivate {
inline QT_DEFINE_TAG(Deprecated);
}
#endif

#ifdef QT_ASCII_CAST_WARNINGS
#  define QT_ASCII_CAST_WARN \
    Q_DECL_DEPRECATED_X("Use fromUtf8, QStringLiteral, or QLatin1StringView")
#else
#  define QT_ASCII_CAST_WARN
#endif

QT_END_NAMESPACE

#endif // QTDEPRECATIONMARKERS_H
