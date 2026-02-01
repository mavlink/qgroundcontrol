#include "SignalHandlerTest.h"

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
    SignalHandler* handler1 = new SignalHandler();
    QCOMPARE(SignalHandler::current(), handler1);
    // Creating a second handler should update current
    SignalHandler* handler2 = new SignalHandler();
    QCOMPARE(SignalHandler::current(), handler2);
    // Delete in reverse order
    delete handler2;
    // Note: current() is now null because handler2's destructor cleared it
    // This is expected behavior - the last handler to be destroyed clears current
    QVERIFY(SignalHandler::current() == nullptr);
    delete handler1;
    QVERIFY(SignalHandler::current() == nullptr);
}
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#endif

UT_REGISTER_TEST(SignalHandlerTest, TestLabel::Unit, TestLabel::Utilities)
