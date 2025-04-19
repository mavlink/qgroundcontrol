/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "StatusTextHandlerTest.h"
#include "StatusTextHandler.h"
#include "MAVLinkLib.h"

#include <QtTest/QTest>

void StatusTextHandlerTest::_testGetMessageText()
{
    mavlink_message_t message;
    (void) mavlink_msg_statustext_pack(255, MAV_COMP_ID_USER1, &message, MAV_SEVERITY_INFO,"StatusTextHandlerTest", 0, 0);

    const QString messageText = StatusTextHandler::getMessageText(message);
    QCOMPARE(messageText, "StatusTextHandlerTest");
}

void StatusTextHandlerTest::_testHandleTextMessage()
{
    StatusTextHandler* statusTextHandler = new StatusTextHandler(this);

    statusTextHandler->handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "StatusTextHandlerTestInfo", "This is the StatusTextHandlerTestInfo Test");
    QString messages = statusTextHandler->formattedMessages();
    QVERIFY(!messages.isEmpty());
    QVERIFY(messages.contains("StatusTextHandlerTestInfo"));
    QCOMPARE(statusTextHandler->getNormalCount(), 1);
    QCOMPARE(statusTextHandler->messageCount(), 1);

    statusTextHandler->handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_WARNING, "StatusTextHandlerTestWarning", "This is the StatusTextHandlerTestWarning Test");
    messages = statusTextHandler->formattedMessages();
    QVERIFY(messages.contains("StatusTextHandlerTestInfo"));
    QVERIFY(messages.contains("StatusTextHandlerTestWarning"));
    QCOMPARE(statusTextHandler->getNormalCount(), 1);
    QCOMPARE(statusTextHandler->getWarningCount(), 1);
    QCOMPARE(statusTextHandler->messageCount(), 2);

    statusTextHandler->clearMessages();
    messages = statusTextHandler->formattedMessages();
    QVERIFY(messages.isEmpty());
    QCOMPARE(statusTextHandler->getNormalCount(), 0);
    QCOMPARE(statusTextHandler->getWarningCount(), 0);
    QCOMPARE(statusTextHandler->messageCount(), 0);
}
