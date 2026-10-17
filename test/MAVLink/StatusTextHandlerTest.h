#pragma once

#include "UnitTest.h"

class StatusTextHandlerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGetMessageText();
    void _testHandleTextMessage();
    void _testHandleErrorMessageAndMultiComponentPrefix();
    void _testUnreadCountsBySeverity_data();
    void _testUnreadCountsBySeverity();
    void _testNewMessagesAreUnread();
    void _testMarkAllRead();
    void _testMarkAllReadThenNewMessage();
    void _testClearMessagesEmitsFormattedMessagesChanged();
    void _testChunkedStatusTextReassembly_data();
    void _testChunkedStatusTextReassembly();
    void _testChunkedStatusTextLateChunkStartsNewSequence();
    void _testChunkedStatusTextTimeoutAddsEllipsis();
    void _testChunkedStatusTextResetsWhenChunkIdChanges();
    void _testMavlinkMessageReceivedIgnoresNonStatusText();
};
