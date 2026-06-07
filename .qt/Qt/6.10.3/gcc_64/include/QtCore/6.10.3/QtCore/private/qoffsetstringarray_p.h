// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QOFFSETSTRINGARRAY_P_H
#define QOFFSETSTRINGARRAY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "private/qglobal_p.h"

#include <QByteArrayView>

#include <QtCore/q20algorithm.h>
#include <array>
#include <limits>
#include <string_view>
#include <tuple>

#ifdef __cpp_concepts
#  include <concepts>
#endif

class tst_QOffsetStringArray;

QT_BEGIN_NAMESPACE

QT_WARNING_PUSH
#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 1100
// we usually don't overread, but GCC has a false positive
QT_WARNING_DISABLE_GCC("-Wstringop-overread")
#endif


template <typename StaticString, typename OffsetList>
class QOffsetStringArray
{
    static auto viewType_helper()
    {
        // returning std::type_identity here to avoid having to #include
        if constexpr (sizeof(Char) == 2) {
            return q20::type_identity<QStringView>();
#ifdef __cpp_char8_t
        } else if constexpr (std::is_same_v<Char, char8_t>) {
            return q20::type_identity<QUtf8StringView>();
#endif
        } else {
            return q20::type_identity<QByteArrayView>();
        }
    }

public:
    using Char = typename StaticString::value_type;
    using View = typename decltype(viewType_helper())::type;

    constexpr QOffsetStringArray(const StaticString &string, const OffsetList &offsets)
        : m_string(string), m_offsets(offsets)
    {}

    constexpr const Char *operator[](const int index) const noexcept
    {
        return m_string.data() + m_offsets[qBound(int(0), index, count())];
    }

    constexpr const Char *at(const int index) const noexcept
    {
        return m_string.data() + m_offsets[index];
    }

    constexpr View viewAt(qsizetype index) const noexcept
    {
        return { m_string.data() + m_offsets[index],
                    qsizetype(m_offsets[index + 1]) - qsizetype(m_offsets[index]) - 1 };
    }

    constexpr int count() const { return int(m_offsets.size()) - 1; }

    bool contains(View needle, Qt::CaseSensitivity cs = Qt::CaseSensitive) const noexcept
    {
        for (qsizetype i = 0; i < count(); ++i) {
            if (viewAt(i).compare(needle, cs) == 0)
                return true;
        }
        return false;
    }

private:
    StaticString m_string;
    OffsetList m_offsets;
    friend tst_QOffsetStringArray;
};

namespace QtPrivate {
template <size_t Highest> constexpr auto minifyValue()
{
    constexpr size_t max8 = (std::numeric_limits<quint8>::max)();
    constexpr size_t max16 = (std::numeric_limits<quint16>::max)();
    if constexpr (Highest <= max8) {
        return quint8(Highest);
    } else if constexpr (Highest <= max16) {
        return quint16(Highest);
    } else {
        // int is probably enough for everyone
        return int(Highest);
    }
}

template <typename Char, int... Nx>
constexpr auto makeOffsetStringArray(const Char (&...entries)[Nx])
{
    constexpr size_t StringLength = (Nx + ...);
    using OffsetType = decltype(QtPrivate::minifyValue<StringLength>());

    // prepend the first offset (zero) pointing to the *start* of the first element
    size_t offset = 0;
    std::array offsetList = {
        OffsetType(0),
        OffsetType(offset += Nx)...
    };

    // append an extra null terminator
    std::array<Char, StringLength + 1> staticString = {};
    const Char *strings[] = { entries... };
    for (size_t i = 0; i < std::size(strings); ++i) {
        size_t length = offsetList[i + 1] - offsetList[i];
        q20::copy_n(strings[i], length, staticString.begin() + offsetList[i]);
    }

    return QOffsetStringArray(staticString, offsetList);
}
} // namespace QtPrivate

template<typename Char, int ... Nx>
#ifdef __cpp_concepts
requires std::is_same_v<Char, char> || std::is_same_v<Char, char16_t>
#  ifdef __cpp_char8_t
    || std::is_same_v<Char, char8_t>
#  endif
#endif
constexpr auto qOffsetStringArray(const Char (&...strings)[Nx]) noexcept
{
    return QtPrivate::makeOffsetStringArray<Char>(strings...);
}

QT_WARNING_POP
QT_END_NAMESPACE

#endif // QOFFSETSTRINGARRAY_P_H
