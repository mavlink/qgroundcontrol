// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QV4QMLCONTEXT_P_H
#define QV4QMLCONTEXT_P_H

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

#include <private/qqmlcontextdata_p.h>
#include <private/qtqmlglobal_p.h>
#include <private/qv4context_p.h>
#include <private/qv4object_p.h>

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct QQmlContextWrapper;

namespace Heap {

#define QQmlContextWrapperMembers(class, Member) \
    Member(class, Pointer, Module *, module)

DECLARE_HEAP_OBJECT(QQmlContextWrapper, Object) {
    DECLARE_MARKOBJECTS(QQmlContextWrapper)

    void init(QQmlRefPointer<QQmlContextData> context, QObject *scopeObject);
    void destroy();

    // This has to be a plain pointer because object needs to be a POD type.
    QQmlContextData *context;
    QV4QPointer<QObject> scopeObject;
};

#define QmlContextMembers(class, Member)

DECLARE_HEAP_OBJECT(QmlContext, ExecutionContext) {
    DECLARE_MARKOBJECTS(QmlContext)

    QQmlContextWrapper *qml() { return static_cast<QQmlContextWrapper *>(activation.get()); }
    void init(QV4::ExecutionContext *outerContext, QV4::QQmlContextWrapper *qml);
};

}

struct Q_QML_EXPORT QQmlContextWrapper : Object
{
    V4_OBJECT2(QQmlContextWrapper, Object)
    V4_NEEDS_DESTROY
    V4_INTERNALCLASS(QmlContextWrapper)

    inline QObject *getScopeObject() const { return d()->scopeObject; }
    inline QQmlRefPointer<QQmlContextData> getContext() const { return d()->context; }

    static ReturnedValue getPropertyAndBase(const QQmlContextWrapper *resource, PropertyKey id, const Value *receiver,
                                            bool *hasProperty, Value *base, Lookup *lookup = nullptr);
    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);

    static ReturnedValue resolveQmlContextPropertyLookupGetter(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupScript(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupSingleton(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupValueSingleton(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupIdObject(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupIdObjectInParentContext(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupScopeObjectProperty(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupScopeObjectMethod(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupScopeFallbackProperty(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupContextObjectProperty(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupContextObjectMethod(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupInGlobalObject(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupInParentContextHierarchy(Lookup *l, ExecutionEngine *engine, Value *base);
    static ReturnedValue lookupType(Lookup *l, ExecutionEngine *engine, Value *base);
};

struct Q_QML_EXPORT QmlContext : public ExecutionContext
{
    V4_MANAGED(QmlContext, ExecutionContext)
    V4_INTERNALCLASS(QmlContext)

    static Heap::QmlContext *create(
            QV4::ExecutionContext *parent, QQmlRefPointer<QQmlContextData> context,
            QObject *scopeObject);

    QObject *qmlScope() const {
        return d()->qml()->scopeObject;
    }

    QQmlRefPointer<QQmlContextData> qmlContext() const {
        return d()->qml()->context;
    }
};

}

QT_END_NAMESPACE

#endif

