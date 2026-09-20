#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <semaphore>
#include <thread>

#include <QtCore/QByteArray>
#include <QtCore/QScopeGuard>

#include "AllocationTracker.h"
#include "UnitTest.h"

namespace {
constexpr auto ALIGNMENT = std::align_val_t{64};
thread_local int handlerCalls = 0;

struct HandlerFailure
{
};

void* allocate(int form, std::size_t size)
{
    switch (form) {
        case 0:
            return ::operator new(size);
        case 1:
            return ::operator new[](size);
        case 2:
            return ::operator new(size, std::nothrow);
        case 3:
            return ::operator new[](size, std::nothrow);
        case 4:
            return ::operator new(size, ALIGNMENT);
        case 5:
            return ::operator new[](size, ALIGNMENT);
        case 6:
            return ::operator new(size, ALIGNMENT, std::nothrow);
        case 7:
            return ::operator new[](size, ALIGNMENT, std::nothrow);
    }
    return nullptr;
}

void release(int form, void* memory)
{
    switch (form) {
        case 0:
            ::operator delete(memory);
            break;
        case 1:
            ::operator delete[](memory);
            break;
        case 2:
            ::operator delete(memory, std::nothrow);
            break;
        case 3:
            ::operator delete[](memory, std::nothrow);
            break;
        case 4:
            ::operator delete(memory, ALIGNMENT);
            break;
        case 5:
            ::operator delete[](memory, ALIGNMENT);
            break;
        case 6:
            ::operator delete(memory, ALIGNMENT, std::nothrow);
            break;
        case 7:
            ::operator delete[](memory, ALIGNMENT, std::nothrow);
            break;
    }
}
}  // namespace

class AllocationTrackerTest : public UnitTest
{
    Q_OBJECT

private slots:

    void _allocationForms_data();
    void _allocationForms();
    void _failureSemantics_data();
    void _failureSemantics();
    void _nestedScopes();
    void _threadIsolation();
};

void AllocationTrackerTest::_allocationForms_data()
{
    QTest::addColumn<int>("form");
    QTest::addColumn<quint64>("size");
    constexpr std::array names{"scalar",  "array",         "nothrow",         "nothrow-array",
                               "aligned", "aligned-array", "aligned-nothrow", "aligned-nothrow-array"};
    for (size_t form = 0; form < names.size(); ++form) {
        for (const quint64 size : {0, 7}) {
            const auto name = QByteArray(names[form]) + '-' + QByteArray::number(size);
            QTest::newRow(name.constData()) << static_cast<int>(form) << size;
        }
    }
}

void AllocationTrackerTest::_allocationForms()
{
    QFETCH(int, form);
    QFETCH(quint64, size);
    void* memory = nullptr;
    QGCTest::AllocationTracker::Counts counts;
    {
        QGCTest::AllocationTracker tracker;
        memory = allocate(form, size);
        counts = tracker.counts();
    }
    const bool allocated = memory != nullptr;
    const auto address = reinterpret_cast<std::uintptr_t>(memory);
    release(form, memory);
    QVERIFY(allocated);
    QCOMPARE(address % (form >= 4 ? 64 : alignof(std::max_align_t)), std::uintptr_t{0});
    QCOMPARE(counts.calls, size_t{1});
    QCOMPARE(counts.bytes, size_t(size));
    QCOMPARE(counts.largest, size_t(size));
}

void AllocationTrackerTest::_failureSemantics_data()
{
    QTest::addColumn<int>("form");
    QTest::addColumn<int>("handler");
    for (int form = 0; form < 8; ++form) {
        for (int handler = 0; handler < 3; ++handler) {
            const auto name = QByteArray::number(form) + '-' + QByteArray::number(handler);
            QTest::newRow(name.constData()) << form << handler;
        }
    }
}

