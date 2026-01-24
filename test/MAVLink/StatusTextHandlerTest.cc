#include "StatusTextHandlerTest.h"
#include "StatusTextHandler.h"
#include "MAVLinkLib.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// StatusText Class Tests
// ============================================================================

void StatusTextHandlerTest::_testStatusTextConstruction()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Test message");

    QCOMPARE_EQ(status.getComponentID(), MAV_COMP_ID_AUTOPILOT1);
    QCOMPARE_EQ(status.getSeverity(), MAV_SEVERITY_INFO);
    QCOMPARE_EQ(status.getText(), QStringLiteral("Test message"));
    QVERIFY(status.getFormattedText().isEmpty());
}

void StatusTextHandlerTest::_testStatusTextSeverityIsErrorEmergency()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_EMERGENCY, "Emergency");
    QVERIFY(status.severityIsError());
}

void StatusTextHandlerTest::_testStatusTextSeverityIsErrorAlert()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ALERT, "Alert");
    QVERIFY(status.severityIsError());
}

void StatusTextHandlerTest::_testStatusTextSeverityIsErrorCritical()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_CRITICAL, "Critical");
    QVERIFY(status.severityIsError());
}

void StatusTextHandlerTest::_testStatusTextSeverityIsErrorError()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error");
    QVERIFY(status.severityIsError());
}

void StatusTextHandlerTest::_testStatusTextSeverityIsErrorWarning()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning");
    QVERIFY(!status.severityIsError());
}

void StatusTextHandlerTest::_testStatusTextSeverityIsErrorNotice()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_NOTICE, "Notice");
    QVERIFY(!status.severityIsError());
}

void StatusTextHandlerTest::_testStatusTextSeverityIsErrorInfo()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info");
    QVERIFY(!status.severityIsError());
}

void StatusTextHandlerTest::_testStatusTextSeverityIsErrorDebug()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_DEBUG, "Debug");
    QVERIFY(!status.severityIsError());
}

void StatusTextHandlerTest::_testStatusTextSetFormattedText()
{
    StatusText status(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Test");
    QVERIFY(status.getFormattedText().isEmpty());

    status.setFormatedText("<b>Formatted</b>");
    QCOMPARE_EQ(status.getFormattedText(), QStringLiteral("<b>Formatted</b>"));
}

// ============================================================================
// StatusTextHandler Construction Tests
// ============================================================================

void StatusTextHandlerTest::_testHandlerConstruction()
{
    StatusTextHandler handler(this);

    QCOMPARE_EQ(handler.messageCount(), 0u);
    QCOMPARE_EQ(handler.getErrorCount(), 0u);
    QCOMPARE_EQ(handler.getWarningCount(), 0u);
    QCOMPARE_EQ(handler.getNormalCount(), 0u);
    QVERIFY(handler.messageTypeNone());
    QVERIFY(handler.formattedMessages().isEmpty());
}

// ============================================================================
// getMessageText Tests
// ============================================================================

void StatusTextHandlerTest::_testGetMessageText()
{
    mavlink_message_t message;
    (void) mavlink_msg_statustext_pack(255, MAV_COMP_ID_USER1, &message, MAV_SEVERITY_INFO, "Test message", 0, 0);

    const QString text = StatusTextHandler::getMessageText(message);
    QCOMPARE_EQ(text, QStringLiteral("Test message"));
}

void StatusTextHandlerTest::_testGetMessageTextMaxLength()
{
    mavlink_message_t message;
    QString longText(MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN, 'A');
    (void) mavlink_msg_statustext_pack(255, MAV_COMP_ID_USER1, &message, MAV_SEVERITY_INFO, longText.toUtf8().constData(), 0, 0);

    const QString text = StatusTextHandler::getMessageText(message);
    QCOMPARE_EQ(text.length(), MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN);
}

void StatusTextHandlerTest::_testGetMessageTextEmpty()
{
    mavlink_message_t message;
    (void) mavlink_msg_statustext_pack(255, MAV_COMP_ID_USER1, &message, MAV_SEVERITY_INFO, "", 0, 0);

    const QString text = StatusTextHandler::getMessageText(message);
    QVERIFY(text.isEmpty());
}

// ============================================================================
// handleHTMLEscapedTextMessage Tests
// ============================================================================

void StatusTextHandlerTest::_testHandleInfoMessage()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info message", "");

    QCOMPARE_EQ(handler.getNormalCount(), 1u);
    QCOMPARE_EQ(handler.messageCount(), 1u);
    QVERIFY(handler.messageTypeNormal());
    QGC_VERIFY_STRING_CONTAINS(handler.formattedMessages(), QStringLiteral("Info message"));
}

void StatusTextHandlerTest::_testHandleWarningMessage()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning message", "");

    QCOMPARE_EQ(handler.getWarningCount(), 1u);
    QCOMPARE_EQ(handler.messageCount(), 1u);
    QVERIFY(handler.messageTypeWarning());
    QGC_VERIFY_STRING_CONTAINS(handler.formattedMessages(), QStringLiteral("Warning message"));
}

