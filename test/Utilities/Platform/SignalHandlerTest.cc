#include "SignalHandlerTest.h"

#include <memory>

#include "SignalHandler.h"

#ifdef Q_OS_UNIX
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

namespace {

/// Expected exit code of a child that the signal terminates
constexpr int KILLED_BY_SIGNAL = -1;

/// Raises @a signum @a count times in a forked child and returns the child's wait status, or -1.
/// The child inherits the installed handlers but runs no event loop, so only the handlers themselves can react.
/// The bytes they write reach the parent's socket: destroy the SignalHandler before the event loop runs again.
int raiseSignalInForkedChild(int signum, int count)
{
    const auto timeoutSecs = static_cast<unsigned int>(TestTimeout::mediumDuration().count() / 1000);

    const pid_t pid = ::fork();
    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        // Forked from a multi-threaded process: async-signal-safe calls only.
        // SIGALRM ends the child if a handler blocks.
        (void) ::signal(SIGALRM, SIG_DFL);
        (void) ::alarm(timeoutSecs);
        errno = 0;
        for (int i = 0; i < count; ++i) {
            (void) ::raise(signum);
        }
        ::_exit((errno == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    int status = 0;
    pid_t waited = -1;
    do {
        waited = ::waitpid(pid, &status, 0);
    } while ((waited == -1) && (errno == EINTR));

    return (waited == pid) ? status : -1;
}

}  // namespace
#endif

void SignalHandlerTest::_testConstruction()
{
    {
        SignalHandler handler;
        // New handler should become current
        QCOMPARE(SignalHandler::current(), &handler);
    }
    // After destruction, current should be cleared
    // (Note: this clears to nullptr, not back to the handler the app may have set up)
    QVERIFY(SignalHandler::current() == nullptr);
}

void SignalHandlerTest::_testSetupReturnsSuccess()
{
    SignalHandler handler;
    // Setup should succeed (return 0)
    const int result = handler.setupSignalHandlers();
    QCOMPARE(result, 0);
}

void SignalHandlerTest::_testCurrentUpdatesWithNewHandler()
{
    auto handler1 = std::make_unique<SignalHandler>();
    QCOMPARE(SignalHandler::current(), handler1.get());
    // Creating a second handler should update current
    auto handler2 = std::make_unique<SignalHandler>();
    QCOMPARE(SignalHandler::current(), handler2.get());
    // Delete in reverse order
    handler2.reset();
    // Note: current() is now null because handler2's destructor cleared it
    // This is expected behavior - the last handler to be destroyed clears current
    QVERIFY(SignalHandler::current() == nullptr);
    handler1.reset();
    QVERIFY(SignalHandler::current() == nullptr);
}

void SignalHandlerTest::_testSignalInChild_data()
{
#ifdef Q_OS_UNIX
    QTest::addColumn<int>("signum");
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("handlerDestroyed");
    QTest::addColumn<int>("expectedExitCode");

    // The handler neither ends the child nor changes errno
    QTest::newRow("first SIGINT leaves the process running") << SIGINT << 1 << false << EXIT_SUCCESS;
    QTest::newRow("second SIGINT exits from the handler") << SIGINT << 2 << false << (128 + SIGINT);
    // The application object owns the handler, so the rest of the process exit runs without it
    QTest::newRow("SIGINT after destruction terminates") << SIGINT << 1 << true << KILLED_BY_SIGNAL;
    // Far more signals than the socket to the event loop can hold while nothing reads it. A handler that blocked
    // would be terminated by the child's alarm, and one that changed errno would make the child fail.
    QTest::newRow("repeated SIGTERM does not block the handler") << SIGTERM << 10000 << false << EXIT_SUCCESS;
#endif
}

void SignalHandlerTest::_testSignalInChild()
{
#ifdef Q_OS_UNIX
    QFETCH(int, signum);
    QFETCH(int, count);
    QFETCH(bool, handlerDestroyed);
    QFETCH(int, expectedExitCode);

    auto handler = std::make_unique<SignalHandler>();
    QCOMPARE(handler->setupSignalHandlers(), 0);
    if (handlerDestroyed) {
        handler.reset();
    }

    const int status = raiseSignalInForkedChild(signum, count);
    QVERIFY(status != -1);
    if (expectedExitCode == KILLED_BY_SIGNAL) {
        QVERIFY2(WIFSIGNALED(status), "The signal was swallowed");
        QCOMPARE(WTERMSIG(status), signum);
    } else {
        QVERIFY2(WIFEXITED(status), "The child was terminated by a signal");
        QCOMPARE(WEXITSTATUS(status), expectedExitCode);
    }
#else
    QSKIP("POSIX signals only");
#endif
}

UT_REGISTER_TEST(SignalHandlerTest, TestLabel::Unit, TestLabel::Utilities)
