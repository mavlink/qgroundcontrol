// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QTCORE_QTCONTENTTYPEPARSER_P_H
#define QTCORE_QTCONTENTTYPEPARSER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearrayview.h>
#include <QtCore/qlatin1stringview.h>

#include <QtCore/qxpfunctional.h>
#include <string>

QT_BEGIN_NAMESPACE

namespace QtContentTypeParser {

constexpr auto parse_OWS(QByteArrayView data) noexcept
{
    struct R {
        QByteArrayView ows, tail;
    };

    constexpr auto is_OWS_char = [](auto ch) { return ch == ' ' || ch == '\t'; };

    qsizetype i = 0;
    while (i < data.size() && is_OWS_char(data[i]))
        ++i;

    return R{data.first(i), data.sliced(i)};
}

constexpr void eat_OWS(QByteArrayView &data) noexcept
{
    data = parse_OWS(data).tail;
}

constexpr auto parse_quoted_string(QByteArrayView data, qxp::function_ref<void(char) const> yield)
{
    struct R {
        QByteArrayView quotedString, tail;
        constexpr explicit operator bool() const noexcept { return !quotedString.isEmpty(); }
    };

    if (!data.startsWith('"'))
        return R{{}, data};

    qsizetype i = 1; // one past initial DQUOTE
    while (i < data.size()) {
        switch (auto ch = data[i++]) {
        case '"':  // final DQUOTE -> end of string
            return R{data.first(i), data.sliced(i)};
        case '\\': // quoted-pair
            // https://www.rfc-editor.org/rfc/rfc9110.html#section-5.6.4-3:
            //   Recipients that process the value of a quoted-string MUST handle a
            //   quoted-pair as if it were replaced by the octet following the backslash.
            if (i == data.size())
                break; // premature end
            ch = data[i++]; // eat '\\'
            [[fallthrough]];
        default:
            // we don't validate quoted-string octets to be only qdtext (Postel's Law)
            yield(ch);
        }
    }

    return R{{}, data}; // premature end
}

constexpr bool is_tchar(char ch) noexcept
{
    // ### optimize
    switch (ch) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
        return true;
    default:
        return (ch >= 'a' && ch <= 'z')
            || (ch >= '0' && ch <= '9')
            || (ch >= 'A' && ch <= 'Z');
    }
}

constexpr auto parse_comment(QByteArrayView data) noexcept
{
    struct R {
        QByteArrayView comment, tail;
        constexpr explicit operator bool() const noexcept { return !comment.isEmpty(); }
    };

    const auto invalid = R{{}, data}; // preserves original `data`

    // comment        = "(" *( ctext / quoted-pair / comment ) ")"
    // ctext          = HTAB / SP / %x21-27 / %x2A-5B / %x5D-7E / obs-text

    if (!data.startsWith('('))
        return invalid;

    qsizetype i = 1;
    qsizetype level = 1;
    while (i < data.size()) {
        switch (data[i++]) {
        case '(': // nested comment
            ++level;
            break;
        case ')': // end of comment
            if (--level == 0)
                return R{data.first(i), data.sliced(i)};
            break;
        case '\\': // quoted-pair
            if (i == data.size())
                return invalid; // premature end
            ++i; // eat escaped character
            break;
        default:
            ; // don't validate ctext - accept everything (Postel's Law)
        }
    }

    return invalid; // premature end / unbalanced nesting levels
}

constexpr void eat_CWS(QByteArrayView &data) noexcept
{
    eat_OWS(data);
    while (const auto comment = parse_comment(data)) {
        data = comment.tail;
        eat_OWS(data);
    }
}

constexpr auto parse_token(QByteArrayView data) noexcept
{
    struct R {
        QByteArrayView token, tail;
        constexpr explicit operator bool() const noexcept { return !token.isEmpty(); }
    };

    qsizetype i = 0;
    while (i < data.size() && is_tchar(data[i]))
        ++i;

    return R{data.first(i), data.sliced(i)};
}

constexpr auto parse_parameter(QByteArrayView data, qxp::function_ref<void(char) const> yield)
{
    struct R {
        QLatin1StringView name; QByteArrayView value; QByteArrayView tail;
        constexpr explicit operator bool() const noexcept { return !name.isEmpty(); }
    };

    const auto invalid = R{{}, {}, data}; // preserves original `data`

    // parameter       = parameter-name "=" parameter-value
    // parameter-name  = token
    // parameter-value = ( token / quoted-string )

    const auto name = parse_token(data);
    if (!name)
        return invalid;
    data = name.tail;

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    if (!data.startsWith('='))
        return invalid;
    data = data.sliced(1);

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    if (Q_UNLIKELY(data.startsWith('"'))) { // value is a quoted-string

        const auto value = parse_quoted_string(data, yield);
        if (!value)
            return invalid;
        data = value.tail;

        return R{QLatin1StringView{name.token}, value.quotedString, data};

    } else { // value is a token

        const auto value = parse_token(data);
        if (!value)
            return invalid;
        data = value.tail;

        return R{QLatin1StringView{name.token}, value.token, data};
    }
}

inline auto parse_content_type(QByteArrayView data)
{
    using namespace Qt::StringLiterals;

    struct R {
        QLatin1StringView type, subtype;
        std::string charset;
        constexpr explicit operator bool() const noexcept { return !type.isEmpty(); }
    };

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    const auto type = parse_token(data);
    if (!type)
        return R{};
    data = type.tail;

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    if (!data.startsWith('/'))
        return R{};
    data = data.sliced(1);

    eat_CWS(data); // not in the grammar, but accepted under Postel's Law

    const auto subtype = parse_token(data);
    if (!subtype)
        return R{};
    data = subtype.tail;

    eat_CWS(data);

    auto r = R{QLatin1StringView{type.token}, QLatin1StringView{subtype.token}, {}};

    while (data.startsWith(';')) {

        data = data.sliced(1); // eat ';'

        eat_CWS(data);

        const auto param = parse_parameter(data, [&](char ch) { r.charset.append(1, ch); });
        if (param.name.compare("charset"_L1, Qt::CaseInsensitive) == 0) {
            if (r.charset.empty() && !param.value.startsWith('"')) // wasn't a quoted-string
                r.charset.assign(param.value.begin(), param.value.end());
            return r; // charset found
        }
        r.charset.clear(); // wasn't an actual charset
        if (param.tail.size() == data.size()) // no progress was made
            break; // returns {type, subtype}
        // otherwise, continue (accepting e.g. `;;`)
        data = param.tail;

        eat_CWS(data);
    }

    return r; // no charset found
}

} // namespace QtContentTypeParser

QT_END_NAMESPACE

#endif // QTCORE_QTCONTENTTYPEPARSER_P_H