void StatusTextHandlerTest::_testHandleErrorMessage()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error message", "");

    QCOMPARE_EQ(handler.getErrorCount(), 1u);
    QCOMPARE_EQ(handler.messageCount(), 1u);
    QVERIFY(handler.messageTypeError());
    QGC_VERIFY_STRING_CONTAINS(handler.formattedMessages(), QStringLiteral("Error message"));
}

void StatusTextHandlerTest::_testHandleMultipleMessages()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "First", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Second", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Third", "");

    QCOMPARE_EQ(handler.getNormalCount(), 3u);
    QCOMPARE_EQ(handler.messageCount(), 3u);

    const QString formatted = handler.formattedMessages();
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("First"));
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("Second"));
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("Third"));
}

void StatusTextHandlerTest::_testHandleMessageWithDescription()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Title", "Description text");

    const QString formatted = handler.formattedMessages();
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("Title"));
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("Description text"));
}

void StatusTextHandlerTest::_testHandleMessageWithNewlines()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Line1\nLine2", "");

    const QString formatted = handler.formattedMessages();
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("<br/>"));
}

// ============================================================================
// Message Type Tests
// ============================================================================

void StatusTextHandlerTest::_testMessageTypeInitialState()
{
    StatusTextHandler handler(this);

    QVERIFY(handler.messageTypeNone());
    QVERIFY(!handler.messageTypeNormal());
    QVERIFY(!handler.messageTypeWarning());
    QVERIFY(!handler.messageTypeError());
}

void StatusTextHandlerTest::_testMessageTypeAfterNormal()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info", "");

    QVERIFY(!handler.messageTypeNone());
    QVERIFY(handler.messageTypeNormal());
    QVERIFY(!handler.messageTypeWarning());
    QVERIFY(!handler.messageTypeError());
}

void StatusTextHandlerTest::_testMessageTypeAfterWarning()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning", "");

    QVERIFY(!handler.messageTypeNone());
    QVERIFY(!handler.messageTypeNormal());
    QVERIFY(handler.messageTypeWarning());
    QVERIFY(!handler.messageTypeError());
}

void StatusTextHandlerTest::_testMessageTypeAfterError()
{
    StatusTextHandler handler(this);
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error", "");

    QVERIFY(!handler.messageTypeNone());
    QVERIFY(!handler.messageTypeNormal());
    QVERIFY(!handler.messageTypeWarning());
    QVERIFY(handler.messageTypeError());
}

void StatusTextHandlerTest::_testMessageTypePriority()
{
    StatusTextHandler handler(this);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info", "");
    QVERIFY(handler.messageTypeNormal());

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning", "");
    QVERIFY(handler.messageTypeWarning());

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error", "");
    QVERIFY(handler.messageTypeError());

    // Adding more info/warning shouldn't downgrade from error
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "More info", "");
    QVERIFY(handler.messageTypeError());
}

// ============================================================================
// Count Tests
// ============================================================================

void StatusTextHandlerTest::_testCountsInitialState()
{
    StatusTextHandler handler(this);

    QCOMPARE_EQ(handler.getErrorCount(), 0u);
    QCOMPARE_EQ(handler.getErrorCountTotal(), 0u);
    QCOMPARE_EQ(handler.getWarningCount(), 0u);
    QCOMPARE_EQ(handler.getNormalCount(), 0u);
    QCOMPARE_EQ(handler.messageCount(), 0u);
}

void StatusTextHandlerTest::_testCountsAfterMessages()
{
    StatusTextHandler handler(this);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info1", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info2", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error", "");

    QCOMPARE_EQ(handler.getNormalCount(), 2u);
    QCOMPARE_EQ(handler.getWarningCount(), 1u);
    QCOMPARE_EQ(handler.getErrorCount(), 1u);
    QCOMPARE_EQ(handler.messageCount(), 4u);
}

void StatusTextHandlerTest::_testErrorCountTotal()
{
    StatusTextHandler handler(this);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error1", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error2", "");
    QCOMPARE_EQ(handler.getErrorCountTotal(), 2u);

    handler.resetErrorLevelMessages();
    QCOMPARE_EQ(handler.getErrorCount(), 0u);
    QCOMPARE_EQ(handler.getErrorCountTotal(), 2u);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error3", "");
    QCOMPARE_EQ(handler.getErrorCountTotal(), 3u);
}

// ============================================================================
// Clear and Reset Tests
// ============================================================================

void StatusTextHandlerTest::_testClearMessages()
{
    StatusTextHandler handler(this);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error", "");

    handler.clearMessages();

    QCOMPARE_EQ(handler.messageCount(), 0u);
    QCOMPARE_EQ(handler.getNormalCount(), 0u);
    QCOMPARE_EQ(handler.getWarningCount(), 0u);
    QCOMPARE_EQ(handler.getErrorCount(), 0u);
    QVERIFY(handler.messageTypeNone());
    QVERIFY(handler.formattedMessages().isEmpty());
}

