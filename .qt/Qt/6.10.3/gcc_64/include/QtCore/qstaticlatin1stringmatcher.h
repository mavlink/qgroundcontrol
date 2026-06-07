// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSTATICLATIN1STRINGMATCHER_H
#define QSTATICLATIN1STRINGMATCHER_H

#include <functional>
#include <iterator>
#include <limits>

#include <QtCore/q20algorithm.h>
#include <QtCore/qlatin1stringmatcher.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

#ifdef Q_CC_GHS
#  define QT_STATIC_BOYER_MOORE_NOT_SUPPORTED
#else
namespace QtPrivate {
template <class RandomIt1,
          class Hash = std::hash<typename std::iterator_traits<RandomIt1>::value_type>,
          class BinaryPredicate = std::equal_to<>>
class q_boyer_moore_searcher
{
public:
    constexpr q_boyer_moore_searcher(RandomIt1 pat_first, RandomIt1 pat_last) : m_skiptable{}
    {
        const size_t n = std::distance(pat_first, pat_last);
        constexpr auto uchar_max = (std::numeric_limits<uchar>::max)();
        uchar max = n > uchar_max ? uchar_max : uchar(n);
        q20::fill(std::begin(m_skiptable), std::end(m_skiptable), max);
        Hash hf;
        RandomIt1 pattern = std::next(pat_first, n - max);
        while (max--)
            m_skiptable[hf(*pattern++)] = max;
    }

    template <class RandomIt2>
    constexpr auto operator()(RandomIt2 first, RandomIt2 last, RandomIt1 pat_first,
                              RandomIt1 pat_last) const
    {
        struct R
        {
            RandomIt2 begin, end;
        };
        Hash hf;
        BinaryPredicate pred;
        auto pat_length = std::distance(pat_first, pat_last);
        if (pat_length == 0)
            return R{ first, first };

        auto haystack_length = std::distance(first, last);
        if (haystack_length < pat_length)
            return R{ last, last };

        const qsizetype pl_minus_one = qsizetype(pat_length - 1);
        RandomIt2 current = first + pl_minus_one;

        qsizetype skip = 0;
        while (current < last - skip) {
            current += skip;
            skip = m_skiptable[hf(*current)];
            if (!skip) {
                // possible match
                while (skip < pat_length) {
                    if (!pred(hf(*(current - skip)), hf(pat_first[pl_minus_one - skip])))
                        break;
                    skip++;
                }
                if (skip > pl_minus_one) { // we have a match
                    auto match = current + 1 - skip;
                    return R{ match, match + pat_length };
                }

                // If we don't have a match we are a bit inefficient as we only skip by one
                // when we have the non matching char in the string.
                if (m_skiptable[hf(*(current - skip))] == pat_length)
                    skip = pat_length - skip;
                else
                    skip = 1;
            }
        }

        return R{ last, last };
    }

private:
    alignas(16) uchar m_skiptable[256];
};
} // namespace QtPrivate

template <Qt::CaseSensitivity CS, size_t N>
class QStaticLatin1StringMatcher
{
    static_assert(N > 2,
                  "QStaticLatin1StringMatcher makes no sense for finding a single-char pattern");

    QLatin1StringView m_pattern;
    using Hasher = std::conditional_t<CS == Qt::CaseSensitive, QtPrivate::QCaseSensitiveLatin1Hash,
                                      QtPrivate::QCaseInsensitiveLatin1Hash>;
    QtPrivate::q_boyer_moore_searcher<const char *, Hasher> m_searcher;

public:
    explicit constexpr QStaticLatin1StringMatcher(QLatin1StringView patternToMatch) noexcept
        : m_pattern(patternToMatch),
          m_searcher(patternToMatch.begin(), patternToMatch.begin() + N - 1)
    {
    }

    constexpr qsizetype indexIn(QLatin1StringView haystack, qsizetype from = 0) const noexcept
    { return indexIn_helper(haystack, from); }

    constexpr qsizetype indexIn(QStringView haystack, qsizetype from = 0) const noexcept
    { return indexIn_helper(haystack, from); }

private:
    template <typename String>
    constexpr qsizetype indexIn_helper(String haystack, qsizetype from = 0) const noexcept
    {
        static_assert(QtPrivate::isLatin1OrUtf16View<String>);

        if (from >= haystack.size())
            return -1;

        const auto start = [haystack]() constexpr {
            if constexpr (std::is_same_v<String, QStringView>)
                return haystack.utf16();
            else
                return haystack.begin();
        }();
        const auto begin = start + from;
        const auto end = start + haystack.size();
        const auto r = m_searcher(begin, end, m_pattern.begin(), m_pattern.end());
        return r.begin == end ? -1 : std::distance(start, r.begin);
    }
};

template <size_t N>
constexpr auto qMakeStaticCaseSensitiveLatin1StringMatcher(const char (&patternToMatch)[N]) noexcept
{
    return QStaticLatin1StringMatcher<Qt::CaseSensitive, N>(
            QLatin1StringView(patternToMatch, qsizetype(N) - 1));
}

template <size_t N>
constexpr auto
qMakeStaticCaseInsensitiveLatin1StringMatcher(const char (&patternToMatch)[N]) noexcept
{
    return QStaticLatin1StringMatcher<Qt::CaseInsensitive, N>(
            QLatin1StringView(patternToMatch, qsizetype(N) - 1));
}
#endif

QT_END_NAMESPACE

#endif // QSTATICLATIN1STRINGMATCHER_H
