#pragma once

#include "UnitTest.h"

class MAVLinkConsoleController;

class MAVLinkConsoleControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _constructionTest();
    void _initialStateTest();
    void _historyUpOnEmptyTest();
    void _historyDownOnEmptyTest();
    void _historyUpSingleEntryTest();
    void _historyUpDownNavigationTest();
    void _historyNoDuplicatesTest();
    void _historyUpAtTopBoundaryTest();
    void _historyDownAtBottomBoundaryTest();
    void _handleClipboardNoNewlineTest();
    void _handleClipboardWithNewlineTest();
    void _handleClipboardEmptyPrefixTest();
    void _handleClipboardMultilineTest();
};
