// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:low-level-memory-management

#ifndef QV4GC_H
#define QV4GC_H

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
#include <private/qv4value_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4object_p.h>
#include <private/qv4mmdefs_p.h>
#include <QVector>

#define MM_DEBUG 0

QT_BEGIN_NAMESPACE

namespace QV4 {

// Iterate a potentially growing container without querying the size on each iteration.
// Return the first index where p doesn't hold, or the end of the container.
template<typename Container, typename UnaryPred>
typename Container::size_type reiterate(
        Container &container, typename Container::size_type first, UnaryPred &&p)
{
    auto last = container.size();
    while (first < last) {
        do {
            if (!p(first))
                return first;
        } while (++first < last);

        Q_ASSERT(first == last);

        // Re-fetch the container size.
        // If the container hasn't grown, we're done since last == first, still.
        // Otherwise, do one more round via the "while (first < last)" above.
        last = container.size();
    }

    // Returning last here allows input of an out of range first, saving us
    // one size check ahead of running this.
    return last;
}

// index based version of partition to handle potential growth at the end
template<typename Container, class UnaryPred>
typename Container::size_type partition(Container &container, UnaryPred &&p)
{
    // Figure out the first entry where p doesn't hold.
    auto first = reiterate(container, 0, p);

    // Iterate the remaining entries and swap any entry where p holds
    // to the (moving) end of the front section of the container.
    // Any time we do that, the front section grows by 1. Therefore, the back
    // section can only contain entries where p doesn't hold in the end.
    reiterate(container, first + 1, [&p, &container, &first](const auto i) {
        if (p(i)) {
            // It's important to re-resolve container.at(i) for the std::swap.
            // The container may have grown as result of determining p, thereby
            // invalidating any reference to an entry taken before.
            std::swap(container.at(i), container.at(first++));
        }
        return true;
    });

    return first;
}

struct GCData { virtual ~GCData(){};};

struct GCIteratorStorage {
    PersistentValueStorage::Iterator it{nullptr, 0};
};

struct GCStateMachine {
    Q_GADGET_EXPORT(Q_QML_EXPORT)

public:
    enum GCState {
        MarkStart = 0,
        MarkGlobalObject,
        MarkJSStack,
        InitMarkPersistentValues,
        MarkPersistentValues,
        InitMarkWeakValues,
        MarkWeakValues,
        MarkDrain,
        MarkReady,
        InitCallDestroyObjects,
        CallDestroyObjects,
        FreeWeakMaps,
        FreeWeakSets,
        HandleQObjectWrappers,
        DoSweep,
        Invalid,
        Count,
    };
    Q_ENUM(GCState)

    struct StepTiming {
        qint64 rolling_sum = 0;
        qint64 count = 0;
    };

    struct GCStateInfo {
        using ExtraData = std::variant<std::monostate, GCIteratorStorage>;
        GCState (*execute)(GCStateMachine *, ExtraData &) = nullptr;  // Function to execute for this state, returns true if ready to transition
        bool breakAfter{false};
    };

    using ExtraData = GCStateInfo::ExtraData;
    GCState state{GCState::Invalid};
    std::chrono::microseconds timeLimit{};
    QDeadlineTimer deadline;
    std::array<GCStateInfo, GCState::Count> stateInfoMap;
    std::array<StepTiming, GCState::Count> executionTiming{};
    MemoryManager *mm = nullptr;
    ExtraData stateData; // extra date for specific states
    bool collectTimings = false;

    GCStateMachine();

    inline void step() {
        if (!inProgress()) {
            reset();
        }
        transition();
    }

    inline bool inProgress() {
        return state != GCState::Invalid;
    }

    inline void reset() {
        state = GCState::MarkStart;
    }

    Q_QML_EXPORT void transition();

    inline void handleTimeout(GCState state) {
        Q_UNUSED(state);
    }
};

using GCState = GCStateMachine::GCState;
using GCStateInfo = GCStateMachine::GCStateInfo;

struct ChunkAllocator;
struct MemorySegment;

struct BlockAllocator {
    BlockAllocator(ChunkAllocator *chunkAllocator, ExecutionEngine *engine)
        : chunkAllocator(chunkAllocator), engine(engine)
    {
        memset(freeBins, 0, sizeof(freeBins));
    }

    enum { NumBins = 8 };

    static inline size_t binForSlots(size_t nSlots) {
        return nSlots >= NumBins ? NumBins - 1 : nSlots;
    }

