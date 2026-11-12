#include "AutoSuspendGuardTest.h"

#include <atomic>
#include <optional>
#include <utility>

#include "AutoSuspendGuard.h"

void AutoSuspendGuardTest::_testSuspendsForItsLifetime()
{
    std::atomic<bool> suspended = false;
    {
        const QGC::AutoSuspendGuard guard(suspended);
        QVERIFY(suspended);
    }
    QVERIFY(!suspended);
}

void AutoSuspendGuardTest::_testMovedFromGuardLeavesFlagSuspended()
{
    std::atomic<bool> suspended = false;
    {
        std::optional<QGC::AutoSuspendGuard> source(std::in_place, suspended);
        const QGC::AutoSuspendGuard target(std::move(*source));
        source.reset();
        QVERIFY(suspended);
    }
    QVERIFY(!suspended);
}

void AutoSuspendGuardTest::_testMoveAssignmentReleasesPreviousFlag()
{
    std::atomic<bool> first = false;
    std::atomic<bool> second = false;
    {
        QGC::AutoSuspendGuard guard(first);
        {
            QGC::AutoSuspendGuard other(second);
            guard = std::move(other);
            QVERIFY(!first);
            QVERIFY(second);
        }
        QVERIFY(second);
    }
    QVERIFY(!second);
}

UT_REGISTER_TEST(AutoSuspendGuardTest, TestLabel::Unit, TestLabel::Utilities)