void AllocationTrackerTest::_failureSemantics()
{
    QFETCH(int, form);
    QFETCH(int, handler);
    handlerCalls = 0;
    std::new_handler replacement = nullptr;
    if (handler == 1) {
        replacement = [] { ++handlerCalls; };
    } else if (handler == 2) {
        replacement = [] {
            ++handlerCalls;
            throw HandlerFailure{};
        };
    }
    bool badAllocation = false;
    bool handlerFailure = false;
    void* memory = nullptr;
    QGCTest::AllocationTracker::Counts counts;
    {
        const auto previous = std::set_new_handler(replacement);
        const auto restore = qScopeGuard([previous] { std::set_new_handler(previous); });
        QGCTest::AllocationFailureScope failure(1);
        QGCTest::AllocationTracker tracker;
        try {
            memory = allocate(form, 7);
        } catch (const std::bad_alloc&) {
            badAllocation = true;
        } catch (const HandlerFailure&) {
            handlerFailure = true;
        }
        counts = tracker.counts();
    }
    const bool allocated = memory != nullptr;
    release(form, memory);
    const bool nothrow = (form & 2) != 0;
    QCOMPARE(handlerCalls, handler ? 1 : 0);
    QCOMPARE(allocated, handler == 1);
    QCOMPARE(badAllocation, !nothrow && handler == 0);
    QCOMPARE(handlerFailure, !nothrow && handler == 2);
    QVERIFY(counts.calls >= size_t{1});
    QVERIFY(counts.bytes >= size_t{7});
    if (handler == 1) {
        QCOMPARE(counts.calls, size_t{1});
    }
}

void AllocationTrackerTest::_nestedScopes()
{
    QGCTest::AllocationTracker::Counts innerCounts;
    QGCTest::AllocationTracker::Counts beforeUnwind;
    QGCTest::AllocationTracker::Counts afterUnwind;
    QGCTest::AllocationTracker::Counts outerCounts;
    {
        QGCTest::AllocationTracker outer;
        void* scalar = ::operator new(7);
#if defined(__cpp_sized_deallocation)
        ::operator delete(scalar, size_t{7});
#else
        ::operator delete(scalar);
#endif
        try {
            QGCTest::AllocationTracker inner;
            void* array = ::operator new[](9, ALIGNMENT);
#if defined(__cpp_sized_deallocation)
            ::operator delete[](array, size_t{9}, ALIGNMENT);
#else
            ::operator delete[](array, ALIGNMENT);
#endif
            innerCounts = inner.counts();
            beforeUnwind = outer.counts();
            throw HandlerFailure{};
        } catch (const HandlerFailure&) {
        }
        afterUnwind = outer.counts();
        void* aligned = ::operator new(64, ALIGNMENT);
#if defined(__cpp_sized_deallocation)
        ::operator delete(aligned, size_t{64}, ALIGNMENT);
#else
        ::operator delete(aligned, ALIGNMENT);
#endif
        outerCounts = outer.counts();
    }
    QCOMPARE(innerCounts.calls, size_t{1});
    QCOMPARE(innerCounts.bytes, size_t{9});
    QCOMPARE(beforeUnwind.calls, size_t{2});
    QCOMPARE(beforeUnwind.bytes, size_t{16});
    QCOMPARE(outerCounts.calls, afterUnwind.calls + 1);
    QCOMPARE(outerCounts.bytes, afterUnwind.bytes + 64);
    QVERIFY(outerCounts.largest >= size_t{64});
}

void AllocationTrackerTest::_threadIsolation()
{
    std::binary_semaphore start(0);
    std::binary_semaphore done(0);
    std::thread worker([&] {
        start.acquire();
        void* memory = ::operator new(17);
        ::operator delete(memory);
        done.release();
    });
    const auto join = qScopeGuard([&worker] {
        if (worker.joinable()) {
            worker.join();
        }
    });
    QGCTest::AllocationTracker::Counts counts;
    {
        QGCTest::AllocationTracker tracker;
        start.release();
        done.acquire();
        void* memory = ::operator new(3);
        ::operator delete(memory);
        counts = tracker.counts();
    }
    worker.join();
    QCOMPARE(counts.calls, size_t{1});
    QCOMPARE(counts.bytes, size_t{3});
}

UT_REGISTER_TEST_LIGHTWEIGHT(AllocationTrackerTest, TestLabel::Unit)

#include "AllocationTrackerTest.moc"
