#include "StatusTextHandlerTest.h"

#include "MAVLinkLib.h"
#include "StatusTextHandler.h"

#include <cstring>
#include <QtCore/QMetaObject>
#include <QtTest/QSignalSpy>

namespace {

mavlink_message_t _makeStatusTextMessage(MAV_COMPONENT compId, MAV_SEVERITY severity, const QByteArray &text, uint16_t id, uint8_t chunkSeq)
{
    char statusText[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN] = {};
    const int copyLength = qMin<int>(text.size(), MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN);
    if (copyLength > 0) {
        std::memcpy(statusText, text.constData(), static_cast<size_t>(copyLength));
    }

    mavlink_message_t message{};
    (void) mavlink_msg_statustext_pack(255, static_cast<uint8_t>(compId), &message, severity, statusText, id, chunkSeq);
    return message;
}

mavlink_message_t _makeRawStatusTextMessage(MAV_COMPONENT compId, MAV_SEVERITY severity, const QByteArray &text, uint16_t id, uint8_t chunkSeq)
{
    mavlink_statustext_t statusText{};
    statusText.severity = severity;
    statusText.id = id;
    statusText.chunk_seq = chunkSeq;

    const int copyLength = qMin<int>(text.size(), MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN);
    if (copyLength > 0) {
        std::memcpy(statusText.text, text.constData(), static_cast<size_t>(copyLength));
    }

    mavlink_message_t message{};
    (void) mavlink_msg_statustext_encode(255, static_cast<uint8_t>(compId), &message, &statusText);
    return message;
}

} // namespace

void StatusTextHandlerTest::_testGetMessageText()
{
    const mavlink_message_t message = _makeStatusTextMessage(MAV_COMP_ID_USER1,
                                                             MAV_SEVERITY_INFO,
                                                             QByteArrayLiteral("StatusTextHandlerTest"),
                                                             0,
                                                             0);
    const QString messageText = StatusTextHandler::getMessageText(message);
    QCOMPARE(messageText, "StatusTextHandlerTest");
}

void StatusTextHandlerTest::_testHandleTextMessage()
{
    StatusTextHandler* statusTextHandler = new StatusTextHandler(this);
    statusTextHandler->handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "StatusTextHandlerTestInfo",
                                                    "This is the StatusTextHandlerTestInfo Test");
    QString messages = statusTextHandler->formattedMessages();
    QVERIFY(!messages.isEmpty());
    QVERIFY(messages.contains("StatusTextHandlerTestInfo"));
    QCOMPARE(statusTextHandler->messages().count(), 1);
    statusTextHandler->handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_WARNING,
                                                    "StatusTextHandlerTestWarning",
                                                    "This is the StatusTextHandlerTestWarning Test");
    messages = statusTextHandler->formattedMessages();
    QVERIFY(messages.contains("StatusTextHandlerTestInfo"));
    QVERIFY(messages.contains("StatusTextHandlerTestWarning"));
    QCOMPARE(statusTextHandler->messages().count(), 2);
    statusTextHandler->clearMessages();
    messages = statusTextHandler->formattedMessages();
    QVERIFY(messages.isEmpty());
    QCOMPARE(statusTextHandler->messages().count(), 0);
}

void StatusTextHandlerTest::_testHandleErrorMessageAndMultiComponentPrefix()
{
    StatusTextHandler statusTextHandler;

    QSignalSpy errorSpy(&statusTextHandler, &StatusTextHandler::newErrorMessage);
    QVERIFY(errorSpy.isValid());

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "InfoMessage", "");
    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER2, MAV_SEVERITY_ERROR, "ErrorMessage", "");

    QCOMPARE(statusTextHandler.messages().count(), 2);
    QCOMPARE(statusTextHandler.unreadCriticalMessageCount(), 1);

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toString(), QStringLiteral("ErrorMessage"));

    const QString formatted = statusTextHandler.formattedMessages();
    QVERIFY(formatted.contains(QStringLiteral("COMP:%1").arg(MAV_COMP_ID_USER2)));
    QVERIFY(formatted.contains(QStringLiteral("Error")));
}

void StatusTextHandlerTest::_testUnreadCriticalMessageCount()
{
    StatusTextHandler statusTextHandler;

    QSignalSpy countSpy(&statusTextHandler, &StatusTextHandler::unreadCriticalMessageCountChanged);
    QVERIFY(countSpy.isValid());

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "InfoMessage", "");
    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_WARNING, "WarningMessage", "");
    QCOMPARE(statusTextHandler.unreadCriticalMessageCount(), 0);
    QCOMPARE(countSpy.count(), 0);

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_ERROR, "ErrorMessage", "");
    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_CRITICAL, "CriticalMessage", "");
    QCOMPARE(statusTextHandler.unreadCriticalMessageCount(), 2);
    QCOMPARE(countSpy.count(), 2);

    statusTextHandler.clearMessages();
    QCOMPARE(statusTextHandler.unreadCriticalMessageCount(), 0);
    QCOMPARE(countSpy.count(), 3);
}

