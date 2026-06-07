// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QSTRINGALGORITHMS_P_H
#define QSTRINGALGORITHMS_P_H

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

#include "qspan.h"
#include "qstring.h"
#include "qlocale_p.h"      // for ascii_isspace

QT_BEGIN_NAMESPACE

template <typename StringType> struct QStringAlgorithms
{
    typedef typename StringType::value_type Char;
    typedef typename StringType::size_type size_type;
    typedef typename std::remove_cv<StringType>::type NakedStringType;
    using ViewType =
        std::conditional_t<std::is_same_v<StringType, QString>, QStringView, QByteArrayView>;
    using ViewChar = typename ViewType::storage_type;
    static const bool isConst = std::is_const<StringType>::value;

    static inline bool isSpace(char ch) { return ascii_isspace(ch); }
    static inline bool isSpace(QChar ch) { return ch.isSpace(); }

    // Surrogate pairs are not handled in either of the functions below. That is
    // not a problem because there are no space characters (Zs, Zl, Zp) outside the
    // Basic Multilingual Plane.

    static inline StringType trimmed_helper_inplace(NakedStringType &str, const Char *begin, const Char *end)
    {
        // in-place trimming:
        Char *data = const_cast<Char *>(str.cbegin());
        if (begin != data)
            memmove(data, begin, (end - begin) * sizeof(Char));
        str.resize(end - begin);
        return std::move(str);
    }

    static inline StringType trimmed_helper_inplace(const NakedStringType &, const Char *, const Char *)
    {
        // can't happen
        Q_UNREACHABLE_RETURN(StringType());
    }

    struct TrimPositions {
        const Char *begin;
        const Char *end;
    };
    // Returns {begin, end} where:
    // - "begin" refers to the first non-space character
    // - if there is a sequence of one or more space chacaters at the end,
    //   "end" refers to the first character in that sequence, otherwise
    //   "end" is str.cend()
    [[nodiscard]] static TrimPositions trimmed_helper_positions(const StringType &str)
    {
        const Char *begin = str.cbegin();
        const Char *end = str.cend();
        // skip white space from end
        while (begin < end && isSpace(end[-1]))
            --end;
        // skip white space from start
        while (begin < end && isSpace(*begin))
            begin++;
        return {begin, end};
    }

    static inline StringType trimmed_helper(StringType &str)
    {
        const auto [begin, end] = trimmed_helper_positions(str);
        if (begin == str.cbegin() && end == str.cend())
            return str;
        if (!isConst && str.isDetached())
            return trimmed_helper_inplace(str, begin, end);
        return StringType(begin, end - begin);
    }

    static inline StringType simplified_helper(StringType &str)
    {
        if (str.isEmpty())
            return str;
        const Char *src = str.cbegin();
        const Char *end = str.cend();
        NakedStringType result = isConst || !str.isDetached() ?
                                     StringType(str.size(), Qt::Uninitialized) :
                                     std::move(str);

        Char *dst = const_cast<Char *>(result.cbegin());
        Char *ptr = dst;
        bool unmodified = true;
        while (true) {
            while (src != end && isSpace(*src))
                ++src;
            while (src != end && !isSpace(*src))
                *ptr++ = *src++;
            if (src == end)
                break;
            if (*src != QChar::Space)
                unmodified = false;
            *ptr++ = QChar::Space;
        }
        if (ptr != dst && ptr[-1] == QChar::Space)
            --ptr;

        qsizetype newlen = ptr - dst;
        if (isConst && newlen == str.size() && unmodified) {
            // nothing happened, return the original
            return str;
        }
        result.resize(newlen);
        return result;
    }

    static inline bool needsReallocate(const StringType &str, qsizetype newSize) noexcept
    {
        const auto capacityAtEnd = str.capacity() - str.data_ptr().freeSpaceAtBegin();
        return newSize > capacityAtEnd;
    }

    static inline const ViewChar *asUnicodeChar(ViewType v)
    {
        if constexpr (sizeof(ViewChar) == sizeof(QChar))
            return v.utf16();
        else
            return v.data();
    }

    static inline qsizetype newSize(StringType &src, qsizetype bsize,
                                    ViewType after, QSpan<const qsizetype> indices)
    {
        if (bsize == after.size())
            return src.size();
        else if (bsize > after.size()) // shrink
            return src.size() - indices.size() * (bsize - after.size());

        // bsize < after.size()
        const qsizetype adjust = indices.size() * (after.size() - bsize);
        return src.size() + adjust;
    }

    // {QString,QByteArray}::resize() but without the extra checks
    static inline void setSize(StringType &src, qsizetype newSize)
    {
        Q_ASSERT(src.isDetached());
        Q_ASSERT(newSize <= src.capacity());

        auto &d = src.data_ptr();
        d.size = newSize;
        d.data()[newSize] = '\0';
    }

    // Instead of detaching, i.e. copying the whole data array then performing the
    // replacement, create a new buffer, copy `src` and `after` to it and swap it
    // with `src`.
    static inline void replace_into_copy(StringType &src, qsizetype bsize,
                                         ViewType after, QSpan<const qsizetype> indices,
                                         qsizetype newlen)
    {
        StringType tmp{newlen, Qt::Uninitialized};
        auto *to = tmp.data_ptr().data();
        const auto *a = asUnicodeChar(after);
        auto *const begin = src.data_ptr().data();
        auto *first = begin;

        for (auto i : indices) {
            to = std::copy(first, begin + i, to);
            to = std::copy(a, a + after.size(), to);
            first = begin + i + bsize;
        }
        std::copy(first, src.data_ptr().end(), to); // remainder
        src.swap(tmp);
    }

    static inline void replace_equal_len(StringType &src, [[maybe_unused]] qsizetype bsize,
                                         ViewType after, QSpan<const qsizetype> indices)
    {
        Q_ASSERT(bsize == after.size());
        Q_ASSERT(!src.data_ptr().needsDetach());

        const auto *a = asUnicodeChar(after);
        auto *const begin = src.data_ptr().data();
        // before and after have the same length, so no reallocation
        for (auto i : indices)
            std::copy(a, a + after.size(), begin + i);
    }

    static inline void replace_shrink(StringType &src, qsizetype bsize, ViewType after,
                                      QSpan<const qsizetype> indices)
    {
        Q_ASSERT(bsize > after.size());
        Q_ASSERT(!src.data_ptr().needsDetach());

        const auto *a = asUnicodeChar(after);
        auto *const begin = src.data_ptr().data(); // data(), without the detach() check
        auto *const end = begin + src.size();
        Q_ASSERT(!indices.isEmpty());
        auto *to = std::copy(a, a + after.size(),  begin + indices.front());
        auto *first = begin + indices.front() + bsize;
        for (qsizetype i = 1; i < indices.size(); ++i) {
            auto *last = begin + indices[i];
            to = std::copy(first, last, to);
            qsizetype adjust = i * (bsize - after.size());
            to = std::copy(a, a + after.size(), last - adjust);
            first = begin + indices[i] + bsize;
        }
        to = std::copy(first, end, to);
        setSize(src,to - begin);
    }

    static inline void replace_grow(StringType &src, qsizetype bsize, ViewType after,
                                    QSpan<const qsizetype> indices, qsizetype newlen)
    {
        Q_ASSERT(after.size() > bsize);
        Q_ASSERT(!src.data_ptr().needsDetach());

        // replace in-place, after is longer than before, so replace from the back
        const qsizetype oldlen = src.size();
        const auto *a = asUnicodeChar(after);
        setSize(src, newlen);
        auto *const begin = src.data_ptr().data(); // data(), without the detach() check
        auto *last = begin + oldlen;
        auto *to = src.data_ptr().end();
        for (auto i = indices.size() - 1; i >= 0; --i) {
            auto *first = begin + indices[i] + bsize;
            to = std::copy_backward(first, last, to);
            to = std::copy_backward(a, a + after.size(), to);
            last = begin + indices[i];
        }
    }

    static inline void replace_helper(StringType &src, qsizetype bsize, ViewType after,
                                      QSpan<const qsizetype> indices)
    {
        if (indices.isEmpty())
            return;

        const qsizetype newlen = newSize(src, bsize, after, indices);
        if (src.data_ptr().needsDetach()
            || (bsize < after.size() && needsReallocate(src, newlen))) {
            // Instead of detaching (which would copy the whole data array) then
            // performing the replacement, allocate a new string and copy the data
            // over from `src` and `after` as needed.
            replace_into_copy(src, bsize, after, indices, newlen);
            return;
        }

        // No detaching or reallocation -> change in-place
        if (bsize == after.size())
            replace_equal_len(src, bsize, after, indices);
        else if (bsize > after.size())
            replace_shrink(src, bsize, after, indices);
        else // bsize < after.size()
            replace_grow(src, bsize, after, indices, newlen);
    }
};

QT_END_NAMESPACE

#endif // QSTRINGALGORITHMS_P_H
