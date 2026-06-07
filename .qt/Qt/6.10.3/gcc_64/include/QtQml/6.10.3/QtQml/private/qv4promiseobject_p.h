// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4PROMISEOBJECT_H
#define QV4PROMISEOBJECT_H

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

#include "qv4object_p.h"
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct PromiseCapability;

namespace Promise {

struct ReactionEvent;
struct ResolveThenableEvent;

class ReactionHandler : public QObject
{
    Q_OBJECT

public:
    ReactionHandler(QObject *parent = nullptr);
    ~ReactionHandler() override;

    void addReaction(ExecutionEngine *e, const Value *reaction, const Value *value);
    void addResolveThenable(ExecutionEngine *e, const PromiseObject *promise, const Object *thenable, const FunctionObject *then);

protected:
    void customEvent(QEvent *event) override;
    void executeReaction(ReactionEvent *event);
    void executeResolveThenable(ResolveThenableEvent *event);
};

} // Promise

namespace Heap {

struct PromiseCtor : FunctionObject {
    void init(ExecutionEngine *engine);
};

#define PromiseObjectMembers(class, Member) \
    Member(class, HeapValue, HeapValue, resolution) \
    Member(class, HeapValue, HeapValue, fulfillReactions) \
    Member(class, HeapValue, HeapValue, rejectReactions)

DECLARE_HEAP_OBJECT(PromiseObject, Object) {
    DECLARE_MARKOBJECTS(PromiseObject)
    void init(ExecutionEngine *e);

    enum State {
        Pending,
        Fulfilled,
        Rejected
    };

    void setState(State);
    bool isSettled() const;
    bool isPending() const;
    bool isFulfilled() const;
    bool isRejected() const;

    State state;

    void triggerFullfillReactions(ExecutionEngine *e);
    void triggerRejectReactions(ExecutionEngine *e);
};

#define PromiseCapabilityMembers(class, Member) \
    Member(class, HeapValue, HeapValue, promise) \
    Member(class, HeapValue, HeapValue, resolve) \
    Member(class, HeapValue, HeapValue, reject)

DECLARE_HEAP_OBJECT(PromiseCapability, Object) {
    DECLARE_MARKOBJECTS(PromiseCapability)
};

#define PromiseReactionMembers(class, Member) \
    Member(class, HeapValue, HeapValue, handler) \
    Member(class, Pointer, PromiseCapability*, capability)

DECLARE_HEAP_OBJECT(PromiseReaction, Object) {
    DECLARE_MARKOBJECTS(PromiseReaction)

    static Heap::PromiseReaction *createFulfillReaction(ExecutionEngine* e, const QV4::PromiseCapability *capability, const QV4::FunctionObject *onFulfilled);
    static Heap::PromiseReaction *createRejectReaction(ExecutionEngine* e, const QV4::PromiseCapability *capability, const QV4::FunctionObject *onRejected);

    void triggerWithValue(ExecutionEngine *e, const Value *value);

    enum Type {
        Function,
        Identity,
        Thrower
    };

    Type type;

    friend class ReactionHandler;
};

#define CapabilitiesExecutorWrapperMembers(class, Member) \
    Member(class, Pointer, PromiseCapability*, capabilities)

DECLARE_HEAP_OBJECT(CapabilitiesExecutorWrapper, FunctionObject) {
    DECLARE_MARKOBJECTS(CapabilitiesExecutorWrapper)
    void init();
    void destroy();
};

#define PromiseExecutionStateMembers(class, Member) \
    Member(class, HeapValue, HeapValue, values) \
    Member(class, HeapValue, HeapValue, capability)

DECLARE_HEAP_OBJECT(PromiseExecutionState, FunctionObject) {
    DECLARE_MARKOBJECTS(PromiseExecutionState)
    void init();

    uint index;
    uint remainingElementCount;
};

#define ResolveElementWrapperMembers(class, Member) \
    Member(class, HeapValue, HeapValue, state)

DECLARE_HEAP_OBJECT(ResolveElementWrapper, FunctionObject) {
    DECLARE_MARKOBJECTS(ResolveElementWrapper)
    void init();

    uint index;
    bool alreadyResolved;
};

#define ResolveWrapperMembers(class, Member) \
    Member(class, Pointer, PromiseObject*, promise)

DECLARE_HEAP_OBJECT(ResolveWrapper, FunctionObject) {
    DECLARE_MARKOBJECTS(ResolveWrapper)
    void init();

    bool alreadyResolved;
};

#define RejectWrapperMembers(class, Member) \
    Member(class, Pointer, PromiseObject*, promise)

DECLARE_HEAP_OBJECT(RejectWrapper, FunctionObject) {
    DECLARE_MARKOBJECTS(RejectWrapper)
    void init();

    bool alreadyResolved;
};

} // Heap

struct PromiseReaction : Object
{
    V4_OBJECT2(PromiseReaction, Object)
};

struct PromiseCapability : Object
{
    V4_OBJECT2(PromiseCapability, Object)
};

struct PromiseExecutionState : Object
{
    V4_OBJECT2(PromiseExecutionState, Object)
};

struct Q_QML_EXPORT PromiseObject : Object
{
    V4_OBJECT2(PromiseObject, Object)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(promisePrototype)
};

struct PromiseCtor: FunctionObject
{
    V4_OBJECT2(PromiseCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_resolve(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_reject(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue method_all(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_race(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct PromisePrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_then(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_catch(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct CapabilitiesExecutorWrapper: FunctionObject {
    V4_OBJECT2(CapabilitiesExecutorWrapper, FunctionObject)

    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct ResolveElementWrapper : FunctionObject {
    V4_OBJECT2(ResolveElementWrapper, FunctionObject)

    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct ResolveWrapper : FunctionObject {
    V4_OBJECT2(ResolveWrapper, FunctionObject)

    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct RejectWrapper : FunctionObject {
    V4_OBJECT2(RejectWrapper, FunctionObject)

    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

} // QV4

QT_END_NAMESPACE

#endif // QV4PROMISEOBJECT_H
