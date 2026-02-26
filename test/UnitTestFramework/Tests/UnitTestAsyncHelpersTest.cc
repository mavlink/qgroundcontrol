#include "UnitTestAsyncHelpersTest.h"

#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>

namespace {

class AsyncSignalEmitter final : public QObject
{
    Q_OBJECT

signals:
    void ping(int value);
    void toggled(bool state);
};

}  // namespace

void UnitTestAsyncHelpersTest::_testWaitForSignalSuccess()
{
    AsyncSignalEmitter emitter;
    QSignalSpy spy(&emitter, &AsyncSignalEmitter::ping);
    QVERIFY(spy.isValid());

    QTimer::singleShot(20, &emitter, [&emitter]() { emit emitter.ping(7); });
    QVERIFY(UnitTest::waitForSignal(spy, TestTimeout::shortMs(), QStringLiteral("ping")));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toInt(), 7);
}

void UnitTestAsyncHelpersTest::_testWaitForSignalTimeout()
{
    AsyncSignalEmitter emitter;
    QSignalSpy spy(&emitter, &AsyncSignalEmitter::ping);
    QVERIFY(spy.isValid());

    QVERIFY(!UnitTest::waitForSignal(spy, 50, QStringLiteral("ping")));
    QCOMPARE(spy.count(), 0);
}

void UnitTestAsyncHelpersTest::_testWaitForSignalCountSuccess()
{
    AsyncSignalEmitter emitter;
    QSignalSpy spy(&emitter, &AsyncSignalEmitter::toggled);
    QVERIFY(spy.isValid());

    QTimer::singleShot(10, &emitter, [&emitter]() { emit emitter.toggled(true); });
    QTimer::singleShot(20, &emitter, [&emitter]() { emit emitter.toggled(false); });
    QTimer::singleShot(30, &emitter, [&emitter]() { emit emitter.toggled(true); });

    QVERIFY(UnitTest::waitForSignalCount(spy, 3, TestTimeout::shortMs(), QStringLiteral("toggled")));
    QCOMPARE(spy.count(), 3);
}

void UnitTestAsyncHelpersTest::_testWaitForSignalCountAlreadySatisfied()
{
    AsyncSignalEmitter emitter;
    QSignalSpy spy(&emitter, &AsyncSignalEmitter::ping);
    QVERIFY(spy.isValid());

    emit emitter.ping(1);
    QVERIFY(UnitTest::waitForSignalCount(spy, 1, 1, QStringLiteral("ping")));
}

void UnitTestAsyncHelpersTest::_testWaitForConditionSuccess()
{
    bool ready = false;
    QTimer::singleShot(15, this, [&ready]() { ready = true; });

    QVERIFY(UnitTest::waitForCondition([&ready]() { return ready; }, TestTimeout::shortMs(), QStringLiteral("ready")));
}

void UnitTestAsyncHelpersTest::_testWaitForConditionTimeout()
{
    bool ready = false;
    QVERIFY(!UnitTest::waitForCondition([&ready]() { return ready; }, 50, QStringLiteral("ready")));
}

void UnitTestAsyncHelpersTest::_testWaitMacros()
{
    AsyncSignalEmitter emitter;

    QSignalSpy signalSpy(&emitter, &AsyncSignalEmitter::ping);
    QVERIFY(signalSpy.isValid());
    QTimer::singleShot(10, &emitter, [&emitter]() { emit emitter.ping(42); });
    QVERIFY_SIGNAL_WAIT(signalSpy, TestTimeout::shortMs());

    QSignalSpy countSpy(&emitter, &AsyncSignalEmitter::toggled);
    QVERIFY(countSpy.isValid());
    QTimer::singleShot(10, &emitter, [&emitter]() { emit emitter.toggled(true); });
    QTimer::singleShot(20, &emitter, [&emitter]() { emit emitter.toggled(false); });
    QVERIFY_SIGNAL_COUNT_WAIT(countSpy, 2, TestTimeout::shortMs());
}

#include "UnitTestAsyncHelpersTest.moc"

UT_REGISTER_TEST(UnitTestAsyncHelpersTest, TestLabel::Unit)
