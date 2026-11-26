#include "UnitTestBase.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEvent>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QSet>
#include <QtTest/QSignalSpy>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UnitTestLog, "Test.UnitTest")

struct UnitTestBase::ExpectedLogMessages
{
    struct Entry
    {
        QtMsgType type;
        QRegularExpression pattern;
        QString category;

        bool matches(const TestLogMessage& message) const
        {
            return (message.type == type) && (message.category == category) &&
                   pattern.match(message.message).hasMatch();
        }
    };

    struct PendingExpectation
    {
        Entry entry;
        qsizetype startIndex = 0;
    };

    QList<Entry> ignored;
    QList<PendingExpectation> pending;
    QSet<qsizetype> consumedMessageIndices;
};

// ============================================================================
// TestContext Implementation
// ============================================================================

QStringList& TestContext::stack()
{
    // QTest runs test functions sequentially on the main thread — no TLS needed.
    static QStringList s_stack;
    return s_stack;
}

TestContext::TestContext(const QString& context)
{
    push(context);
}

TestContext::~TestContext()
{
    pop();
}

QString TestContext::current()
{
    const QStringList& s = stack();
    if (s.isEmpty()) {
        return QString();
    }
    return s.join(QStringLiteral(" > "));
}

void TestContext::push(const QString& context)
{
    stack().append(context);
}

void TestContext::pop()
{
    if (!stack().isEmpty()) {
        stack().removeLast();
    }
}

// ============================================================================
// TestDebug Implementation
// ============================================================================

QStringList& TestDebug::messages()
{
    static QStringList s_messages;
    return s_messages;
}

bool& TestDebug::verbose()
{
    static bool s_verbose = qEnvironmentVariableIsSet("QGC_TEST_VERBOSE");
    return s_verbose;
}

void TestDebug::log(const QString& message)
{
    messages().append(message);
    while (messages().size() > kMaxBufferedMessages) {
        messages().removeFirst();
    }

    if (isVerbose()) {
        qCDebug(UnitTestLog) << message;
    }
}

void TestDebug::setVerbose(bool v)
{
    verbose() = v;
}

bool TestDebug::isVerbose()
{
    return verbose();
}

QStringList TestDebug::recentMessages()
{
    return messages();
}

void TestDebug::clearMessages()
{
    messages().clear();
}

namespace {

constexpr int kMaxTimeoutDebugMessages = 10;

QString currentTestName()
{
    const char* functionName = QTest::currentTestFunction();
    const char* dataTag = QTest::currentDataTag();

    QString testName = functionName ? QString::fromLatin1(functionName) : QStringLiteral("<unknown>");
    if (dataTag && dataTag[0] != '\0') {
        testName += QStringLiteral(" [") + QString::fromLatin1(dataTag) + QStringLiteral("]");
    }

    return testName;
}

void logRecentDebugMessages(const char* header)
{
    const QStringList debugMessages = TestDebug::recentMessages();
    if (debugMessages.isEmpty()) {
        return;
    }

    qCWarning(UnitTestLog) << header;

    const int startIndex = qMax(0, debugMessages.size() - kMaxTimeoutDebugMessages);
    for (int i = startIndex; i < debugMessages.size(); ++i) {
        qCWarning(UnitTestLog) << "  " << debugMessages.at(i);
    }
}

}  // anonymous namespace

// ============================================================================
// UnitTestBase Implementation
// ============================================================================

UnitTestBase::UnitTestBase(QObject* parent)
    : QObject(parent)
    , _expectedLogMessages(std::make_unique<ExpectedLogMessages>())
{}

UnitTestBase::~UnitTestBase() = default;

bool UnitTestBase::waitForSignal(QSignalSpy& spy, std::chrono::milliseconds timeout, QStringView signalName)
{
    QElapsedTimer waitTimer;
    waitTimer.start();

    if (spy.wait(timeout)) {
        return true;
    }

    const QString displayName = signalName.isEmpty() ? QStringLiteral("<unnamed>") : signalName.toString();
    qCWarning(UnitTestLog) << "Timeout waiting for signal" << displayName << "in" << currentTestName() << "after"
                           << waitTimer.elapsed() << "ms (timeout:" << timeout.count() << "ms, count:" << spy.count()
                           << ")";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");

    return false;
}

bool UnitTestBase::waitForNoSignal(QSignalSpy& spy, std::chrono::milliseconds timeout, QStringView signalName)
{
    const int initialCount = spy.count();
    QElapsedTimer waitTimer;
    waitTimer.start();

    const bool signalReceived = QTest::qWaitFor([&spy, initialCount]() { return spy.count() > initialCount; }, timeout);
    if (!signalReceived) {
        return true;
    }

    const QString displayName = signalName.isEmpty() ? QStringLiteral("<unnamed>") : signalName.toString();
    qCWarning(UnitTestLog) << "Unexpected signal" << displayName << "in" << currentTestName() << "after"
                           << waitTimer.elapsed() << "ms (timeout:" << timeout.count() << "ms, initial:" << initialCount
                           << ", current:" << spy.count() << ")";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");
    return false;
}

