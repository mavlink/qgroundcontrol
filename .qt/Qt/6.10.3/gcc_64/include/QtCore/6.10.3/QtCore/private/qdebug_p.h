// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parsing

#ifndef QDEBUG_P_H
#define QDEBUG_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include "QtCore/qdebug.h"
#include "QtCore/qmetaobject.h"
#include "QtCore/qflags.h"
#include "QtCore/qbytearray.h"

QT_BEGIN_NAMESPACE

class QRect;

namespace QtDebugUtils {

Q_CORE_EXPORT QByteArray toPrintable(const char *data, qint64 len, qsizetype maxSize);

// inline helpers for formatting basic classes.

template <class Point>
static inline void formatQPoint(QDebug &debug, const Point &point)
{
    debug << point.x() << ',' << point.y();
}

template <class Size>
static inline void formatQSize(QDebug &debug, const Size &size)
{
    debug << size.width() << ", " << size.height();
}

template <class Rect>
static inline void formatQRect(QDebug &debug, const Rect &rect)
{
    debug << rect.x() << ',' << rect.y() << ' ';
    if constexpr (std::is_same_v<Rect, QRect>) {
        // QRect may overflow. Calculate width and height in higher precision.
        const qint64 w = qint64(rect.right()) - rect.left() + 1;
        const qint64 h = qint64(rect.bottom()) - rect.top() + 1;
        debug << w << 'x' << h;

        constexpr qint64 M = (std::numeric_limits<int>::max)();
        if (w > M || h > M)
            debug << " (oversized)";
    } else {
        debug << rect.width() << 'x' << rect.height();
    }
}

template <class Margins>
static inline void formatQMargins(QDebug &debug, const Margins &margins)
{
    debug << margins.left() << ", " << margins.top() << ", " << margins.right()
        << ", " << margins.bottom();
}

#ifndef QT_NO_QOBJECT
template <class QEnum>
static inline void formatQEnum(QDebug &debug, QEnum value)
{
    const QMetaObject *metaObject = qt_getEnumMetaObject(value);
    const QMetaEnum me = metaObject->enumerator(metaObject->indexOfEnumerator(qt_getEnumName(value)));
    if (const char *key = me.valueToKey(int(value)))
        debug << key;
    else
        debug << int(value);
}

template <class QEnum>
static inline void formatNonNullQEnum(QDebug &debug, const char *prefix, QEnum value)
{
    if (value) {
         debug << prefix;
         formatQEnum(debug, value);
    }
}

template <class Enum>
static inline void formatQFlags(QDebug &debug, const QFlags<Enum> &value)
{
    const QMetaEnum me = QMetaEnum::fromType<QFlags<Enum>>();
    const QDebugStateSaver saver(debug);
    debug.noquote();
    debug << me.valueToKeys(value.toInt());
}

template <class Enum>
static inline void formatNonNullQFlags(QDebug &debug, const char *prefix, const QFlags<Enum> &value)
{
    if (value) {
        debug << prefix;
        formatQFlags(debug, value);
    }
}

#endif // !QT_NO_QOBJECT

} // namespace QtDebugUtils

QT_END_NAMESPACE

#endif // QDEBUG_P_H
