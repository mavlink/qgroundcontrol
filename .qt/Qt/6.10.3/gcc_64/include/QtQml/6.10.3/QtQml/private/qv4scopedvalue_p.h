// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QV4SCOPEDVALUE_P_H
#define QV4SCOPEDVALUE_P_H

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

#include "qv4engine_p.h"
#include "qv4value_p.h"
#include "qv4property_p.h"
#include "qv4propertykey_p.h"

#ifdef V4_USE_VALGRIND
#include <valgrind/memcheck.h>
#endif

QT_BEGIN_NAMESPACE

#define SAVE_JS_STACK(ctx) Value *__jsStack = ctx->engine->jsStackTop
#define CHECK_JS_STACK(ctx) Q_ASSERT(__jsStack == ctx->engine->jsStackTop)

namespace QV4 {

struct ScopedValue;

inline bool hasExceptionOrIsInterrupted(ExecutionEngine *engine)
{
    return engine->hasException || engine->isInterrupted.loadRelaxed();
}

#define CHECK_EXCEPTION() \
    do { \
        if (hasExceptionOrIsInterrupted(scope.engine)) { \
            return QV4::Encode::undefined(); \
        } \
    } while (false)

#define RETURN_UNDEFINED() \
    return QV4::Encode::undefined()

#define RETURN_RESULT(r) \
    return QV4::Encode(r)

#define THROW_TYPE_ERROR() \
    return scope.engine->throwTypeError()

#define THROW_GENERIC_ERROR(str) \
    return scope.engine->throwError(QString::fromUtf8(str))

struct Scope {
    explicit Scope(ExecutionContext *ctx)
        : engine(ctx->engine())
        , mark(engine->jsStackTop)
    {
    }

    explicit Scope(ExecutionEngine *e)
        : engine(e)
        , mark(engine->jsStackTop)
    {
    }

    explicit Scope(const Managed *m)
        : engine(m->engine())
        , mark(engine->jsStackTop)
    {
    }

    ~Scope() {
#ifndef QT_NO_DEBUG
        Q_ASSERT(engine->jsStackTop >= mark);
//        Q_ASSERT(engine->currentContext < mark);
        memset(mark, 0, (engine->jsStackTop - mark)*sizeof(Value));
#endif
#ifdef V4_USE_VALGRIND
        VALGRIND_MAKE_MEM_UNDEFINED(mark, (engine->jsStackLimit - mark) * sizeof(Value));
#endif
        engine->jsStackTop = mark;
    }

    enum AllocMode {
        Undefined,
        Empty,
        /* Be careful when using Uninitialized, the stack has to be fully initialized before calling into the memory manager again */
        Uninitialized
    };

    template <AllocMode mode = Undefined>
    Value *alloc(qint64 nValues) const = delete; // use safeForAllocLength

    template <AllocMode mode = Undefined>
    QML_NEARLY_ALWAYS_INLINE Value *alloc(int nValues) const
    {
        Value *ptr = engine->jsAlloca(nValues);
        switch (mode) {
        case Undefined:
            for (int i = 0; i < nValues; ++i)
                ptr[i] = Value::undefinedValue();
            break;
        case Empty:
            for (int i = 0; i < nValues; ++i)
                ptr[i] = Value::emptyValue();
            break;
        case Uninitialized:
            break;
        }
        return ptr;
    }
    template <AllocMode mode = Undefined>
    QML_NEARLY_ALWAYS_INLINE Value *alloc() const
    {
        Value *ptr = engine->jsAlloca(1);
        switch (mode) {
        case Undefined:
            *ptr = Value::undefinedValue();
            break;
        case Empty:
            *ptr = Value::emptyValue();
            break;
        case Uninitialized:
            break;
        }
        return ptr;
    }

    QML_NEARLY_ALWAYS_INLINE Value *construct(int nValues, const ReturnedValue& initialValue) const
    {
        Value *ptr = engine->jsAlloca(nValues);
        for (int i = 0; i < nValues; ++i)
            ptr[i].setRawValue(initialValue);
        return ptr;
    }

    QML_NEARLY_ALWAYS_INLINE Value *construct(int nValues, const Value &initialValue) const
    {
        Value *ptr = engine->jsAlloca(nValues);
        for (int i = 0; i < nValues; ++i)
            ptr[i] = initialValue;
        return ptr;
    }

    bool hasException() const {
        return engine->hasException;
    }

    ExecutionEngine *engine;
    Value *mark;

private:
    Q_DISABLE_COPY(Scope)
};

struct ScopedValue
{
    ScopedValue(const ScopedValue &) = default;
    ScopedValue(ScopedValue &&) = default;

    ScopedValue(const Scope &scope)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        ptr->setRawValue(0);
    }

