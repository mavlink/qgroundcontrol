#pragma once

#include "UnitTest.h"

class StatusTextHandlerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGetMessageText();
    void _testHandleTextMessage();
    void _testHandleErrorMessageAndMultiComponentPrefix();
    void _testResetErrorLevelMessages();
    void _testChunkedStatusTextMissingChunk();
    void _testChunkedStatusTextTimeoutAddsEllipsis();
    void _testChunkedStatusTextResetsWhenChunkIdChanges();
    void _testMavlinkMessageReceivedIgnoresNonStatusText();
};
