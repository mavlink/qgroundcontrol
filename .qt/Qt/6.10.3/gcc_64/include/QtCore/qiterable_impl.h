// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QITERABLE_IMPL_H
#define QITERABLE_IMPL_H

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

namespace QIterablePrivate {

template<typename Callback>
static QVariant retrieveElement(QMetaType type, Callback callback)
{
    QVariant v(type);
    void *dataPtr;
    if (type == QMetaType::fromType<QVariant>())
        dataPtr = &v;
    else
        dataPtr = v.data();
    callback(dataPtr);
    return v;
}

} // namespace QIterablePrivate

QT_END_NAMESPACE

#endif // QITERABLE_IMPL_H
