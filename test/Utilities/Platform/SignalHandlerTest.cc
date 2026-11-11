#include "SignalHandlerTest.h"

#include <memory>

#include "SignalHandler.h"
#include "UnitTest.h"

#ifdef Q_OS_UNIX
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

namespace {

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
    // Save the original handler (app may have one set up)
    SignalHandler* originalHandler = SignalHandler::current();
    {
        SignalHandler handler;
        // New handler should become current
        QCOMPARE(SignalHandler::current(), &handler);
    }
    // After destruction, current should be cleared
    // (Note: this clears to nullptr, not back to original)
    QVERIFY(SignalHandler::current() == nullptr);
    Q_UNUSED(originalHandler);
}

void SignalHandlerTest::_testCurrentReturnsInstance()
{
    SignalHandler handler;
    // current() should return the handler we just created
    QCOMPARE(SignalHandler::current(), &handler);
}

void SignalHandlerTest::_testSetupReturnsSuccess()
{
    SignalHandler handler;
    // Setup should succeed (return 0)
    const int result = handler.setupSignalHandlers();
    QCOMPARE(result, 0);
}

void SignalHandlerTest::_testDestructorClearsCurrent()
{
    {
        SignalHandler handler;
        QVERIFY(SignalHandler::current() != nullptr);
    }
    // After handler goes out of scope, current should be null
    QVERIFY(SignalHandler::current() == nullptr);
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

void SignalHandlerTest::_testSecondSigIntExitsFromHandler()
{
#ifdef Q_OS_UNIX
    SignalHandler handler;
    QCOMPARE(handler.setupSignalHandlers(), 0);

    const int status = raiseSignalInForkedChild(SIGINT, 2);
    QVERIFY(status != -1);
    QVERIFY2(WIFEXITED(status), "The child was terminated by a signal");
    QCOMPARE(WEXITSTATUS(status), 128 + SIGINT);
#else
    QSKIP("POSIX signals only");
#endif
}

void SignalHandlerTest::_testRepeatedSigTermDoesNotBlockHandler()
{
#ifdef Q_OS_UNIX
    SignalHandler handler;
    QCOMPARE(handler.setupSignalHandlers(), 0);

    // Far more signals than the socket to the event loop can hold while nothing reads it
    const int status = raiseSignalInForkedChild(SIGTERM, 10000);
    QVERIFY(status != -1);
    QVERIFY2(WIFEXITED(status), "The signal handler blocked until the child was terminated");
    QVERIFY2(WEXITSTATUS(status) == EXIT_SUCCESS, "The signal handler changed errno");
#else
    QSKIP("POSIX signals only");
#endif
}

UT_REGISTER_TEST(SignalHandlerTest, TestLabel::Unit, TestLabel::Utilities)
