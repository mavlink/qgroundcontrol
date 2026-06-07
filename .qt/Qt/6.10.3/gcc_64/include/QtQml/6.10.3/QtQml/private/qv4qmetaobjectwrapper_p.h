// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4QMETAOBJECTWRAPPER_P_H
#define QV4QMETAOBJECTWRAPPER_P_H

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

#include <private/qv4functionobject_p.h>
#include <private/qv4value_p.h>

#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyData;

namespace QV4 {
namespace Heap {

struct QMetaObjectWrapper : FunctionObject
{
    void init(const QMetaObject *metaObject);
    void destroy();

    const QMetaObject *metaObject() const { return m_metaObject; }
    QMetaType metaType() const
    {
        const QMetaType type = m_metaObject->metaType();
        if (type.flags() & QMetaType::IsGadget)
            return type;

        // QObject* is our best guess because we can't get from a metatype to
        // the metatype of its pointer.
        return QMetaType::fromType<QObject *>();
    }

    const QQmlPropertyData *ensureConstructorsCache(
            const QMetaObject *metaObject, QMetaType metaType)
    {
        Q_ASSERT(metaObject);
        if (!m_constructors)
            m_constructors = createConstructors(metaObject, metaType);
        return m_constructors;
    }


    static const QQmlPropertyData *createConstructors(
            const QMetaObject *metaObject, QMetaType metaType)
    {
        Q_ASSERT(metaObject);
        const int count = metaObject->constructorCount();
        if (count == 0)
            return nullptr;

        QQmlPropertyData *constructors = new QQmlPropertyData[count];

        for (int i = 0; i < count; ++i) {
            QMetaMethod method = metaObject->constructor(i);
            QQmlPropertyData &d = constructors[i];
            d.load(method);
            d.setPropType(metaType);
            d.setCoreIndex(i);
        }

        return constructors;
    }

private:
    const QMetaObject *m_metaObject;
    const QQmlPropertyData *m_constructors;
};

} // namespace Heap

struct Q_QML_EXPORT QMetaObjectWrapper : public FunctionObject
{
    V4_OBJECT2(QMetaObjectWrapper, FunctionObject)
    V4_NEEDS_DESTROY

    static ReturnedValue create(ExecutionEngine *engine, const QMetaObject* metaObject);
    const QMetaObject *metaObject() const { return d()->metaObject(); }

    template<typename HeapObject>
    ReturnedValue static construct(HeapObject *d, const Value *argv, int argc)
    {
        const QMetaObject *mo = d->metaObject();
        return constructInternal(
                mo, d->ensureConstructorsCache(mo, d->metaType()), d, argv, argc);
    }

protected:
    static ReturnedValue virtualCallAsConstructor(
            const FunctionObject *, const Value *argv, int argc, const Value *);
    static bool virtualIsEqualTo(Managed *a, Managed *b);

private:
    void init(ExecutionEngine *engine);

    static ReturnedValue constructInternal(
            const QMetaObject *mo, const QQmlPropertyData *constructors, Heap::FunctionObject *d,
            const Value *argv, int argc);
};

} // namespace QV4

QT_END_NAMESPACE

#endif // QV4QMETAOBJECTWRAPPER_P_H


