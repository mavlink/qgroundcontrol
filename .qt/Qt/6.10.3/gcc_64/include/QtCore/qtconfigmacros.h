// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTCONFIGMACROS_H
#define QTCONFIGMACROS_H

#if 0
#  pragma qt_sync_stop_processing
#endif

#include <QtCore/qtconfiginclude.h>
#include <QtCore/qtdeprecationdefinitions.h>
#include <QtCore/qtversionchecks.h>

#include <assert.h>

/*
   The Qt modules' export macros.
   The options are:
    - defined(QT_STATIC): Qt was built or is being built in static mode
    - defined(QT_SHARED): Qt was built or is being built in shared/dynamic mode
   If neither was defined, then QT_SHARED is implied. If Qt was compiled in static
   mode, QT_STATIC is defined in qconfig.h. In shared mode, QT_STATIC is implied
   for the bootstrapped tools.
*/

#ifdef QT_BOOTSTRAPPED
#  ifdef QT_SHARED
#    error "QT_SHARED and QT_BOOTSTRAPPED together don't make sense. Please fix the build"
#  elif !defined(QT_STATIC)
#    define QT_STATIC
#  endif
#endif

#if defined(QT_SHARED) || !defined(QT_STATIC)
#  ifdef QT_STATIC
#    error "Both QT_SHARED and QT_STATIC defined, please make up your mind"
#  endif
#  ifndef QT_SHARED
#    define QT_SHARED
#  endif
#endif

/*
   No, this is not an evil backdoor. QT_BUILD_INTERNAL just exports more symbols
   for Qt's internal unit tests. If you want slower loading times and more
   symbols that can vanish from version to version, feel free to define QT_BUILD_INTERNAL.

   \note After adding Q_AUTOTEST_EXPORT to a method, you'll need to wrap any unittests
   that will use that method in "#ifdef QT_BUILD_INTERNAL".
*/
#if defined(QT_BUILD_INTERNAL) && defined(QT_BUILDING_QT) && defined(QT_SHARED)
#    define Q_AUTOTEST_EXPORT Q_DECL_EXPORT
#elif defined(QT_BUILD_INTERNAL) && defined(QT_SHARED)
#    define Q_AUTOTEST_EXPORT Q_DECL_IMPORT
#else
#    define Q_AUTOTEST_EXPORT
#endif

/*
    The QT_CONFIG macro implements a safe compile time check for features of Qt.
    Features can be in three states:
        0 or undefined: This will lead to a compile error when testing for it
        -1: The feature is not available
        1: The feature is available
*/
#define QT_CONFIG(feature) (1/QT_FEATURE_##feature == 1)
#define QT_REQUIRE_CONFIG(feature) static_assert(QT_FEATURE_##feature == 1, "Required feature " #feature " for file " __FILE__ " not available.")

/* moc compats (signals/slots) */
#ifndef QT_MOC_COMPAT
#  define QT_MOC_COMPAT
#else
#  undef QT_MOC_COMPAT
#  define QT_MOC_COMPAT
#endif

/*
   Debugging and error handling
*/

#if !defined(QT_NO_DEBUG) && !defined(QT_DEBUG)
#  define QT_DEBUG
#endif

// valid for both C and C++
#define QT_MANGLE_NAMESPACE0(x) x
#define QT_MANGLE_NAMESPACE1(a, b) a##_##b
#define QT_MANGLE_NAMESPACE2(a, b) QT_MANGLE_NAMESPACE1(a,b)
#if !defined(QT_NAMESPACE) || defined(Q_MOC_RUN) /* user namespace */
# define QT_MANGLE_NAMESPACE(name) name
#else
# define QT_MANGLE_NAMESPACE(name) QT_MANGLE_NAMESPACE2( \
        QT_MANGLE_NAMESPACE0(name), QT_MANGLE_NAMESPACE0(QT_NAMESPACE))
#endif

#ifdef __cplusplus

#if !defined(QT_NAMESPACE) || defined(Q_MOC_RUN) /* user namespace */

# define QT_PREPEND_NAMESPACE(name) ::name
# define QT_USE_NAMESPACE
# define QT_BEGIN_NAMESPACE
# define QT_END_NAMESPACE
# define QT_BEGIN_INCLUDE_NAMESPACE
# define QT_END_INCLUDE_NAMESPACE
# define QT_FORWARD_DECLARE_CLASS(name) class name;
# define QT_FORWARD_DECLARE_STRUCT(name) struct name;

#elif defined(QT_INLINE_NAMESPACE) /* user inline namespace FIXME in Qt 7: Default */

# define QT_PREPEND_NAMESPACE(name) ::QT_NAMESPACE::name
# define QT_USE_NAMESPACE
# define QT_BEGIN_NAMESPACE inline namespace QT_NAMESPACE {
# define QT_END_NAMESPACE }
# define QT_BEGIN_INCLUDE_NAMESPACE }
# define QT_END_INCLUDE_NAMESPACE inline namespace QT_NAMESPACE {
# define QT_FORWARD_DECLARE_CLASS(name) \
QT_BEGIN_NAMESPACE class name; QT_END_NAMESPACE