    ScopedValue(const Scope &scope, const Value &v)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        *ptr = v;
    }

    ScopedValue(const Scope &scope, Heap::Base *o)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        ptr->setM(o);
    }

    ScopedValue(const Scope &scope, Managed *m)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        ptr->setRawValue(m->asReturnedValue());
    }

    ScopedValue(const Scope &scope, const ReturnedValue &v)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        ptr->setRawValue(v);
    }

    ScopedValue &operator=(const Value &v) {
        *ptr = v;
        return *this;
    }

    ScopedValue &operator=(Heap::Base *o) {
        ptr->setM(o);
        return *this;
    }

    ScopedValue &operator=(Managed *m) {
        *ptr = *m;
        return *this;
    }

    ScopedValue &operator=(const ReturnedValue &v) {
        ptr->setRawValue(v);
        return *this;
    }

    ScopedValue &operator=(const ScopedValue &other) {
        *ptr = *other.ptr;
        return *this;
    }

    Value *operator->() {
        return ptr;
    }

    const Value *operator->() const {
        return ptr;
    }

    operator Value *() { return ptr; }
    operator const Value &() const { return *ptr; }

    Value *ptr;
};


struct ScopedPropertyKey
{
    ScopedPropertyKey(const Scope &scope)
    {
        ptr = reinterpret_cast<PropertyKey *>(scope.alloc<Scope::Uninitialized>());
        *ptr = PropertyKey::invalid();
    }

    ScopedPropertyKey(const Scope &scope, const PropertyKey &v)
    {
        ptr = reinterpret_cast<PropertyKey *>(scope.alloc<Scope::Uninitialized>());
        *ptr = v;
    }

    ScopedPropertyKey &operator=(const PropertyKey &other) {
        *ptr = other;
        return *this;
    }

    PropertyKey *operator->() {
        return ptr;
    }
    operator PropertyKey() const {
        return *ptr;
    }

    bool operator==(const PropertyKey &other) const {
        return *ptr == other;
    }
    bool operator==(const ScopedPropertyKey &other) const {
        return *ptr == *other.ptr;
    }
    bool operator!=(const PropertyKey &other) const {
        return *ptr != other;
    }
    bool operator!=(const ScopedPropertyKey &other) const {
        return *ptr != *other.ptr;
    }

    PropertyKey *ptr;
};


template<typename T>
struct Scoped
{
    enum ConvertType { Convert };

    QML_NEARLY_ALWAYS_INLINE void setPointer(const Managed *p) {
        ptr->setM(p ? p->m() : nullptr);
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope)
    {
        ptr = scope.alloc<Scope::Undefined>();
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, const Value &v)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        setPointer(v.as<T>());
    }
    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, Heap::Base *o)
    {
        Value v;
        v = o;
        ptr = scope.alloc<Scope::Uninitialized>();
        setPointer(v.as<T>());
    }
    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, const ScopedValue &v)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        setPointer(v.ptr->as<T>());
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, const Value &v, ConvertType)
    {
        ptr = scope.construct(1, value_convert<T>(scope.engine, v));
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, const Value *v)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        setPointer(v ? v->as<T>() : nullptr);
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, T *t)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        setPointer(t);
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, const T *t)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        setPointer(t);
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, typename T::Data *t)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        *ptr = t;
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, const ReturnedValue &v)
    {
        ptr = scope.alloc<Scope::Uninitialized>();
        setPointer(QV4::Value::fromReturnedValue(v).as<T>());
    }

    QML_NEARLY_ALWAYS_INLINE Scoped(const Scope &scope, const ReturnedValue &v, ConvertType)
    {
        ptr = scope.construct(1, value_convert<T>(scope.engine, QV4::Value::fromReturnedValue(v)));
    }

    Scoped<T> &operator=(Heap::Base *o) {
        setPointer(Value::fromHeapObject(o).as<T>());
        return *this;
    }
    Scoped<T> &operator=(typename T::Data *t) {
        *ptr = t;
        return *this;
    }
    Scoped<T> &operator=(const Value &v) {
        setPointer(v.as<T>());
        return *this;
    }
    Scoped<T> &operator=(Value *v) {
        setPointer(v ? v->as<T>() : nullptr);
        return *this;
    }

    Scoped<T> &operator=(const ReturnedValue &v) {
        setPointer(QV4::Value::fromReturnedValue(v).as<T>());
        return *this;
    }

    Scoped<T> &operator=(T *t) {
        setPointer(t);
        return *this;
    }

    operator T *() {
        return static_cast<T *>(ptr->managed());
    }
    operator const Value &() const {
        return *ptr;
    }

    T *operator->() {
        return getPointer();
    }

    const T *operator->() const {
        return getPointer();
    }

    explicit operator bool() const {
        return ptr->m();
    }

    T *getPointer() {
        return reinterpret_cast<T *>(ptr);
    }

    const T *getPointer() const {
        return reinterpret_cast<T *>(ptr);
    }

    Value *getRef() {
        return ptr;
    }

    QML_NEARLY_ALWAYS_INLINE ReturnedValue asReturnedValue() const {
        return ptr->rawValue();
    }

    Value *ptr;
};

inline Value &Value::operator =(const ScopedValue &v)
{
    _val = v.ptr->rawValue();
    return *this;
}

template<typename T>
inline Value &Value::operator=(const Scoped<T> &t)
{
    _val = t.ptr->rawValue();
    return *this;
}

struct ScopedProperty
{
    ScopedProperty(Scope &scope)
    {
        property = reinterpret_cast<Property*>(scope.alloc(int(sizeof(Property) / sizeof(Value))));
    }

    Property *operator->() { return property; }
    operator const Property *() const { return property; }
    operator Property *() { return property; }

    Property *property;
};

}

QT_END_NAMESPACE

#endif