    HeapItem *allocate(size_t size, bool forceAllocation = false);

    size_t totalSlots() const {
        return Chunk::AvailableSlots*chunks.size();
    }

    size_t allocatedMem() const {
        return chunks.size()*Chunk::DataSize;
    }
    size_t usedMem() const {
        uint used = 0;
        for (auto c : chunks)
            used += c->nUsedSlots()*Chunk::SlotSize;
        return used;
    }

    void sweep();
    void freeAll();
    void resetBlackBits();

    // bump allocations
    HeapItem *nextFree = nullptr;
    size_t nFree = 0;
    size_t usedSlotsAfterLastSweep = 0;
    HeapItem *freeBins[NumBins];
    ChunkAllocator *chunkAllocator;
    ExecutionEngine *engine;
    std::vector<Chunk *> chunks;
    uint *allocationStats = nullptr;
};

struct HugeItemAllocator {
    HugeItemAllocator(ChunkAllocator *chunkAllocator, ExecutionEngine *engine)
        : chunkAllocator(chunkAllocator), engine(engine)
    {}

    HeapItem *allocate(size_t size);
    void sweep(ClassDestroyStatsCallback classCountPtr);
    void freeAll();
    void resetBlackBits();

    size_t usedMem() const {
        size_t used = 0;
        for (const auto &c : chunks)
            used += c.size;
        return used;
    }

    ChunkAllocator *chunkAllocator;
    ExecutionEngine *engine;
    struct HugeChunk {
        MemorySegment *segment;
        Chunk *chunk;
        size_t size;
    };

    std::vector<HugeChunk> chunks;
};


class Q_QML_EXPORT MemoryManager
{
    Q_DISABLE_COPY(MemoryManager);

public:
    MemoryManager(ExecutionEngine *engine);
    ~MemoryManager();

    template <typename ToBeMarked>
    friend struct  GCCriticalSection;

    // TODO: this is only for 64bit (and x86 with SSE/AVX), so exend it for other architectures to be slightly more efficient (meaning, align on 8-byte boundaries).
    // Note: all occurrences of "16" in alloc/dealloc are also due to the alignment.
    constexpr static inline std::size_t align(std::size_t size)
    { return (size + Chunk::SlotSize - 1) & ~(Chunk::SlotSize - 1); }

