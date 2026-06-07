// Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSTRINGALGORITHMS_H
#define QSTRINGALGORITHMS_H

#include <QtCore/qbytearrayalgorithms.h>
#include <QtCore/qcontainerfwd.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qstringfwd.h>
#if 0
#pragma qt_class(QStringAlgorithms)
#endif

#include <algorithm>        // std::find
#include <iterator>         // std::size

#include <QtCore/q20type_traits.h>      // q20::is_constant_evaluated

QT_BEGIN_NAMESPACE

namespace QtPrivate {

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype qustrlen(const char16_t *str) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype qustrnlen(const char16_t *str, qsizetype maxlen) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION const char16_t *qustrchr(QStringView str, char16_t ch) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION const char16_t *qustrcasechr(QStringView str, char16_t ch) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QStringView   lhs, QStringView   rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QStringView   lhs, QLatin1StringView rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QStringView   lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QLatin1StringView lhs, QStringView   rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QLatin1StringView lhs, QLatin1StringView rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QLatin1StringView lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QBasicUtf8StringView<false> lhs, QStringView   rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QBasicUtf8StringView<false> lhs, QLatin1StringView rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION int compareStrings(QBasicUtf8StringView<false> lhs, QBasicUtf8StringView<false> rhs, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QStringView   lhs, QStringView   rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QStringView   lhs, QLatin1StringView rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QStringView   lhs, QBasicUtf8StringView<false> rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QLatin1StringView lhs, QStringView   rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QLatin1StringView lhs, QLatin1StringView rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QLatin1StringView lhs, QBasicUtf8StringView<false> rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QBasicUtf8StringView<false> lhs, QStringView   rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QBasicUtf8StringView<false> lhs, QLatin1StringView rhs) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool equalStrings(QBasicUtf8StringView<false> lhs, QBasicUtf8StringView<false> rhs) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QStringView   haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QStringView   haystack, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QLatin1StringView haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool startsWith(QLatin1StringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QStringView   haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QStringView   haystack, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QLatin1StringView haystack, QStringView   needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool endsWith(QLatin1StringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]]                             inline qsizetype findString(QStringView str, qsizetype from, QChar needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QStringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QStringView haystack, qsizetype from, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QLatin1StringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype findString(QLatin1StringView haystack, qsizetype from, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QStringView haystack, qsizetype from, char16_t needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QStringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QStringView haystack, qsizetype from, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QLatin1StringView haystack, qsizetype from, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype lastIndexOf(QLatin1StringView haystack, qsizetype from, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION QStringView   trimmed(QStringView   s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION QLatin1StringView trimmed(QLatin1StringView s) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isLower(QStringView s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isUpper(QStringView s) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype count(QStringView haystack, QChar needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype count(QStringView haystack, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype count(QStringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive);
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype count(QLatin1StringView haystack, QLatin1StringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive);
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype count(QLatin1StringView haystack, QStringView needle, Qt::CaseSensitivity cs = Qt::CaseSensitive);
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION qsizetype count(QLatin1StringView haystack, QChar needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;

#if QT_CONFIG(regularexpression)
// ### Qt 7: unify these overloads;
// remove the ones taking only a QStringView, export the others, adjust callers
[[nodiscard]] qsizetype indexOf(QStringView viewHaystack,
                                const QString *stringHaystack,
                                const QRegularExpression &re,
                                qsizetype from = 0,
                                QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] Q_CORE_EXPORT qsizetype indexOf(QStringView haystack,
                                              const QRegularExpression &re,
                                              qsizetype from = 0,
                                              QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] qsizetype lastIndexOf(QStringView viewHaystack,
                                    const QString *stringHaystack,
                                    const QRegularExpression &re,
                                    qsizetype from = -1,
                                    QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] Q_CORE_EXPORT qsizetype lastIndexOf(QStringView haystack,
                                                  const QRegularExpression &re,
                                                  qsizetype from = -1,
                                                  QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] bool contains(QStringView viewHaystack,
                            const QString *stringHaystack,
                            const QRegularExpression &re,
                            QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] Q_CORE_EXPORT bool contains(QStringView haystack,
                                          const QRegularExpression &re,
                                          QRegularExpressionMatch *rmatch = nullptr);
[[nodiscard]] Q_CORE_EXPORT qsizetype count(QStringView haystack, const QRegularExpression &re);
#endif

[[nodiscard]] Q_CORE_EXPORT QString convertToQString(QAnyStringView s);

[[nodiscard]] Q_CORE_EXPORT QByteArray convertToLatin1(QStringView str);
[[nodiscard]] Q_CORE_EXPORT QByteArray convertToUtf8(QStringView str);
[[nodiscard]] Q_CORE_EXPORT QByteArray convertToLocal8Bit(QStringView str);
[[nodiscard]] Q_CORE_EXPORT QList<uint> convertToUcs4(QStringView str); // ### Qt 7 char32_t

[[nodiscard]] Q_CORE_EXPORT QByteArray convertToUtf8(QLatin1StringView str);

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isRightToLeft(QStringView string) noexcept;

[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isAscii(QLatin1StringView s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isAscii(QStringView   s) noexcept;
[[nodiscard]] constexpr inline                   bool isLatin1(QLatin1StringView s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isLatin1(QStringView   s) noexcept;
[[nodiscard]] Q_CORE_EXPORT Q_DECL_PURE_FUNCTION bool isValidUtf16(QStringView s) noexcept;

template <typename Char, size_t N> [[nodiscard]] Q_ALWAYS_INLINE constexpr
qsizetype lengthHelperContainerLoop(const Char (&str)[N])
{
#if defined(__cpp_lib_constexpr_algorithms) && defined(Q_CC_GNU_ONLY)
    // libstdc++'s std::find / std::find_if manages to execute more steps
    // than the loop below
    const auto it = std::find(str, str + N, Char(0));
    return it - str;
#else
    // std::char_traits<C> is deprecated for C not one of the standard char
    // types, so we have to roll out our own loop.
    for (size_t i = 0; i < N; ++i) {
        if (str[i] == Char(0))
            return qsizetype(i);
    }
    return qsizetype(N);
#endif
}

template <typename Char, size_t N> [[nodiscard]] Q_ALWAYS_INLINE constexpr
std::enable_if_t<sizeof(Char) == sizeof(char16_t), qsizetype>
lengthHelperContainer(const Char (&str)[N])
{
    // The following values were empirically determined to detect the threshold
    // at which the compiler gives up pre-calculating the std::find() below and
    // instead inserts code to be executed at runtime.
    constexpr size_t RuntimeThreshold =
#if defined(Q_CC_CLANG)
            // tested on Clang 15, 16 & 17
            1023
#elif defined(Q_CC_GNU)
            // tested through GCC 13.1 at -O3 compilation level
            // note: at -O2, GCC always generates a loop!
            __cplusplus >= 202002L ? 39 : 17
#else
            0
#endif
            ;
    if constexpr (N == 1) {
        return str[0] == Char(0) ? 0 : 1;
    } else if constexpr (N > RuntimeThreshold) {
#ifdef QT_SUPPORTS_IS_CONSTANT_EVALUATED
        if (!q20::is_constant_evaluated())
            return QtPrivate::qustrnlen(reinterpret_cast<const char16_t *>(str), N);
#endif
    }

    return lengthHelperContainerLoop(str);
}

inline qsizetype qstrnlen_helper(const char *str, size_t maxlen)
{
#if !defined(Q_COMPILER_SLOW_QSTRNLEN_COMPILATION)
    return qstrnlen(str, maxlen);
#else
    return strnlen_s(str, maxlen);
#endif
}

template <typename Char, size_t N> [[nodiscard]] constexpr inline
std::enable_if_t<sizeof(Char) == 1, qsizetype> lengthHelperContainer(const Char (&str)[N])
{
#ifdef QT_SUPPORTS_IS_CONSTANT_EVALUATED
    if (!q20::is_constant_evaluated())
        return qstrnlen_helper(reinterpret_cast<const char *>(str), N);
#endif

    return lengthHelperContainerLoop(str);
}

template <typename Container>
constexpr qsizetype lengthHelperContainer(const Container &c) noexcept
{
    return qsizetype(std::size(c));
}
} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QSTRINGALGORTIHMS_H
