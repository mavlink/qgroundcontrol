// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV8TYPEWRAPPER_P_H
#define QV8TYPEWRAPPER_P_H

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

#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4qmetaobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

class QQmlTypeNameCache;
class QQmlType;
class QQmlTypePrivate;
struct QQmlImportRef;

namespace QV4 {

namespace Heap {

struct QQmlTypeWrapper : FunctionObject {

    enum TypeNameMode : quint8  {
        ExcludeEnums     = 0x0,
        IncludeEnums     = 0x1,
        TypeNameModeMask = 0x1,
    };

    enum Kind : quint8 {
        Type      = 0x0,
        Namespace = 0x2,
        KindMask  = 0x2
    };

    void init(TypeNameMode m, QObject *o, const QQmlTypePrivate *type);
    void init(TypeNameMode m, QObject *o, QQmlTypeNameCache *type, const QQmlImportRef *import);

    void destroy();

    const QMetaObject *metaObject() const { return type().metaObject(); }
    QMetaType metaType() const { return type().typeId(); }

    QQmlType type() const;
    TypeNameMode typeNameMode() const { return TypeNameMode(flags & TypeNameModeMask); }
    Kind kind() const { return Kind(flags & KindMask); }

    const QQmlPropertyData *ensureConstructorsCache(
            const QMetaObject *metaObject, QMetaType metaType)
    {
        Q_ASSERT(kind() == Type);
        if (!t.constructors && metaObject) {
            t.constructors = QMetaObjectWrapper::createConstructors(metaObject, metaType);
            warnIfUncreatable();
        }
        return t.constructors;
    }
    void warnIfUncreatable() const;

    QQmlTypeNameCache::Result queryNamespace(
            const QV4::String *name, QQmlEnginePrivate *enginePrivate) const;

    QV4QPointer<QObject> object;

    union {
        struct {
            const QQmlTypePrivate *typePrivate;
            const QQmlPropertyData *constructors;
        } t;
        struct {
            QQmlTypeNameCache *typeNamespace;
            const QQmlImportRef *importNamespace;
        } n;
    };

    quint8 flags;
};

using QQmlTypeConstructor = QQmlTypeWrapper;

struct QQmlEnumWrapper : Object
{
    void init() { Object::init(); }
    void destroy();
    QQmlType type() const;

    const QQmlTypePrivate *typePrivate;
    int enumIndex;
    bool scoped;
};

}

struct Q_QML_EXPORT QQmlTypeWrapper : FunctionObject
{
    V4_OBJECT2(QQmlTypeWrapper, FunctionObject)
    V4_PROTOTYPE(typeWrapperPrototype)
    Q_MANAGED_TYPE(QMLTypeWrapper);
    V4_NEEDS_DESTROY

    bool isSingleton() const;
    const QMetaObject *metaObject() const;
    QObject *object() const;
    QObject *singletonObject() const;

    QVariant toVariant() const;

    static void initProto(ExecutionEngine *v4);

    static ReturnedValue create(ExecutionEngine *, QObject *, const QQmlType &,
                                Heap::QQmlTypeWrapper::TypeNameMode = Heap::QQmlTypeWrapper::IncludeEnums);
    static ReturnedValue create(ExecutionEngine *, QObject *, const QQmlRefPointer<QQmlTypeNameCache> &, const QQmlImportRef *,
                                Heap::QQmlTypeWrapper::TypeNameMode = Heap::QQmlTypeWrapper::IncludeEnums);

    static ReturnedValue virtualResolveLookupGetter(const Object *object, ExecutionEngine *engine, Lookup *lookup);
    static bool virtualResolveLookupSetter(Object *object, ExecutionEngine *engine, Lookup *lookup, const Value &value);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);
    static int virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a);

    static ReturnedValue lookupSingletonProperty(Lookup *l, ExecutionEngine *engine, const Value &base);
    static ReturnedValue lookupSingletonMethod(Lookup *l, ExecutionEngine *engine, const Value &base);
    static ReturnedValue lookupEnumValue(Lookup *l, ExecutionEngine *engine, const Value &base);
    static ReturnedValue lookupEnum(Lookup *l, ExecutionEngine *engine, const Value &base);

protected:
    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
    static bool virtualIsEqualTo(Managed *that, Managed *o);
    static ReturnedValue virtualInstanceOf(const Object *typeObject, const Value &var);

private:
    static ReturnedValue method_hasInstance(
            const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toString(
            const FunctionObject *b, const Value *thisObject, const Value *, int);
};

struct QQmlTypeConstructor : QQmlTypeWrapper
{
    V4_OBJECT2(QQmlTypeConstructor, QQmlTypeWrapper)

    static ReturnedValue virtualCallAsConstructor(
            const FunctionObject *f, const Value *argv, int argc, const Value *)
    {
        Q_ASSERT(f->as<QQmlTypeWrapper>());
        return QMetaObjectWrapper::construct(
                static_cast<const QQmlTypeWrapper *>(f)->d(), argv, argc);
    }
};

struct Q_QML_EXPORT QQmlEnumWrapper : Object
{
    V4_OBJECT2(QQmlEnumWrapper, Object)
    V4_NEEDS_DESTROY

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
};

}

QT_END_NAMESPACE

#endif // QV8TYPEWRAPPER_P_H