void StatusTextHandlerTest::_testResetAllMessages()
{
    StatusTextHandler handler(this);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error", "");

    handler.resetAllMessages();

    QCOMPARE_EQ(handler.messageCount(), 0u);
    QCOMPARE_EQ(handler.getErrorCount(), 0u);
    QVERIFY(handler.messageTypeNone());
    // Note: messages list is NOT cleared by resetAllMessages
}

void StatusTextHandlerTest::_testResetErrorLevelMessages()
{
    StatusTextHandler handler(this);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error", "");

    QVERIFY(handler.messageTypeError());

    handler.resetErrorLevelMessages();

    QCOMPARE_EQ(handler.getErrorCount(), 0u);
    QCOMPARE_EQ(handler.getWarningCount(), 1u);
    QCOMPARE_EQ(handler.getNormalCount(), 1u);
    QVERIFY(handler.messageTypeWarning());
}

// ============================================================================
// Signal Tests
// ============================================================================

void StatusTextHandlerTest::_testNewFormattedMessageSignal()
{
    StatusTextHandler handler(this);
    QSignalSpy spy(&handler, &StatusTextHandler::newFormattedMessage);
    QGC_VERIFY_SPY_VALID(spy);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Test", "");

    QCOMPARE_EQ(spy.count(), 1);
    const QString message = spy.first().at(0).toString();
    QGC_VERIFY_STRING_CONTAINS(message, QStringLiteral("Test"));
}

void StatusTextHandlerTest::_testMessageCountChangedSignal()
{
    StatusTextHandler handler(this);
    QSignalSpy spy(&handler, &StatusTextHandler::messageCountChanged);
    QGC_VERIFY_SPY_VALID(spy);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Test", "");

    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(spy.first().at(0).toUInt(), 1u);
}

void StatusTextHandlerTest::_testMessageTypeChangedSignal()
{
    StatusTextHandler handler(this);
    QSignalSpy spy(&handler, &StatusTextHandler::messageTypeChanged);
    QGC_VERIFY_SPY_VALID(spy);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info", "");
    QCOMPARE_EQ(spy.count(), 1);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning", "");
    QCOMPARE_EQ(spy.count(), 2);

    // Same level shouldn't trigger again
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_WARNING, "Warning2", "");
    QCOMPARE_EQ(spy.count(), 2);
}

void StatusTextHandlerTest::_testNewErrorMessageSignal()
{
    StatusTextHandler handler(this);
    QSignalSpy spy(&handler, &StatusTextHandler::newErrorMessage);
    QGC_VERIFY_SPY_VALID(spy);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "Info", "");
    QCOMPARE_EQ(spy.count(), 0);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_ERROR, "Error message", "");
    QCOMPARE_EQ(spy.count(), 1);
    QCOMPARE_EQ(spy.first().at(0).toString(), QStringLiteral("Error message"));
}

// ============================================================================
// MAVLink Message Handling Tests
// ============================================================================

void StatusTextHandlerTest::_testMavlinkMessageReceived()
{
    StatusTextHandler handler(this);
    QSignalSpy spy(&handler, &StatusTextHandler::textMessageReceived);
    QGC_VERIFY_SPY_VALID(spy);

    mavlink_message_t message;
    (void) mavlink_msg_statustext_pack(1, MAV_COMP_ID_AUTOPILOT1, &message, MAV_SEVERITY_INFO, "MAVLink test", 0, 0);

    handler.mavlinkMessageReceived(message);

    QVERIFY(spy.wait(TestHelpers::kDefaultTimeoutMs) || spy.count() > 0);
    QCOMPARE_GE(spy.count(), 1);
}

void StatusTextHandlerTest::_testMavlinkMessageReceivedNonStatusText()
{
    StatusTextHandler handler(this);
    QSignalSpy spy(&handler, &StatusTextHandler::textMessageReceived);
    QGC_VERIFY_SPY_VALID(spy);

    mavlink_message_t message;
    mavlink_heartbeat_t heartbeat = {0};
    (void) mavlink_msg_heartbeat_encode(1, MAV_COMP_ID_AUTOPILOT1, &message, &heartbeat);

    handler.mavlinkMessageReceived(message);

    // Should not process non-statustext messages
    QTest::qWait(100);
    QCOMPARE_EQ(spy.count(), 0);
}

// ============================================================================
// Multi-Component Tests
// ============================================================================

void StatusTextHandlerTest::_testMultipleComponents()
{
    StatusTextHandler handler(this);

    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_AUTOPILOT1, MAV_SEVERITY_INFO, "From Autopilot", "");
    handler.handleHTMLEscapedTextMessage(MAV_COMP_ID_CAMERA, MAV_SEVERITY_INFO, "From Camera", "");

    QCOMPARE_EQ(handler.messageCount(), 2u);

    const QString formatted = handler.formattedMessages();
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("From Autopilot"));
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("From Camera"));
    // Multi-component should show COMP: prefix
    QGC_VERIFY_STRING_CONTAINS(formatted, QStringLiteral("COMP:"));
}