void StatusTextHandlerTest::_testNewMessagesAreUnread()
{
    StatusTextHandler statusTextHandler;

    QSignalSpy unreadSpy(&statusTextHandler, &StatusTextHandler::hasUnreadMessagesChanged);
    QSignalSpy unreadCountSpy(&statusTextHandler, &StatusTextHandler::unreadMessageCountChanged);
    QSignalSpy newMessageSpy(&statusTextHandler, &StatusTextHandler::newFormattedMessage);
    QVERIFY(unreadSpy.isValid());
    QVERIFY(unreadCountSpy.isValid());
    QVERIFY(newMessageSpy.isValid());
    QVERIFY(!statusTextHandler.hasUnreadMessages());
    QCOMPARE(statusTextHandler.unreadMessageCount(), 0);

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "InfoMessage", "");
    QVERIFY(statusTextHandler.hasUnreadMessages());
    QCOMPARE(statusTextHandler.unreadMessageCount(), 1);
    QCOMPARE(unreadSpy.count(), 1);
    QCOMPARE(unreadCountSpy.count(), 1);
    QVERIFY(statusTextHandler.formattedMessages().contains(QStringLiteral("<b>")));
    QCOMPARE(newMessageSpy.count(), 1);
    QVERIFY(newMessageSpy.at(0).at(0).toString().contains(QStringLiteral("<b>")));

    // Already unread, so no further transition, but the count still changes
    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_WARNING, "WarningMessage", "");
    QCOMPARE(unreadSpy.count(), 1);
    QCOMPARE(statusTextHandler.unreadMessageCount(), 2);
    QCOMPARE(unreadCountSpy.count(), 2);

    // The total includes critical messages
    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_ERROR, "ErrorMessage", "");
    QCOMPARE(statusTextHandler.unreadMessageCount(), 3);
    QCOMPARE(statusTextHandler.unreadCriticalMessageCount(), 1);

    statusTextHandler.markAllRead();
    QCOMPARE(statusTextHandler.unreadMessageCount(), 0);

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "InfoMessage2", "");
    statusTextHandler.clearMessages();
    QVERIFY(!statusTextHandler.hasUnreadMessages());
    QCOMPARE(statusTextHandler.unreadMessageCount(), 0);
    QCOMPARE(unreadSpy.count(), 4);
}

void StatusTextHandlerTest::_testMarkAllRead()
{
    StatusTextHandler statusTextHandler;

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "InfoMessage", "");
    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_ERROR, "ErrorMessage", "");
    QCOMPARE(statusTextHandler.unreadCriticalMessageCount(), 1);

    QSignalSpy countSpy(&statusTextHandler, &StatusTextHandler::unreadCriticalMessageCountChanged);
    QSignalSpy unreadSpy(&statusTextHandler, &StatusTextHandler::hasUnreadMessagesChanged);
    QSignalSpy formattedSpy(&statusTextHandler, &StatusTextHandler::formattedMessagesChanged);
    QVERIFY(countSpy.isValid());
    QVERIFY(unreadSpy.isValid());
    QVERIFY(formattedSpy.isValid());

    statusTextHandler.markAllRead();

    QCOMPARE(statusTextHandler.unreadCriticalMessageCount(), 0);
    QVERIFY(!statusTextHandler.hasUnreadMessages());
    QCOMPARE(statusTextHandler.messages().count(), 2);
    QVERIFY(!statusTextHandler.formattedMessages().contains(QStringLiteral("<b>")));
    QVERIFY(statusTextHandler.formattedMessages().contains(QStringLiteral("ErrorMessage")));
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(unreadSpy.count(), 1);
    QCOMPARE(formattedSpy.count(), 1);

    // Nothing left to mark, so no further signals
    statusTextHandler.markAllRead();
    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(unreadSpy.count(), 1);
    QCOMPARE(formattedSpy.count(), 1);
}

void StatusTextHandlerTest::_testMarkAllReadThenNewMessage()
{
    StatusTextHandler statusTextHandler;

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_ERROR, "OldError", "");
    statusTextHandler.markAllRead();

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_ERROR, "NewError", "");

    QCOMPARE(statusTextHandler.unreadCriticalMessageCount(), 1);
    QVERIFY(statusTextHandler.hasUnreadMessages());
    // Only the new message is bold
    QCOMPARE(statusTextHandler.formattedMessages().count(QStringLiteral("<b>")), 1);
}

