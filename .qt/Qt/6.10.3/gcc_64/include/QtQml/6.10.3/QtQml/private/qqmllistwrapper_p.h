// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLLISTWRAPPER_P_H
#define QQMLLISTWRAPPER_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qpointer.h>

#include <QtQml/qqmllist.h>

#include <private/qv4value_p.h>
#include <private/qv4object_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcIncompatibleElement)

class QQmlProperty;

namespace QV4 {

namespace Heap {

struct QmlListWrapper : Object
{
    void init(QMetaType propertyType);
    void init(QObject *object, int propertyId, QMetaType propertyType);
    void init(QObject *object, const QQmlListProperty<QObject> &list, QMetaType propertyType);
    void destroy();

    QObject *object() const { return m_object.data(); }
    QMetaType propertyType() const { return QMetaType(m_propertyType); }
    QMetaType elementType() const { return QQmlMetaType::listValueType(propertyType()); }

    const QQmlListProperty<QObject> *property() const
    {
        return reinterpret_cast<const QQmlListProperty<QObject>*>(m_propertyData);
    }

    QQmlListProperty<QObject> *property()
    {
        return reinterpret_cast<QQmlListProperty<QObject>*>(m_propertyData);
    }

private:
    void *m_propertyData[sizeof(QQmlListProperty<QObject>)/sizeof(void*)];

    QV4QPointer<QObject> m_object;

    // interface instead of QMetaType to keep class a POD
    const QtPrivate::QMetaTypeInterface *m_propertyType;
};

}

struct Q_QML_EXPORT QmlListWrapper : Object
{
    V4_OBJECT2(QmlListWrapper, Object)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(propertyListPrototype)
    Q_MANAGED_TYPE(QmlListProperty)

    static ReturnedValue create(ExecutionEngine *engine, QObject *object, int propId, QMetaType propType);
    static ReturnedValue create(ExecutionEngine *engine, const QQmlListProperty<QObject> &prop, QMetaType propType);
    static ReturnedValue create(ExecutionEngine *engine, QMetaType propType);
    static ReturnedValue createOwned(ExecutionEngine *engine, const QQmlProperty &prop);

    QVariant toVariant() const;
    QQmlListReference toListReference() const;

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static qint64 virtualGetLength(const Managed *m);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
};

struct PropertyListPrototype : Object
{
    V4_PROTOTYPE(arrayPrototype)

    void init();

    static ReturnedValue method_pop(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_push(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_shift(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_splice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_unshift(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_indexOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_lastIndexOf(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sort(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_length(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_set_length(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif

