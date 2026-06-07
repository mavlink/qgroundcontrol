// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QANYSTRINGVIEWUTILS_P_H
#define QANYSTRINGVIEWUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/private/qjson_p.h>

#include <QtCore/qanystringview.h>

QT_BEGIN_NAMESPACE

namespace QAnyStringViewUtils {

// Note: This only works if part is US-ASCII, but there is no type to encode this information!
inline bool endsWith(QAnyStringView whole, QLatin1StringView part)
{
    Q_ASSERT(QtPrivate::isAscii(part));
    return whole.length() >= part.length() && whole.last(part.length()) == part;
}

// Note: This only works if part is US-ASCII, but there is no type to encode this information!
inline bool startsWith(QAnyStringView whole, QLatin1StringView part)
{
    Q_ASSERT(QtPrivate::isAscii(part));
    return whole.length() >= part.length() && whole.first(part.length()) == part;
}

inline bool doesContain(QStringView whole, QLatin1Char part) { return whole.contains(part); }
inline bool doesContain(QLatin1StringView whole, QLatin1Char part) { return whole.contains(part); }
inline bool doesContain(QUtf8StringView whole, QLatin1Char part)
{
    return QByteArrayView(whole.data(), whole.size()).contains(part.toLatin1());
}
inline bool contains(QAnyStringView whole, QLatin1Char part)
{
    return whole.visit([&](auto view) { return doesContain(view, part); });
}

inline qsizetype getLastIndexOf(QStringView whole, QLatin1StringView part)
{
    return whole.lastIndexOf(part);
}
inline qsizetype getLastIndexOf(QLatin1StringView whole, QLatin1StringView part)
{
    return whole.lastIndexOf(part);
}
inline qsizetype getLastIndexOf(QUtf8StringView whole, QLatin1StringView part)
{
    return QByteArrayView(whole.data(), whole.size()).lastIndexOf(part);
}
inline qsizetype lastIndexOf(QAnyStringView whole, QLatin1StringView part)
{
    Q_ASSERT(QtPrivate::isAscii(part));
    return whole.visit([&](auto view) { return getLastIndexOf(view, part); });
}

inline int toInt(QUtf8StringView view)
{
    return QByteArrayView(view.data(), view.length()).toInt();
}
inline int toInt(QLatin1StringView view) { return view.toInt(); }
inline int toInt(QStringView view) { return view.toInt(); }

inline int toInt(QAnyStringView string)
{
    return string.visit([](auto view) { return toInt(view); });
}

template<typename StringView>
QAnyStringView doTrimmed(StringView string)
{
    if constexpr (std::is_same_v<StringView, QStringView>)
        return string.trimmed();
    if constexpr (std::is_same_v<StringView, QLatin1StringView>)
        return string.trimmed();
    if constexpr (std::is_same_v<StringView, QUtf8StringView>)
        return QByteArrayView(string.data(), string.length()).trimmed();
}


inline QAnyStringView trimmed(QAnyStringView string)
{
    return string.visit([](auto data) {
        return doTrimmed(data);
    });
}

template<typename StringView, typename Handler>
auto processAsUtf8(StringView string, Handler &&handler)
{
    if constexpr (std::is_same_v<StringView, QStringView>)
        return handler(QByteArrayView(string.toUtf8()));
    if constexpr (std::is_same_v<StringView, QLatin1StringView>)
        return handler(QByteArrayView(string.data(), string.length()));
    if constexpr (std::is_same_v<StringView, QUtf8StringView>)
        return handler(QByteArrayView(string.data(), string.length()));
    if constexpr (std::is_same_v<StringView, QByteArrayView>)
        return handler(string);
    if constexpr (std::is_same_v<StringView, QByteArray>)
        return handler(QByteArrayView(string));
    if constexpr (std::is_same_v<StringView, QAnyStringView>) {

        // Handler is:
        // * a reference if an lvalue ref is passed
        // * a value otherwise
        // We conserve its nature for passing to the lambda below.
        // This is necessary because we need to decide on the nature of
        // the lambda capture as part of the syntax (prefix '&' or not).
        // So we always pass a reference-conserving wrapper as value.
        struct Wrapper { Handler handler; };

        return string.visit([w = Wrapper { std::forward<Handler>(handler) }](auto view) mutable {
            static_assert(!(std::is_same_v<decltype(view), QAnyStringView>));
            return processAsUtf8(std::move(view), std::forward<Handler>(w.handler));
        });
    }
    Q_UNREACHABLE();
}

// Note: This only works if sep is US-ASCII, but there is no type to encode this information!
inline QList<QAnyStringView> split(QAnyStringView source, QLatin1StringView sep)
{
    Q_ASSERT(QtPrivate::isAscii(sep));

    QList<QAnyStringView> list;
    if (source.isEmpty()) {
        list.append(source);
        return list;
    }

    qsizetype start = 0;
    qsizetype end = source.length();

    for (qsizetype current = 0; current < end; ++current) {
        if (source.mid(current, sep.length()) == sep) {
            list.append(source.mid(start, current - start));
            start = current + sep.length();
        }
    }

    if (start < end)
        list.append(source.mid(start, end - start));

    return list;
}

}

QT_END_NAMESPACE

#endif // QANYSTRINGVIEWUTILS_P_H
