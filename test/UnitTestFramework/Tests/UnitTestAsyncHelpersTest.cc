#include "UnitTestAsyncHelpersTest.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtTest/QSignalSpy>

#include "LogManager.h"

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

    expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("Timeout waiting for signal")));
    QVERIFY(!UnitTest::waitForSignal(spy, 50, QStringLiteral("ping")));
    verifyExpectedLogMessage();
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
    expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(QStringLiteral("Timeout waiting for condition")));
    QVERIFY(!UnitTest::waitForCondition([&ready]() { return ready; }, 50, QStringLiteral("ready")));
    verifyExpectedLogMessage();
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

void UnitTestAsyncHelpersTest::_testFailedWaitLogsContext_data()
{
    QTest::addColumn<QString>("helper");
    QTest::addColumn<QString>("warning");

    QTest::newRow("waitForSignal") << "waitForSignal" << "Timeout waiting for signal \"ping\"";
    QTest::newRow("waitForNoSignal") << "waitForNoSignal" << "Unexpected signal \"ping\"";
    // A part of the count is there when the wait begins
    QTest::newRow("waitForSignalCount") << "waitForSignalCount"
                                        << "Timeout waiting for signal count \"ping\".*expected: 2 , actual: 1";
    QTest::newRow("waitForCondition") << "waitForCondition" << "Timeout waiting for condition \"never\"";
    QTest::newRow("waitForDeleted") << "waitForDeleted" << "Timeout waiting for QObject deletion \"emitter\"";
}

void UnitTestAsyncHelpersTest::_testFailedWaitLogsContext()
{
    QFETCH(QString, helper);
    QFETCH(QString, warning);

    AsyncSignalEmitter emitter;
    QSignalSpy spy(&emitter, &AsyncSignalEmitter::ping);
    // The debug messages are kept for the rest of the class
    const auto clearDebugMessages = qScopeGuard([] { TestDebug::clearMessages(); });

    TEST_CONTEXT(QStringLiteral("outer"));
    TEST_CONTEXT(QStringLiteral("inner"));
    TEST_DEBUG(QStringLiteral("what the test was doing"));
    const QStringList lines{warning, QStringLiteral("^Context: \"outer > inner\""),
                            QStringLiteral("^Recent test debug messages:"),
                            QStringLiteral("\"what the test was doing\"")};
    for (const QString& line : lines) {
        expectLogMessage("Test.UnitTest", QtWarningMsg, QRegularExpression(line));
    }

    constexpr int timeoutMs = 50;
    if (helper == "waitForSignal") {
        QVERIFY(!UnitTest::waitForSignal(spy, timeoutMs, QStringLiteral("ping")));
    } else if (helper == "waitForNoSignal") {
        QTimer::singleShot(0, &emitter, [&emitter]() { emit emitter.ping(1); });
        QVERIFY(!UnitTest::waitForNoSignal(spy, TestTimeout::shortMs(), QStringLiteral("ping")));
    } else if (helper == "waitForSignalCount") {
        emit emitter.ping(1);
        QVERIFY(!UnitTest::waitForSignalCount(spy, 2, timeoutMs, QStringLiteral("ping")));
    } else if (helper == "waitForCondition") {
        QVERIFY(!UnitTest::waitForCondition([] { return false; }, timeoutMs, QStringLiteral("never")));
    } else {
        QVERIFY(!UnitTest::waitForDeleted(QPointer<QObject>(&emitter), timeoutMs, QStringLiteral("emitter")));
    }

    // The expectations take the lines in any order: these are the warnings as they were logged
    const QList<LogEntry> captured = LogManager::capturedMessages(QStringLiteral("Test.UnitTest"));
    QStringList logged;
    for (const LogEntry& entry : captured) {
        if (entry.level == LogEntry::Warning) {
            logged.append(entry.message);
        }
    }
    QCOMPARE(logged.size(), lines.size());
    for (qsizetype i = 0; i < lines.size(); ++i) {
        QVERIFY2(logged.at(i).contains(QRegularExpression(lines.at(i))), qPrintable(logged.at(i)));
    }

    for (qsizetype i = 0; i < lines.size(); ++i) {
        verifyExpectedLogMessage();
    }
}

#include "UnitTestAsyncHelpersTest.moc"

UT_REGISTER_TEST(UnitTestAsyncHelpersTest, TestLabel::Unit)
