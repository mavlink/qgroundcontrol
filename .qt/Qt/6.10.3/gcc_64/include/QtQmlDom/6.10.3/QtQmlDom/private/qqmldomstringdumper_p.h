// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef DUMPER_H
#define DUMPER_H

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

#include "qqmldom_global.h"
#include "qqmldomconstants_p.h"
#include "qqmldomfunctionref_p.h"

#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QDebug>

#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

using Sink = function_ref<void(QStringView)>;
using SinkF = std::function<void(QStringView)>;
using DumperFunction = std::function<void(const Sink &)>;

class Dumper{
public:
    DumperFunction dumper;
private:
    // We want to avoid the limit of one user conversion:
    // after doing (* -> QStringView) we cannot have QStringView -> Dumper, as it
    // would be the second user defined conversion.
    // For a similar reason we have a template to accept function_ref<void(Sink)> .
    // The end result is that void f(Dumper) can be called nicely, and avoid overloads:
    // f(u"bla"), f(QLatin1String("bla")), f(QString()), f([](const Sink &s){...}),...
    template <typename T>
    using if_compatible_dumper = typename
    std::enable_if<std::is_convertible<T, DumperFunction>::value, bool>::type;

    template<typename T>
    using if_string_view_convertible = typename
    std::enable_if<std::is_convertible_v<T, QStringView>, bool>::type;

public:
    Dumper(QStringView s):
        dumper([s](const Sink &sink){ sink(s); }) {}

    Dumper(std::nullptr_t): Dumper(QStringView(nullptr)) {}

    template <typename Stringy, if_string_view_convertible<Stringy> = true>
    Dumper(Stringy string):
        Dumper(QStringView(string)) {}

    template <typename U, if_compatible_dumper<U> = true>
    Dumper(U f): dumper(std::move(f)) {}

    void operator()(const Sink &s) const { dumper(s); }
};

template <typename T>
void sinkInt(const Sink &s, T i) {
    const int BUFSIZE = 42; // safe up to 128 bits
    QChar buf[BUFSIZE];
    int ibuf = BUFSIZE;
    buf[--ibuf] = QChar(0);
    bool neg = false;
    if (i < 0)
        neg=true;
    int digit = i % 10;
    i = i / 10;
    if constexpr (std::is_signed_v<T>) {
        if (neg) { // we change the sign here because -numeric_limits<T>::min() == numeric_limits<T>::min()
            i = -i;
            digit = - digit;
        }
    }
    buf[--ibuf] = QChar::fromLatin1('0' + digit);
    while (i > 0 && ibuf > 0) {
        digit = i % 10;
        buf[--ibuf] = QChar::fromLatin1('0' + digit);
        i = i / 10;
    }
    if (neg && ibuf > 0)
        buf[--ibuf] = QChar::fromLatin1('-');
    s(QStringView(&buf[ibuf], BUFSIZE - ibuf -1));
}

QMLDOM_EXPORT QString dumperToString(const Dumper &writer);

QMLDOM_EXPORT void sinkEscaped(const Sink &sink, QStringView s,
                               EscapeOptions options = EscapeOptions::OuterQuotes);

inline void devNull(QStringView) {}

QMLDOM_EXPORT void sinkIndent(const Sink &s, int indent);

QMLDOM_EXPORT void sinkNewline(const Sink &s, int indent = 0);

QMLDOM_EXPORT void dumpErrorLevel(const Sink &s, ErrorLevel level);

QMLDOM_EXPORT void dumperToQDebug(const Dumper &dumper, QDebug debug);

QMLDOM_EXPORT void dumperToQDebug(const Dumper &dumper, ErrorLevel level = ErrorLevel::Debug);

QMLDOM_EXPORT QDebug operator<<(QDebug d, const Dumper &dumper);

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE

#endif // DUMPER_H
