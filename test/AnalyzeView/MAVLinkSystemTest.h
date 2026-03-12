#pragma once

#include "UnitTest.h"

class MAVLinkSystemTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _constructionTest();
    void _initiallyEmptyMessagesTest();
    void _initiallyNoCompIDsTest();
    void _appendAndFindMessageTest();
    void _findMessageNotFoundTest();
    void _findMessageByPointerTest();
    void _setSelectedAndSelectedMsgTest();
    void _setSelectedOutOfBoundsTest();
    void _appendUpdatesCompIDsTest();
    void _selectedMsgOnEmptySystemTest();
};
