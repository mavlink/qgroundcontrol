// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant
#ifndef QMLJS_MANAGED_H
#define QMLJS_MANAGED_H

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

#include "qv4global_p.h"
#include "qv4value_p.h"
#include "qv4enginebase_p.h"
#include <private/qv4heap_p.h>
#include <private/qv4vtable_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

#define Q_MANAGED_CHECK \
    template <typename Type> inline void qt_check_for_QMANAGED_macro(const Type *_q_argument) const \
    { int i = qYouForgotTheQ_MANAGED_Macro(this, _q_argument); i = i + 1; }

template <typename T>
inline int qYouForgotTheQ_MANAGED_Macro(T, T) { return 0; }

template <typename T1, typename T2>
inline void qYouForgotTheQ_MANAGED_Macro(T1, T2) {}

#define V4_MANAGED_SIZE_TEST void __dataTest() { static_assert (sizeof(*this) == sizeof(Managed), "Classes derived from Managed can't have own data members."); }

#define V4_NEEDS_DESTROY static void virtualDestroy(QV4::Heap::Base *b) { static_cast<Data *>(b)->destroy(); }


#define V4_MANAGED_ITSELF(DataClass, superClass) \
    public: \
        Q_MANAGED_CHECK \
        typedef QV4::Heap::DataClass Data; \
        typedef superClass SuperClass; \
        static const QV4::VTable static_vtbl; \
        static inline const QV4::VTable *staticVTable() { return &static_vtbl; } \
        V4_MANAGED_SIZE_TEST \
        QV4::Heap::DataClass *d_unchecked() const { return static_cast<QV4::Heap::DataClass *>(m()); } \
        QV4::Heap::DataClass *d() const { \
            QV4::Heap::DataClass *dptr = d_unchecked(); \
            dptr->_checkIsInitialized(); \
            return dptr; \
        }

#define V4_MANAGED(DataClass, superClass) \
    private: \
        DataClass() = delete; \
        Q_DISABLE_COPY(DataClass) \
        V4_MANAGED_ITSELF(DataClass, superClass) \
        static_assert(std::is_trivially_copyable_v<QV4::Heap::DataClass>); \
        static_assert(std::is_trivially_default_constructible_v<QV4::Heap::DataClass>);

#define Q_MANAGED_TYPE(type) \
    public: \
        enum { MyType = Type_##type };

#define V4_INTERNALCLASS(c) \
    static Heap::InternalClass *defaultInternalClass(QV4::EngineBase *e) \
        { return e->internalClasses(QV4::EngineBase::Class_##c); }

struct Q_QML_EXPORT Managed : Value, VTableBase
{
    V4_MANAGED_ITSELF(Base, Managed)
    enum {
        IsExecutionContext = false,
        IsString = false,
        IsStringOrSymbol = false,
        IsObject = false,
        IsTailCallable = false,
        IsErrorObject = false,
        IsArrayData = false
    };
private:
    void *operator new(size_t);
    Managed() = delete;
    Q_DISABLE_COPY(Managed)

public:
    enum { NInlineProperties = 0 };

    enum Type {
        Type_Invalid,
        Type_String,
        Type_Object,
        Type_Symbol,
        Type_ArrayObject,
        Type_FunctionObject,
        Type_GeneratorObject,
        Type_BooleanObject,
        Type_NumberObject,
        Type_StringObject,
        Type_SymbolObject,
        Type_DateObject,
        Type_RegExpObject,
        Type_ErrorObject,
        Type_ArgumentsObject,
        Type_JsonObject,
        Type_MathObject,
        Type_ProxyObject,
        Type_UrlObject,
        Type_UrlSearchParamsObject,

        Type_ExecutionContext,
        Type_InternalClass,
        Type_SetIteratorObject,
        Type_MapIteratorObject,
        Type_ArrayIteratorObject,
        Type_StringIteratorObject,
        Type_ForInIterator,
        Type_RegExp,

        Type_V4Sequence,
        Type_QmlListProperty,
        Type_V4QObjectWrapper,
        Type_QMLTypeWrapper,
        Type_V4ReferenceObject,
        Type_QMLValueTypeWrapper,
    };
    Q_MANAGED_TYPE(Invalid)

    Heap::InternalClass *internalClass() const { return d()->internalClass; }
    const VTable *vtable() const { return d()->internalClass->vtable; }
    inline ExecutionEngine *engine() const { return internalClass()->engine; }

    bool isV4SequenceType() const { return d()->internalClass->vtable->type == Type_V4Sequence; }
    bool isV4QObjectWrapper() const { return d()->internalClass->vtable->type == Type_V4QObjectWrapper; }
    bool isQmlListPropertyType() const { return d()->internalClass->vtable->type == Type_QmlListProperty; }
    bool isArrayLike() const { return isArrayObject() || isV4SequenceType() || isQmlListPropertyType(); }

    bool isArrayObject() const { return d()->internalClass->vtable->type == Type_ArrayObject; }
    bool isStringObject() const { return d()->internalClass->vtable->type == Type_StringObject; }
    bool isSymbolObject() const { return d()->internalClass->vtable->type == Type_SymbolObject; }

    QString className() const;

    bool isEqualTo(const Managed *other) const
    { return d()->internalClass->vtable->isEqualTo(const_cast<Managed *>(this), const_cast<Managed *>(other)); }

    bool inUse() const { return d()->inUse(); }
    bool markBit() const { return d()->isMarked(); }
    inline void mark(MarkStack *markStack);

    Q_ALWAYS_INLINE Heap::Base *heapObject() const {
        return m();
    }

    template<typename T> inline T *cast() {
        return static_cast<T *>(this);
    }
    template<typename T> inline const T *cast() const {
        return static_cast<const T *>(this);
    }

protected:
    static bool virtualIsEqualTo(Managed *m, Managed *other);

private:
    friend class MemoryManager;
    friend struct Identifiers;
    friend struct ObjectIterator;
};

inline void Managed::mark(MarkStack *markStack)
{
    Q_ASSERT(m());
    m()->mark(markStack);
}

template<>
inline const Managed *Value::as() const {
    return managed();
}

template<>
inline const Object *Value::as() const {
    return objectValue();
}


struct InternalClass : Managed
{
    V4_MANAGED_ITSELF(InternalClass, Managed)
    Q_MANAGED_TYPE(InternalClass)
    V4_INTERNALCLASS(Empty)
    V4_NEEDS_DESTROY

    Q_REQUIRED_RESULT Heap::InternalClass *changeVTable(const VTable *vt) {
        return d()->changeVTable(vt);
    }
    Q_REQUIRED_RESULT Heap::InternalClass *changePrototype(Heap::Object *proto) {
        return d()->changePrototype(proto);
    }
    Q_REQUIRED_RESULT Heap::InternalClass *addMember(PropertyKey identifier, PropertyAttributes data, InternalClassEntry *entry = nullptr) {
        return d()->addMember(identifier, data, entry);
    }

    Q_REQUIRED_RESULT Heap::InternalClass *changeMember(PropertyKey identifier, PropertyAttributes data, InternalClassEntry *entry = nullptr) {
        return d()->changeMember(identifier, data, entry);
    }

    void operator =(Heap::InternalClass *ic) {
        Value::operator=(ic);
    }
};

}


QT_END_NAMESPACE

#endif
