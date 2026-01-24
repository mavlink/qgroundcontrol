#pragma once

#include "TestFixtures.h"

/// Unit tests for StatusTextHandler and StatusText classes.
/// Tests message handling, severity classification, and chunked message assembly.
class StatusTextHandlerTest : public OfflineTest
{
    Q_OBJECT

public:
    StatusTextHandlerTest() = default;

private slots:
    // StatusText class tests
    void _testStatusTextConstruction();
    void _testStatusTextSeverityIsErrorEmergency();
    void _testStatusTextSeverityIsErrorAlert();
    void _testStatusTextSeverityIsErrorCritical();
    void _testStatusTextSeverityIsErrorError();
    void _testStatusTextSeverityIsErrorWarning();
    void _testStatusTextSeverityIsErrorNotice();
    void _testStatusTextSeverityIsErrorInfo();
    void _testStatusTextSeverityIsErrorDebug();
    void _testStatusTextSetFormattedText();

    // StatusTextHandler construction tests
    void _testHandlerConstruction();

    // getMessageText tests
    void _testGetMessageText();
    void _testGetMessageTextMaxLength();
    void _testGetMessageTextEmpty();

    // handleHTMLEscapedTextMessage tests
    void _testHandleInfoMessage();
    void _testHandleWarningMessage();
    void _testHandleErrorMessage();
    void _testHandleMultipleMessages();
    void _testHandleMessageWithDescription();
    void _testHandleMessageWithNewlines();

    // Message type tests
    void _testMessageTypeInitialState();
    void _testMessageTypeAfterNormal();
    void _testMessageTypeAfterWarning();
    void _testMessageTypeAfterError();
    void _testMessageTypePriority();

    // Count tests
    void _testCountsInitialState();
    void _testCountsAfterMessages();
    void _testErrorCountTotal();

    // Clear and reset tests
    void _testClearMessages();
    void _testResetAllMessages();
    void _testResetErrorLevelMessages();

    // Signal tests
    void _testNewFormattedMessageSignal();
    void _testMessageCountChangedSignal();
    void _testMessageTypeChangedSignal();
    void _testNewErrorMessageSignal();

    // MAVLink message handling tests
    void _testMavlinkMessageReceived();
    void _testMavlinkMessageReceivedNonStatusText();

    // Multi-component tests
    void _testMultipleComponents();
};
