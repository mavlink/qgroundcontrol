// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QJSVALUEITERATOR_P_H
#define QJSVALUEITERATOR_P_H

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

#include "qjsvalue.h"
#include "private/qv4objectiterator_p.h"

QT_BEGIN_NAMESPACE

class QJSValueIteratorPrivate
{
public:
    QJSValueIteratorPrivate(const QJSValue &v);

    void init(const QJSValue &v);
    bool isValid() const;

    void next();

    QV4::ExecutionEngine *engine = nullptr;
    QV4::PersistentValue object;
    QScopedPointer<QV4::OwnPropertyKeyIterator> iterator;
    QV4::PersistentValue currentKey;
    QV4::PersistentValue nextKey;
};


QT_END_NAMESPACE

#endif // QJSVALUEITERATOR_P_H