# define QT_FORWARD_DECLARE_STRUCT(name) \
QT_BEGIN_NAMESPACE struct name; QT_END_NAMESPACE

inline namespace QT_NAMESPACE {}

#else /* user namespace */

# define QT_PREPEND_NAMESPACE(name) ::QT_NAMESPACE::name
# define QT_USE_NAMESPACE using namespace ::QT_NAMESPACE;
# define QT_BEGIN_NAMESPACE namespace QT_NAMESPACE {
# define QT_END_NAMESPACE }
# define QT_BEGIN_INCLUDE_NAMESPACE }
# define QT_END_INCLUDE_NAMESPACE namespace QT_NAMESPACE {
# define QT_FORWARD_DECLARE_CLASS(name) \
    QT_BEGIN_NAMESPACE class name; QT_END_NAMESPACE \
    using QT_PREPEND_NAMESPACE(name);

# define QT_FORWARD_DECLARE_STRUCT(name) \
    QT_BEGIN_NAMESPACE struct name; QT_END_NAMESPACE \
    using QT_PREPEND_NAMESPACE(name);

namespace QT_NAMESPACE {}

# ifndef QT_BOOTSTRAPPED
# ifndef QT_NO_USING_NAMESPACE
   /*
    This expands to a "using QT_NAMESPACE" also in _header files_.
    It is the only way the feature can be used without too much
    pain, but if people _really_ do not want it they can add
    QT_NO_USING_NAMESPACE to their build configuration.
    */
   QT_USE_NAMESPACE
# endif
# endif

#endif /* user namespace */

#else /* __cplusplus */

# define QT_BEGIN_NAMESPACE
# define QT_END_NAMESPACE
# define QT_USE_NAMESPACE
# define QT_BEGIN_INCLUDE_NAMESPACE
# define QT_END_INCLUDE_NAMESPACE

#endif /* __cplusplus */

/* ### Qt 6.9 (or later): remove *_MOC_* macros (moc does not need them since 6.5) */
#ifndef QT_BEGIN_MOC_NAMESPACE
# define QT_BEGIN_MOC_NAMESPACE QT_USE_NAMESPACE
#endif
#ifndef QT_END_MOC_NAMESPACE
# define QT_END_MOC_NAMESPACE
#endif

/*
    Strict mode
*/
#ifdef QT_ENABLE_STRICT_MODE_UP_TO
#ifndef QT_DISABLE_DEPRECATED_UP_TO
#  define QT_DISABLE_DEPRECATED_UP_TO QT_ENABLE_STRICT_MODE_UP_TO
#endif

#if QT_ENABLE_STRICT_MODE_UP_TO >= QT_VERSION_CHECK(6, 0, 0)
# ifndef QT_NO_FOREACH
#  define QT_NO_FOREACH
# endif
# ifndef QT_NO_CAST_TO_ASCII
#  define QT_NO_CAST_TO_ASCII
# endif
# ifndef QT_NO_CAST_FROM_BYTEARRAY
#  define QT_NO_CAST_FROM_BYTEARRAY
# endif
# ifndef QT_NO_URL_CAST_FROM_STRING
#  define QT_NO_URL_CAST_FROM_STRING
# endif
# ifndef QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
#  define QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
# endif
# ifndef QT_NO_JAVA_STYLE_ITERATORS
#  define QT_NO_JAVA_STYLE_ITERATORS
# endif
#endif // 6.0.0

#if QT_ENABLE_STRICT_MODE_UP_TO >= QT_VERSION_CHECK(6, 6, 0)
# ifndef QT_NO_QEXCHANGE
#  define QT_NO_QEXCHANGE
# endif
#endif // 6.6.0

#if QT_ENABLE_STRICT_MODE_UP_TO >= QT_VERSION_CHECK(6, 7, 0)
# ifndef QT_NO_CONTEXTLESS_CONNECT
#  define QT_NO_CONTEXTLESS_CONNECT
# endif
#endif // 6.7.0

#if QT_ENABLE_STRICT_MODE_UP_TO >= QT_VERSION_CHECK(6, 8, 0)
# ifndef QT_NO_QASCONST
#  define QT_NO_QASCONST
# endif
#  if !defined(QT_USE_NODISCARD_FILE_OPEN) && !defined(QT_NO_USE_NODISCARD_FILE_OPEN)
#    define QT_USE_NODISCARD_FILE_OPEN
#  endif
#endif // 6.8.0

#if QT_ENABLE_STRICT_MODE_UP_TO >= QT_VERSION_CHECK(6, 9, 0)
# ifndef QT_NO_QSNPRINTF
#  define QT_NO_QSNPRINTF
# endif
#endif // 6.9.0
#endif // QT_ENABLE_STRICT_MODE_UP_TO

#endif /* QTCONFIGMACROS_H */