    /* NOTE: allocManaged comes in various overloads. If size is not passed explicitly
       sizeof(ManagedType::Data) is used for size. However, there are quite a few cases
       where we allocate more than sizeof(ManagedType::Data); that's generally the case
       when the Object has a ValueArray member.
       If no internal class pointer is provided, ManagedType::defaultInternalClass(engine)
       will be used as the internal class.
    */

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(std::size_t size, Heap::InternalClass *ic)
    {
        static_assert(std::is_trivially_copyable_v<typename ManagedType::Data>);
        static_assert(std::is_trivially_default_constructible_v<typename ManagedType::Data>);
        size = align(size);
        typename ManagedType::Data *d = static_cast<typename ManagedType::Data *>(allocData(size));
        d->internalClass.set(engine, ic);
        Q_ASSERT(d->internalClass && d->internalClass->vtable);
        Q_ASSERT(ic->vtable == ManagedType::staticVTable());
        return d;
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(Heap::InternalClass *ic)
    {
        return allocManaged<ManagedType>(sizeof(typename ManagedType::Data), ic);
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(std::size_t size, InternalClass *ic)
    {
        return allocManaged<ManagedType>(size, ic->d());
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(InternalClass *ic)
    {
        return allocManaged<ManagedType>(sizeof(typename ManagedType::Data), ic);
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged(std::size_t size)
    {
        Scope scope(engine);
        Scoped<InternalClass> ic(scope, ManagedType::defaultInternalClass(engine));
        return allocManaged<ManagedType>(size, ic);
    }

    template<typename ManagedType>
    inline typename ManagedType::Data *allocManaged()
    {
        auto constexpr size = sizeof(typename ManagedType::Data);
        Scope scope(engine);
        Scoped<InternalClass> ic(scope, ManagedType::defaultInternalClass(engine));
        return allocManaged<ManagedType>(size, ic);
    }

    template <typename ObjectType>
    typename ObjectType::Data *allocateObject(Heap::InternalClass *ic)
    {
        Heap::Object *o = allocObjectWithMemberData(ObjectType::staticVTable(), ic->size);
        o->internalClass.set(engine, ic);
        Q_ASSERT(o->internalClass.get() && o->vtable());
        Q_ASSERT(o->vtable() == ObjectType::staticVTable());
        return static_cast<typename ObjectType::Data *>(o);
    }

    template <typename ObjectType>
    typename ObjectType::Data *allocateObject(InternalClass *ic)
    {
        return allocateObject<ObjectType>(ic->d());
    }

    template <typename ObjectType>
    typename ObjectType::Data *allocateObject()
    {
        Scope scope(engine);
        Scoped<InternalClass> ic(scope,  ObjectType::defaultInternalClass(engine));
        ic = ic->changeVTable(ObjectType::staticVTable());
        ic = ic->changePrototype(ObjectType::defaultPrototype(engine)->d());
        return allocateObject<ObjectType>(ic);
    }

    template <typename ManagedType, typename Arg1>
    typename ManagedType::Data *allocWithStringData(std::size_t unmanagedSize, Arg1 &&arg1)
    {
        typename ManagedType::Data *o = reinterpret_cast<typename ManagedType::Data *>(allocString(unmanagedSize));
        o->internalClass.set(engine, ManagedType::defaultInternalClass(engine));
        Q_ASSERT(o->internalClass && o->internalClass->vtable);
        o->init(std::forward<Arg1>(arg1));
        return o;
    }

    template <typename ObjectType, typename... Args>
    typename ObjectType::Data *allocObject(Heap::InternalClass *ic, Args&&... args)
    {
        typename ObjectType::Data *d = allocateObject<ObjectType>(ic);
        d->init(std::forward<Args>(args)...);
        return d;
    }

    template <typename ObjectType, typename... Args>
    typename ObjectType::Data *allocObject(InternalClass *ic, Args&&... args)
    {
        typename ObjectType::Data *d = allocateObject<ObjectType>(ic);
        d->init(std::forward<Args>(args)...);
        return d;
    }

    template <typename ObjectType, typename... Args>
    typename ObjectType::Data *allocate(Args&&... args)
    {
        Scope scope(engine);
        Scoped<ObjectType> t(scope, allocateObject<ObjectType>());
        t->d_unchecked()->init(std::forward<Args>(args)...);
        return t->d();
    }

    template <typename ManagedType, typename... Args>
    typename ManagedType::Data *alloc(Args&&... args)
    {
        Scope scope(engine);
        Scoped<ManagedType> t(scope, allocManaged<ManagedType>());
        t->d_unchecked()->init(std::forward<Args>(args)...);
        return t->d();
    }

    void runGC();
    bool tryForceGCCompletion();
    void runFullGC();

    void dumpStats() const;

    size_t getUsedMem() const;
    size_t getAllocatedMem() const;
    size_t getLargeItemsMem() const;

    // called when a JS object grows itself. Specifically: Heap::String::append
    // and InternalClassDataPrivate<PropertyAttributes>.
    void changeUnmanagedHeapSizeUsage(qptrdiff delta) { unmanagedHeapSize += delta; }

    // called at the end of a gc cycle
    void updateUnmanagedHeapSizeGCLimit();

    template<typename ManagedType>
    typename ManagedType::Data *allocIC()
    {
        Heap::Base *b = *allocate(&icAllocator, align(sizeof(typename ManagedType::Data)));
        return static_cast<typename ManagedType::Data *>(b);
    }

    void registerWeakMap(Heap::MapObject *map);
    void registerWeakSet(Heap::SetObject *set);

    void onEventLoop();

    //GC related methods
    void setGCTimeLimit(int timeMs);
    MarkStack* markStack() { return m_markStack.get(); }

protected:
    /// expects size to be aligned
    Heap::Base *allocString(std::size_t unmanagedSize);
    Heap::Base *allocData(std::size_t size);
    Heap::Object *allocObjectWithMemberData(const QV4::VTable *vtable, uint nMembers);

private:
    enum {
        MinUnmanagedHeapSizeGCLimit = 128 * 1024
    };

public:
    void collectFromJSStack(MarkStack *markStack) const;
    void sweep(bool lastSweep = false, ClassDestroyStatsCallback classCountPtr = nullptr);
    void cleanupDeletedQObjectWrappersInSweep();
    bool isAboveUnmanagedHeapLimit()
    {
        const bool incrementalGCIsAlreadyRunning = m_markStack != nullptr;
        const bool aboveUnmanagedHeapLimit = incrementalGCIsAlreadyRunning
                ? unmanagedHeapSize > 3 * unmanagedHeapSizeGCLimit / 2
                : unmanagedHeapSize > unmanagedHeapSizeGCLimit;
        return aboveUnmanagedHeapLimit;
    }
private:
    bool shouldRunGC() const;

    HeapItem *allocate(BlockAllocator *allocator, std::size_t size)
    {
        const bool incrementalGCIsAlreadyRunning = m_markStack != nullptr;

        bool didGCRun = false;
        if (aggressiveGC) {
            runFullGC();
            didGCRun = true;
        }

        if (isAboveUnmanagedHeapLimit()) {
            if (!didGCRun)
                incrementalGCIsAlreadyRunning ? (void) tryForceGCCompletion() : runGC();
            didGCRun = true;
        }

        if (size > Chunk::DataSize)
            return hugeItemAllocator.allocate(size);

        if (HeapItem *m = allocator->allocate(size))
            return m;

        if (!didGCRun && shouldRunGC())
            runGC();

        return allocator->allocate(size, true);
    }

public:
    QV4::ExecutionEngine *engine;
    ChunkAllocator *chunkAllocator;
    BlockAllocator blockAllocator;
    BlockAllocator icAllocator;
    HugeItemAllocator hugeItemAllocator;
    PersistentValueStorage *m_persistentValues;
    PersistentValueStorage *m_weakValues;
    QVector<Value *> m_pendingFreedObjectWrapperValue;
    Heap::MapObject *weakMaps = nullptr;
    Heap::SetObject *weakSets = nullptr;

    std::unique_ptr<GCStateMachine> gcStateMachine{nullptr};
    std::unique_ptr<MarkStack> m_markStack{nullptr};

    std::size_t unmanagedHeapSize = 0; // the amount of bytes of heap that is not managed by the memory manager, but which is held onto by managed items.
    std::size_t unmanagedHeapSizeGCLimit;
    std::size_t usedSlotsAfterLastFullSweep = 0;

    enum Blockness : quint8 {Unblocked, NormalBlocked, InCriticalSection };
    Blockness gcBlocked = Unblocked;
    bool aggressiveGC = false;
    bool gcStats = false;
    bool gcCollectorStats = false;

    int allocationCount = 0;
    size_t lastAllocRequestedSlots = 0;

    struct {
        size_t maxReservedMem = 0;
        size_t maxAllocatedMem = 0;
        size_t maxUsedMem = 0;
        uint allocations[BlockAllocator::NumBins];
    } statistics;
};

/*!
    \internal
    GCCriticalSection prevets the gc from running, until it is destructed.
    In its dtor, it runs a check whether we've reached the unmanaegd heap limit,
    and triggers a gc run if necessary.
    Lastly, it can optionally mark an object passed to it before runnig the gc.
 */
template <typename ToBeMarked = void>
struct GCCriticalSection {
    Q_DISABLE_COPY_MOVE(GCCriticalSection)

    Q_NODISCARD_CTOR GCCriticalSection(QV4::ExecutionEngine *engine, ToBeMarked *toBeMarked = nullptr)
        : m_engine(engine)
          , m_oldState(std::exchange(engine->memoryManager->gcBlocked, MemoryManager::InCriticalSection))
          , m_toBeMarked(toBeMarked)
    {
        // disallow nested critical sections
        Q_ASSERT(m_oldState != MemoryManager::InCriticalSection);
    }
    ~GCCriticalSection()
    {
        m_engine->memoryManager->gcBlocked = m_oldState;
        if (m_oldState != MemoryManager::Unblocked)
            if constexpr (!std::is_same_v<ToBeMarked, void>)
                if (m_toBeMarked)
                    m_toBeMarked->markObjects(m_engine->memoryManager->markStack());
        /* because we blocked the gc, we might be using too much memoryon the unmanaged heap
           and did not run the normal fixup logic. So recheck again, and trigger a gc run
           if necessary*/
        if (!m_engine->memoryManager->isAboveUnmanagedHeapLimit())
            return;
        if (!m_engine->isGCOngoing) {
            m_engine->memoryManager->runGC();
        } else {
            [[maybe_unused]] bool gcFinished = m_engine->memoryManager->tryForceGCCompletion();
            Q_ASSERT(gcFinished);
        }
    }

private:
    QV4::ExecutionEngine *m_engine;
    MemoryManager::Blockness m_oldState;
    ToBeMarked *m_toBeMarked;
};

}

QT_END_NAMESPACE

#endif // QV4GC_H
