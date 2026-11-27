#include "SignalHandlerTest.h"

#include <csignal>
#include <memory>

#include <QtTest/QSignalSpy>

#include "SignalHandler.h"
#include "UnitTest.h"

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

void SignalHandlerTest::_testTerminateIsSignalled()
{
#ifdef Q_OS_WIN
    QSKIP("SIGTERM is handled on POSIX only");
#else
    SignalHandler handler;
    QCOMPARE(handler.setupSignalHandlers(), 0);
    QSignalSpy terminateSpy(&handler, &SignalHandler::terminateRequested);

    // With the signal connected the handler leaves the application alone: its receiver closes it.
    QCOMPARE(::raise(SIGTERM), 0);
    QVERIFY(terminateSpy.wait(TestTimeout::shortMs()));
    QCOMPARE(terminateSpy.count(), 1);
#endif
}

UT_REGISTER_TEST(SignalHandlerTest, TestLabel::Unit, TestLabel::Utilities)
