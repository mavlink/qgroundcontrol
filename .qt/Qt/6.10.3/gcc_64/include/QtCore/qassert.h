// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QASSERT_H
#define QASSERT_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtcoreexports.h>
#include <QtCore/qtnoop.h>

#if 0
#pragma qt_class(QtAssert)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

#if defined(__cplusplus)

#if !defined(Q_CC_MSVC_ONLY)
Q_NORETURN
#endif
Q_DECL_COLD_FUNCTION
Q_CORE_EXPORT void qt_assert(const char *assertion, const char *file, int line) noexcept;

#if !defined(Q_ASSERT)
#  if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#    define Q_ASSERT(cond) static_cast<void>(false && (cond))
#  else
#    define Q_ASSERT(cond) ((cond) ? static_cast<void>(0) : QT_PREPEND_NAMESPACE(qt_assert)(#cond, __FILE__, __LINE__))
#  endif
#endif

#if !defined(Q_CC_MSVC_ONLY)
Q_NORETURN
#endif
Q_DECL_COLD_FUNCTION
Q_CORE_EXPORT
void qt_assert_x(const char *where, const char *what, const char *file, int line) noexcept;
inline bool qt_no_assert_x(bool, const char *, const char *) noexcept { return false; }

#if !defined(Q_ASSERT_X)
#  if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#    define Q_ASSERT_X(cond, where, what) \
        static_cast<void>(false && QT_PREPEND_NAMESPACE(qt_no_assert_x)(bool(cond), where, what))
#  else
#    define Q_ASSERT_X(cond, where, what) ((cond) ? static_cast<void>(0) : QT_PREPEND_NAMESPACE(qt_assert_x)(where, what, __FILE__, __LINE__))
#  endif
#endif

#ifndef Q_PRE
# define Q_PRE(cond) \
    Q_ASSERT(cond) /* for now... */
#endif

#ifndef Q_PRE_X
# define Q_PRE_X(cond, what) \
    Q_ASSERT_X(cond, Q_FUNC_INFO, what) /* for now... */
#endif

Q_NORETURN Q_CORE_EXPORT void qt_check_pointer(const char *, int) noexcept;
Q_NORETURN Q_DECL_COLD_FUNCTION
Q_CORE_EXPORT void qBadAlloc();

#ifdef QT_NO_EXCEPTIONS
#  if defined(QT_NO_DEBUG) && !defined(QT_FORCE_ASSERTS)
#    define Q_CHECK_PTR(p) qt_noop()
#  else
#    define Q_CHECK_PTR(p) do {if (!(p)) QT_PREPEND_NAMESPACE(qt_check_pointer)(__FILE__,__LINE__);} while (false)
#  endif
#else
#  define Q_CHECK_PTR(p) do { if (!(p)) QT_PREPEND_NAMESPACE(qBadAlloc)(); } while (false)
#endif

template <typename T>
inline T *q_check_ptr(T *p) { Q_CHECK_PTR(p); return p; }

// Q_UNREACHABLE_IMPL() and Q_ASSUME_IMPL() used below are defined in qcompilerdetection.h
#define Q_UNREACHABLE() \
    do {\
        Q_ASSERT_X(false, "Q_UNREACHABLE()", "Q_UNREACHABLE was reached");\
        Q_UNREACHABLE_IMPL();\
    } while (false)

#ifndef Q_UNREACHABLE_RETURN
#  ifdef Q_COMPILER_COMPLAINS_ABOUT_RETURN_AFTER_UNREACHABLE
#    define Q_UNREACHABLE_RETURN(...) Q_UNREACHABLE()
#  else
#    define Q_UNREACHABLE_RETURN(...) do { Q_UNREACHABLE(); return __VA_ARGS__; } while (0)
#  endif
#endif

Q_DECL_DEPRECATED_X("Q_ASSUME() is deprecated because it can produce worse code than when it's absent; "
                    "use C++23 [[assume]] instead")
inline bool qt_assume_is_deprecated(bool cond) noexcept { return cond; }
#define Q_ASSUME(Expr) \
    [] (bool valueOfExpression) {\
        Q_ASSERT_X(valueOfExpression, "Q_ASSUME()", "Assumption in Q_ASSUME(\"" #Expr "\") was not correct");\
        Q_ASSUME_IMPL(valueOfExpression);\
    }(qt_assume_is_deprecated(Expr))


#if __has_builtin(__builtin_assume)
// Clang has this intrinsic and won't warn about its use in C++20 mode
#  define Q_PRESUME_IMPL(assumption) __builtin_assume(assumption)
#elif __has_cpp_attribute(assume)
// GCC has implemented this attribute and allows its use in C++20 mode
#  define Q_PRESUME_IMPL(assumption) [[assume(assumption)]]
#elif defined(Q_CC_MSVC)
#  define Q_PRESUME_IMPL(assumption) __assume(assumption)
#else
#  define Q_PRESUME_IMPL(assumption) (void)0
#endif

#define Q_PRESUME(assumption)       \
    [&] {                            \
        Q_ASSERT(assumption);       \
        Q_PRESUME_IMPL(assumption); \
    }()

// Don't use these in C++ mode, use static_assert directly.
// These are here only to keep old code compiling.
#  define Q_STATIC_ASSERT(Condition) static_assert(bool(Condition), #Condition)
#  define Q_STATIC_ASSERT_X(Condition, Message) static_assert(bool(Condition), Message)

#elif defined(Q_COMPILER_STATIC_ASSERT)
// C11 mode - using the _S version in case <assert.h> doesn't do the right thing
#  define Q_STATIC_ASSERT(Condition) _Static_assert(!!(Condition), #Condition)
#  define Q_STATIC_ASSERT_X(Condition, Message) _Static_assert(!!(Condition), Message)
#else
// C89 & C99 version
#  define Q_STATIC_ASSERT_PRIVATE_JOIN(A, B) Q_STATIC_ASSERT_PRIVATE_JOIN_IMPL(A, B)
#  define Q_STATIC_ASSERT_PRIVATE_JOIN_IMPL(A, B) A ## B
#  ifdef __COUNTER__
#  define Q_STATIC_ASSERT(Condition) \
    typedef char Q_STATIC_ASSERT_PRIVATE_JOIN(q_static_assert_result, __COUNTER__) [(Condition) ? 1 : -1];
#  else
#  define Q_STATIC_ASSERT(Condition) \
    typedef char Q_STATIC_ASSERT_PRIVATE_JOIN(q_static_assert_result, __LINE__) [(Condition) ? 1 : -1];
#  endif /* __COUNTER__ */
#  define Q_STATIC_ASSERT_X(Condition, Message) Q_STATIC_ASSERT(Condition)
#endif // __cplusplus

QT_END_NAMESPACE

#endif // QASSERT_H
