// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qpoint.h>
#include <QtCore/qstring.h>
#include <QtGui/qpolygon.h>
#include <QtCore/qstringbuilder.h>

#ifndef QHEXSTRING_P_H
#define QHEXSTRING_P_H

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

QT_BEGIN_NAMESPACE

// internal helper. Converts an integer value to a unique string token
template <typename T>
        struct HexString
{
    inline HexString(const T t)
        : val(t)
    {}

    inline void write(QChar *&dest) const
    {
        const char16_t hexChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
        const char *c = reinterpret_cast<const char *>(&val);
        for (uint i = 0; i < sizeof(T); ++i) {
            *dest++ = hexChars[*c & 0xf];
            *dest++ = hexChars[(*c & 0xf0) >> 4];
            ++c;
        }
    }
    const T val;
};

// specialization to enable fast concatenating of our string tokens to a string
template <typename T>
        struct QConcatenable<HexString<T> >
{
    typedef HexString<T> type;
    enum { ExactSize = true };
    static int size(const HexString<T> &) { return sizeof(T) * 2; }
    static inline void appendTo(const HexString<T> &str, QChar *&out) { str.write(out); }
    typedef QString ConvertTo;
};

QT_END_NAMESPACE

#endif // QHEXSTRING_P_H
