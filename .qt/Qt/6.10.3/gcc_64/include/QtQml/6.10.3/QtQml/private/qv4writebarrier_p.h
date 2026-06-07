// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:low-level-memory-management
#ifndef QV4WRITEBARRIER_P_H
#define QV4WRITEBARRIER_P_H

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

#include <private/qv4global_p.h>
#include <private/qv4enginebase_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
struct EngineBase;
typedef quint64 ReturnedValue;

struct WriteBarrier {

    static constexpr bool isInsertionBarrier = true;

    Q_ALWAYS_INLINE static void write(EngineBase *engine, Heap::Base *base, ReturnedValue *slot, ReturnedValue value)
    {
        if (engine->isGCOngoing)
            write_slowpath(engine, base, slot, value);
        *slot = value;
    }
    Q_QML_EXPORT Q_NEVER_INLINE static void write_slowpath(
            EngineBase *engine, Heap::Base *base,
            ReturnedValue *slot, ReturnedValue value);

    Q_ALWAYS_INLINE static void write(EngineBase *engine, Heap::Base *base, Heap::Base **slot, Heap::Base *value)
    {
        if (engine->isGCOngoing)
            write_slowpath(engine, base, slot, value);
        *slot = value;
    }
    Q_QML_EXPORT Q_NEVER_INLINE static void write_slowpath(
            EngineBase *engine, Heap::Base *base,
            Heap::Base **slot, Heap::Base *value);

    // MemoryManager isn't a complete type here, so make Engine a template argument
    // so that we can still call engine->memoryManager->markStack()
    template<typename F, typename Engine = EngineBase>
    static void markCustom(Engine *engine, F &&markFunction) {
        if (engine->isGCOngoing)
            (std::forward<F>(markFunction))(engine->memoryManager->markStack());
    }

    // HeapObjectWrapper(Base) are helper classes to ensure that
    // we always use a WriteBarrier when setting heap-objects
    // they are also trivial; if triviality is not required, use Pointer instead
    struct HeapObjectWrapperBase
    {
        // enum class avoids accidental construction via brace-init
        enum class PointerWrapper : quintptr {};
        PointerWrapper wrapped;

        void clear() { wrapped = PointerWrapper(quintptr(0)); }
    };

    template<typename HeapType>
    struct HeapObjectWrapperCommon : HeapObjectWrapperBase
    {
        HeapType *get() const { return reinterpret_cast<HeapType *>(wrapped); }
        operator HeapType *() const { return get(); }
        HeapType * operator->() const { return get(); }

        template <typename ConvertibleToHeapType>
        void set(QV4::EngineBase *engine, ConvertibleToHeapType *heapObject)
        {
            WriteBarrier::markCustom(engine, [heapObject](QV4::MarkStack *ms){
                if (heapObject)
                    heapObject->mark(ms);
            });
            wrapped = static_cast<HeapObjectWrapperBase::PointerWrapper>(quintptr(heapObject));
        }
    };

    // all types are trivial; we however want to block copies bypassing the write barrier
    // therefore, all members use a PhantomTag to reduce the likelihood
    template<typename HeapType, int PhantomTag>
    struct HeapObjectWrapper : HeapObjectWrapperCommon<HeapType> {};

    /* similar Heap::Pointer, but without the Base conversion (and its inUse assert)
       and for storing references in engine classes stored on the native heap
       Stores a "non-owning" reference to a heap-item (in the C++ sense), but should
       generally mark the heap-item; therefore set goes through a write-barrier
    */
    template<typename T>
    struct Pointer
    {
        Pointer() = default;
        ~Pointer() = default;
        Q_DISABLE_COPY_MOVE(Pointer)
        T* operator->() const { return get(); }
        operator T* () const { return get(); }

        void set(EngineBase *e, T *newVal) {
            WriteBarrier::markCustom(e, [newVal](QV4::MarkStack *ms) {
                if (newVal)
                    newVal->mark(ms);
            });
            ptr = newVal;
        }

        T* get() const { return ptr; }



    private:
        T *ptr = nullptr;
    };
};

       // ### this needs to be filled with a real memory fence once marking is concurrent
Q_ALWAYS_INLINE void fence() {}

}

QT_END_NAMESPACE

#endif
