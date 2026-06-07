// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLOBJECTORGADGET_P_H
#define QQMLOBJECTORGADGET_P_H

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

#include <private/qqmlmetaobject_p.h>
#include <private/qbipointer_p.h>

QT_BEGIN_NAMESPACE

class QQmlObjectOrGadget: public QQmlMetaObject
{
public:
    QQmlObjectOrGadget(QObject *obj)
        : QQmlMetaObject(obj),
        ptr(obj)
    {}
    QQmlObjectOrGadget(const QMetaObject *metaObject, void *gadget)
        : QQmlMetaObject(metaObject)
        , ptr(gadget)
    {}
    QQmlObjectOrGadget(const QMetaObject* metaObject)
        : QQmlMetaObject(metaObject)
    {}

    void metacall(QMetaObject::Call type, int index, void **argv) const;

    bool isNull() const { return ptr.isNull(); }
    QObject *qObject() const { return ptr.isT1() ? ptr.asT1() : nullptr; }

private:
    QBiPointer<QObject, void> ptr;
};

QT_END_NAMESPACE

#endif // QQMLOBJECTORGADGET_P_H