void StatusTextHandlerTest::_testClearMessagesEmitsFormattedMessagesChanged()
{
    StatusTextHandler statusTextHandler;

    QSignalSpy formattedSpy(&statusTextHandler, &StatusTextHandler::formattedMessagesChanged);
    QVERIFY(formattedSpy.isValid());

    // Nothing to clear, so no signal
    statusTextHandler.clearMessages();
    QCOMPARE(formattedSpy.count(), 0);

    statusTextHandler.handleHTMLEscapedTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "InfoMessage", "");
    QCOMPARE(formattedSpy.count(), 0);

    statusTextHandler.clearMessages();
    QCOMPARE(formattedSpy.count(), 1);
    QVERIFY(statusTextHandler.formattedMessages().isEmpty());
}

void StatusTextHandlerTest::_testChunkedStatusTextMissingChunk()
{
    StatusTextHandler statusTextHandler;

    QSignalSpy textSpy(&statusTextHandler, &StatusTextHandler::textMessageReceived);
    QVERIFY(textSpy.isValid());

    const QByteArray chunk0(MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN, 'A');
    statusTextHandler.mavlinkMessageReceived(_makeStatusTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_WARNING, chunk0, 42, 0));

    const QByteArray chunk2("TAIL");
    statusTextHandler.mavlinkMessageReceived(_makeStatusTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_WARNING, chunk2, 42, 2));

    QCOMPARE(textSpy.count(), 1);
    const QList<QVariant> args = textSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), static_cast<int>(MAV_COMP_ID_USER1));
    QCOMPARE(args.at(1).toInt(), static_cast<int>(MAV_SEVERITY_WARNING));

    const QString text = args.at(2).toString();
    QCOMPARE(text.length(), MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN + QStringLiteral(" ... ").length() + chunk2.length());
    QVERIFY(text.contains(QStringLiteral(" ... ")));
    QVERIFY(text.endsWith(QStringLiteral("TAIL")));
}

void StatusTextHandlerTest::_testChunkedStatusTextTimeoutAddsEllipsis()
{
    StatusTextHandler statusTextHandler;

    QSignalSpy textSpy(&statusTextHandler, &StatusTextHandler::textMessageReceived);
    QVERIFY(textSpy.isValid());

    const QByteArray chunk0(MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN, 'B');
    statusTextHandler.mavlinkMessageReceived(_makeStatusTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, chunk0, 7, 0));

    const bool invoked = QMetaObject::invokeMethod(&statusTextHandler, "_chunkedStatusTextTimeout", Qt::DirectConnection);
    QVERIFY(invoked);

    QCOMPARE(textSpy.count(), 1);
    const QString text = textSpy.takeFirst().at(2).toString();
    QCOMPARE(text.length(), MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN + QStringLiteral(" ... ").length());
    QVERIFY(text.endsWith(QStringLiteral(" ... ")));
}

void StatusTextHandlerTest::_testChunkedStatusTextResetsWhenChunkIdChanges()
{
    StatusTextHandler statusTextHandler;

    QSignalSpy textSpy(&statusTextHandler, &StatusTextHandler::textMessageReceived);
    QVERIFY(textSpy.isValid());

    const QByteArray chunk0(MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN, 'C');
    statusTextHandler.mavlinkMessageReceived(
        _makeRawStatusTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_WARNING, chunk0, 10, 0));

    statusTextHandler.mavlinkMessageReceived(
        _makeStatusTextMessage(MAV_COMP_ID_USER1, MAV_SEVERITY_INFO, "NEW-ID", 11, 0));

    QCOMPARE(textSpy.count(), 2);

    const QString firstText = textSpy.takeFirst().at(2).toString();
    QVERIFY(firstText != QStringLiteral("NEW-ID"));
    QVERIFY(firstText.startsWith(QStringLiteral("CCCC")));

    const QList<QVariant> secondArgs = textSpy.takeFirst();
    QCOMPARE(secondArgs.at(1).toInt(), static_cast<int>(MAV_SEVERITY_INFO));
    QCOMPARE(secondArgs.at(2).toString(), QStringLiteral("NEW-ID"));
}

void StatusTextHandlerTest::_testMavlinkMessageReceivedIgnoresNonStatusText()
{
    StatusTextHandler statusTextHandler;

    QSignalSpy textSpy(&statusTextHandler, &StatusTextHandler::textMessageReceived);
    QVERIFY(textSpy.isValid());

    mavlink_message_t heartbeat{};
    heartbeat.msgid = MAVLINK_MSG_ID_HEARTBEAT;
    heartbeat.compid = MAV_COMP_ID_USER1;

    statusTextHandler.mavlinkMessageReceived(heartbeat);

    QCOMPARE(textSpy.count(), 0);
    QVERIFY(statusTextHandler.messages().isEmpty());
}

UT_REGISTER_TEST(StatusTextHandlerTest, TestLabel::Unit)