bool UnitTestBase::waitForSignalCount(QSignalSpy& spy, int expectedCount, std::chrono::milliseconds timeout,
                                      QStringView signalName)
{
    if (expectedCount <= 0 || spy.count() >= expectedCount) {
        return true;
    }

    QElapsedTimer waitTimer;
    waitTimer.start();

    // QSignalSpy::wait spins an event loop; QTest::qWaitFor sleeps 10 ms between event passes,
    // which throttles multi-message exchanges such as MockLink FTP transfers.
    while (spy.count() < expectedCount) {
        const std::chrono::milliseconds remaining = timeout - std::chrono::milliseconds(waitTimer.elapsed());
        if ((remaining <= std::chrono::milliseconds::zero()) || !spy.wait(remaining)) {
            break;
        }
    }
    if (spy.count() >= expectedCount) {
        return true;
    }

    const QString displayName = signalName.isEmpty() ? QStringLiteral("<unnamed>") : signalName.toString();
    qCWarning(UnitTestLog) << "Timeout waiting for signal count" << displayName << "in" << currentTestName() << "after"
                           << waitTimer.elapsed() << "ms (timeout:" << timeout.count()
                           << "ms, expected:" << expectedCount << ", actual:" << spy.count() << ")";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");

    return false;
}

bool UnitTestBase::waitForCondition(const std::function<bool()>& condition, std::chrono::milliseconds timeout,
                                    QStringView conditionName)
{
    QElapsedTimer waitTimer;
    waitTimer.start();

    if (QTest::qWaitFor(condition, timeout)) {
        return true;
    }

    const QString displayName = conditionName.isEmpty() ? QStringLiteral("<unnamed>") : conditionName.toString();
    qCWarning(UnitTestLog) << "Timeout waiting for condition" << displayName << "in" << currentTestName() << "after"
                           << waitTimer.elapsed() << "ms (timeout:" << timeout.count() << "ms)";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");

    return false;
}

bool UnitTestBase::waitForDeleted(const QPointer<QObject>& objectPtr, std::chrono::milliseconds timeout,
                                  QStringView objectName)
{
    if (objectPtr.isNull()) {
        return true;
    }

    if (timeout <= std::chrono::milliseconds::zero()) {
        timeout = std::chrono::milliseconds(TestTimeout::mediumMs());
    }

    QElapsedTimer waitTimer;
    waitTimer.start();

    const bool deleted = QTest::qWaitFor(
        [&objectPtr]() {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            return objectPtr.isNull();
        },
        timeout);
    if (deleted) {
        return true;
    }

    const QString displayName = objectName.isEmpty() ? QStringLiteral("<unnamed>") : objectName.toString();
    qCWarning(UnitTestLog) << "Timeout waiting for QObject deletion" << displayName << "in" << currentTestName()
                           << "after" << waitTimer.elapsed() << "ms (timeout:" << timeout.count()
                           << "ms, ptr:" << objectPtr.data() << ")";

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    logRecentDebugMessages("Recent test debug messages:");

    return false;
}

void UnitTestBase::settleEventLoopForCleanup(int iterations, int waitMs)
{
    if (iterations <= 0) {
        iterations = TestTimeout::isCI() ? 6 : 3;
    }
    if (waitMs < 0) {
        waitMs = TestTimeout::isCI() ? 10 : 5;
    }

    for (int i = 0; i < iterations; ++i) {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        if (i < iterations - 1 && waitMs > 0) {
            QTest::qWait(waitMs);
        }
    }
}

void UnitTestBase::expectLogMessage(const char* category, QtMsgType type, const QRegularExpression& pattern)
{
    Q_ASSERT_X(category && *category != '\0', "expectLogMessage",
               "category must not be empty — use the exact Qt logging category string (e.g. "
               "\"Utilities.QGCFileHelper\")");
    _expectedLogMessages->pending.append({
        .entry = {type, pattern, QString::fromLatin1(category)},
        .startIndex = capturedLogMessages().size(),
    });
}

void UnitTestBase::verifyExpectedLogMessage()
{
    if (_expectedLogMessages->pending.isEmpty()) {
        QFAIL("verifyExpectedLogMessage called with no pending expectLogMessage.");
    }

    const ExpectedLogMessages::PendingExpectation expected = _expectedLogMessages->pending.takeFirst();

    const QList<TestLogMessage> allMsgs = capturedLogMessages();
    for (qsizetype i = expected.startIndex; i < allMsgs.size(); ++i) {
        if (!_expectedLogMessages->consumedMessageIndices.contains(i) && expected.entry.matches(allMsgs[i])) {
            _expectedLogMessages->consumedMessageIndices.insert(i);
            return;
        }
    }

    const QString detail =
        QStringLiteral("Expected log message was not captured after expectLogMessage. pattern='%1' category='%2'")
            .arg(expected.entry.pattern.pattern(), expected.entry.category);
    QFAIL(qPrintable(detail));
}

