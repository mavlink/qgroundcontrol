#include <coroutine>
#include <type_traits>
#include <utility>
#include <vector>

#include "GPSTask.h"
#include "UnitTest.h"

namespace {

/// Suspends its awaiter until the test opens it, standing in for an I/O request the runtime would complete.
struct Gate
{
    struct Awaiter
    {
        Gate& gate;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> handle) noexcept { gate.waiter = handle; }

        int await_resume() const noexcept { return gate.value; }
    };

    Awaiter operator co_await() noexcept { return {*this}; }

    void open(int result)
    {
        value = result;
        std::exchange(waiter, {}).resume();
    }

    std::coroutine_handle<> waiter;
    int value = 0;
};

struct Tracker
{
    int& destroyed;

    ~Tracker() { ++destroyed; }
};

GPSTask<int> gated(Gate& gate, std::vector<int>& order, int& destroyed)
{
    const Tracker tracker{destroyed};
    order.push_back(2);
    const int value = co_await gate;
    order.push_back(3);
    co_return value * 10;
}

GPSTask<void> leaf(std::vector<int>& order)
{
    order.push_back(4);
    co_return;
}

GPSTask<int> outer(Gate& gate, std::vector<int>& order, int& destroyed)
{
    const Tracker tracker{destroyed};
    order.push_back(1);
    const int child = co_await gated(gate, order, destroyed);
    co_await leaf(order);
    auto named = leaf(order);
    co_await named;
    // A finished task yields its result again without suspending.
    co_await named;
    order.push_back(5);
    co_return child + 1;
}

GPSTask<int> immediate(int value)
{
    co_return value;
}

GPSTask<int> awaitEmpty(std::vector<int>& order)
{
    co_await GPSTask<void>{};
    order.push_back(1);
    co_await GPSTask<void>::ready();
    order.push_back(2);
    co_return 3;
}

GPSTask<long> sum(int count)
{
    long total = 0;
    for (int index = 0; index < count; ++index) {
        total += co_await immediate(index);
    }
    co_return total;
}

}  // namespace

class GPSTaskTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _lazyNestedResult();
    void _destroyWhileSuspended();
    void _moveSemantics();
    void _emptyVoidTaskIsFinished();
    void _symmetricTransferKeepsStackFlat();
};

void GPSTaskTest::_lazyNestedResult()
{
    Gate gate;
    std::vector<int> order;
    int destroyed = 0;
    auto task = outer(gate, order, destroyed);
    QVERIFY(task.valid());
    QVERIFY(order.empty());
    QVERIFY(!task.done());

    task.start();
    QCOMPARE(order, (std::vector<int>{1, 2}));
    QVERIFY(!task.done());
    // A second start never resumes a task suspended inside a child.
    task.start();
    QCOMPARE(order, (std::vector<int>{1, 2}));

    gate.open(4);
    QVERIFY(task.done());
    QCOMPARE(order, (std::vector<int>{1, 2, 3, 4, 4, 5}));
    QCOMPARE(task.result(), 41);
    QCOMPARE(destroyed, 2);
}

void GPSTaskTest::_destroyWhileSuspended()
{
    Gate gate;
    std::vector<int> order;
    int destroyed = 0;
    {
        auto task = outer(gate, order, destroyed);
        task.start();
        QVERIFY(gate.waiter);
        QCOMPARE(destroyed, 0);
    }
    // Both the suspended child frame and its parent were destroyed; nothing after the suspension ran.
    QCOMPARE(destroyed, 2);
    QCOMPARE(order, (std::vector<int>{1, 2}));

    auto task = outer(gate, order, destroyed);
    task.start();
    task.reset();
    QVERIFY(!task.valid());
    QVERIFY(task.done());
    QCOMPARE(destroyed, 4);
}

void GPSTaskTest::_moveSemantics()
{
    Gate gate;
    std::vector<int> order;
    int destroyed = 0;
    auto first = outer(gate, order, destroyed);
    first.start();

    GPSTask<int> moved(std::move(first));
    QVERIFY(!first.valid());
    QVERIFY(first.done());
    QVERIFY(moved.valid());
    QVERIFY(!moved.done());

    // Assigning over a suspended task cancels it.
    moved = immediate(7);
    QCOMPARE(destroyed, 2);
    moved.start();
    QVERIFY(moved.done());
    QCOMPARE(moved.result(), 7);

    GPSTask<int> assigned = immediate(1);
    assigned = std::move(moved);
    QVERIFY(!moved.valid());
    QCOMPARE(assigned.result(), 7);
}

void GPSTaskTest::_emptyVoidTaskIsFinished()
{
    // Only GPSTask<void> has an empty state that can be awaited; a value task always carries a frame.
    static_assert(std::is_default_constructible_v<GPSTask<void>>);
    static_assert(!std::is_default_constructible_v<GPSTask<int>>);
    GPSTask<void> empty;
    QVERIFY(!empty.valid());
    QVERIFY(empty.done());
    empty.start();
    empty.result();
    std::vector<int> order;
    auto task = awaitEmpty(order);
    task.start();
    QVERIFY(task.done());
    QCOMPARE(order, (std::vector<int>{1, 2}));
    QCOMPARE(task.result(), 3);
}

void GPSTaskTest::_symmetricTransferKeepsStackFlat()
{
    constexpr int COUNT = 200000;
    auto task = sum(COUNT);
    task.start();
    QVERIFY(task.done());
    QCOMPARE(task.result(), static_cast<long>(COUNT) * (COUNT - 1) / 2);
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSTaskTest, TestLabel::Unit)

#include "GPSTaskTest.moc"
