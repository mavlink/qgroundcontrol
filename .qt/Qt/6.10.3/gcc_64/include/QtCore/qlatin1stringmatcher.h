// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QLATIN1STRINGMATCHER_H
#define QLATIN1STRINGMATCHER_H

#include <functional>
#include <iterator>
#include <limits>

#include <QtCore/q20algorithm.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {
template <typename T> constexpr inline bool isLatin1OrUtf16View = false;
template <> constexpr inline bool isLatin1OrUtf16View<QLatin1StringView> = true;
template <> constexpr inline bool isLatin1OrUtf16View<QStringView> = true;

template<class RandomIt1,
         class Hash = std::hash<typename std::iterator_traits<RandomIt1>::value_type>,
         class BinaryPredicate = std::equal_to<>>
class q_boyer_moore_searcher_hashed_needle
{
public:
    constexpr q_boyer_moore_searcher_hashed_needle(RandomIt1 pat_first, RandomIt1 pat_last)
        : m_skiptable{}
    {
        const size_t n = std::distance(pat_first, pat_last);
        constexpr auto uchar_max = (std::numeric_limits<uchar>::max)();
        uchar max = n > uchar_max ? uchar_max : uchar(n);
        q20::fill(std::begin(m_skiptable), std::end(m_skiptable), max);

        RandomIt1 pattern = pat_first;
        pattern += n - max;
        while (max--)
            m_skiptable[uchar(*pattern++)] = max;
    }

    template<class RandomIt2>
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

        const qsizetype pl_minus_one = qsizetype(pat_length - 1);
        RandomIt2 current = first + pl_minus_one;

        while (current < last) {
            qsizetype skip = m_skiptable[hf(*current)];
            if (!skip) {
                // possible match
                while (skip < pat_length) {
                    if (!pred(hf(*(current - skip)), uchar(pat_first[pl_minus_one - skip])))
                        break;
                    skip++;
                }
                if (skip > pl_minus_one) { // we have a match
                    auto match = current - skip + 1;
                    return R{ match, match + pat_length };
                }

                // If we don't have a match we are a bit inefficient as we only skip by one
                // when we have the non matching char in the string.
                if (m_skiptable[hf(*(current - skip))] == pat_length)
                    skip = pat_length - skip;
                else
                    skip = 1;
            }
            current += skip;
        }

        return R{ last, last };
    }

private:
    alignas(16) uchar m_skiptable[256];
};

struct QCaseSensitiveLatin1Hash
{
    constexpr QCaseSensitiveLatin1Hash() = default;

    constexpr std::size_t operator()(char c) const noexcept { return std::size_t(uchar(c)); }
};

struct QCaseInsensitiveLatin1Hash
{
    constexpr QCaseInsensitiveLatin1Hash() = default;

    constexpr std::size_t operator()(char c) const noexcept
    {
        return std::size_t(latin1Lower[uchar(c)]);
    }

    static int difference(char lhs, char rhs)
    {
        return int(latin1Lower[uchar(lhs)]) - int(latin1Lower[uchar(rhs)]);
    }

    static auto matcher(char ch)
    {
        return [sought = latin1Lower[uchar(ch)]](char other) {
            return latin1Lower[uchar(other)] == sought;
        };
    }

private:
    static constexpr uchar latin1Lower[256] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
        0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
        0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
        0x40,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x5b,0x5c,0x5d,0x5e,0x5f,
        0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
        0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
        0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
        0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
        0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
        // 0xd7 (multiplication sign) and 0xdf (sz ligature) complicate life
        0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xd7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xdf,
        0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
    };
};
}

class QLatin1StringMatcher
{
public:
    Q_CORE_EXPORT QLatin1StringMatcher() noexcept;
    Q_CORE_EXPORT explicit QLatin1StringMatcher(
            QLatin1StringView pattern, Qt::CaseSensitivity cs = Qt::CaseSensitive) noexcept;
    Q_CORE_EXPORT ~QLatin1StringMatcher() noexcept;

    Q_CORE_EXPORT void setPattern(QLatin1StringView pattern) noexcept;
    Q_CORE_EXPORT QLatin1StringView pattern() const noexcept;
    Q_CORE_EXPORT void setCaseSensitivity(Qt::CaseSensitivity cs) noexcept;
    Q_CORE_EXPORT Qt::CaseSensitivity caseSensitivity() const noexcept;

    Q_CORE_EXPORT qsizetype indexIn(QLatin1StringView haystack, qsizetype from = 0) const noexcept;
    Q_CORE_EXPORT qsizetype indexIn(QStringView haystack, qsizetype from = 0) const noexcept;

private:
    void setSearcher() noexcept;
    void freeSearcher() noexcept;

    QLatin1StringView m_pattern;
    Qt::CaseSensitivity m_cs;
    typedef QtPrivate::q_boyer_moore_searcher_hashed_needle<const char *,
                                                            QtPrivate::QCaseSensitiveLatin1Hash>
            CaseSensitiveSearcher;
    typedef QtPrivate::q_boyer_moore_searcher_hashed_needle<const char *,
                                                            QtPrivate::QCaseInsensitiveLatin1Hash>
            CaseInsensitiveSearcher;
    union {
        CaseSensitiveSearcher m_caseSensitiveSearcher;
        CaseInsensitiveSearcher m_caseInsensitiveSearcher;
    };

    template <typename String>
    qsizetype indexIn_helper(String haystack, qsizetype from) const noexcept;

    char m_foldBuffer[256];
};

QT_END_NAMESPACE

#endif // QLATIN1MATCHER_H