void UnitTestBase::ignoreLogMessage(const char* category, QtMsgType type, const QRegularExpression& pattern)
{
    Q_ASSERT_X(category && *category != '\0', "ignoreLogMessage",
               "category must not be empty — use the exact Qt logging category string (e.g. "
               "\"Utilities.QGCFileHelper\")");
    _expectedLogMessages->ignored.append({type, pattern, QString::fromLatin1(category)});
}

void UnitTestBase::init()
{
    _failureContextDumped = false;
    _expectedLogMessages->ignored.clear();
    _expectedLogMessages->pending.clear();
    _expectedLogMessages->consumedMessageIndices.clear();

    startLogCapture();
}

void UnitTestBase::cleanup()
{
    dumpFailureContextIfTestFailed(QStringLiteral("cleanup"));
    // Again on the way out, for a failure the checks below raise: QFAIL returns from here.
    const auto dumpGuard = qScopeGuard([this] { dumpFailureContextIfTestFailed(QStringLiteral("cleanup")); });

    // Keep log capture enabled through the settle/verification phase so warnings
    // emitted while draining queued events are still subject to strict checking.
    // The scope guard guarantees capture is disabled on every exit path, including
    // the early returns taken by the QFAIL macros below.
    const auto captureGuard = qScopeGuard([this] { stopLogCapture(); });

    // Drain queued events and flush DeferredDelete across several passes to
    // prevent cross-test contamination; a single qWait(0) pass is insufficient.
    settleEventLoopForCleanup();

    // Fail the test if any unexpected captured log messages were emitted (strict mode).
    // Skip if the test already failed to avoid noisy double-failure reports.
    if (QTest::currentTestFailed()) {
        return;
    }

    if (!_expectedLogMessages->pending.isEmpty()) {
        const auto expected = _expectedLogMessages->pending.front().entry;
        _expectedLogMessages->pending.clear();
        const QString detail =
            QStringLiteral(
                "expectLogMessage was called without a matching verifyExpectedLogMessage. pattern='%1' category='%2'")
                .arg(expected.pattern.pattern(), expected.category);
        QFAIL(qPrintable(detail));
    }

    const auto isIgnored = [this](const TestLogMessage& message) {
        for (const auto& entry : std::as_const(_expectedLogMessages->ignored)) {
            if (entry.matches(message)) {
                return true;
            }
        }
        return false;
    };

    const QList<TestLogMessage> allMsgs = capturedLogMessages();
    QString strictDetails;
    for (qsizetype i = 0; i < allMsgs.size(); ++i) {
        const TestLogMessage& m = allMsgs[i];
        if (_expectedLogMessages->consumedMessageIndices.contains(i) || isIgnored(m)) {
            continue;
        }
        const char* lvl = (m.type == QtDebugMsg)      ? "debug"
                          : (m.type == QtWarningMsg)  ? "warning"
                          : (m.type == QtInfoMsg)     ? "info"
                          : (m.type == QtCriticalMsg) ? "critical"
                                                      : "other";
        strictDetails += QStringLiteral("  [%1][%2] %3\n").arg(QLatin1String(lvl), m.category, m.message);
    }

    if (!strictDetails.isEmpty()) {
        QFAIL(qPrintable(QStringLiteral("Unexpected log messages (strict mode):\n%1").arg(strictDetails)));
    }
}

void UnitTestBase::dumpFailureContextIfTestFailed(QStringView reason)
{
    if (_failureContextDumped || !QTest::currentTestFailed()) {
        return;
    }

    _failureContextDumped = true;

    const QString testFunction = currentTestName();
    if (reason.isEmpty()) {
        qCWarning(UnitTestLog) << "Failure context dump for" << objectName() << "::" << testFunction;
    } else {
        qCWarning(UnitTestLog) << "Failure context dump for" << objectName() << "::" << testFunction << "("
                               << reason.toString() << ")";
    }

    const QString context = TestContext::current();
    if (!context.isEmpty()) {
        qCWarning(UnitTestLog) << "Context:" << context;
    }

    const QString fixtureSummary = failureContextSummary();
    if (!fixtureSummary.isEmpty()) {
        qCWarning(UnitTestLog) << "Fixture state:";
        const QStringList lines = fixtureSummary.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            qCWarning(UnitTestLog).noquote() << "  " << line;
        }
    }

    logRecentDebugMessages("Recent test debug messages:");
}

QString UnitTestBase::failureContextSummary() const
{
    return {};
}
