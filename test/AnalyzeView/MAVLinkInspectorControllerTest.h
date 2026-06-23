#pragma once

#include "UnitTest.h"

class MAVLinkInspectorControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _constructionTest();
    void _timeScalesNonEmptyTest();
    void _rangeListNonEmptyTest();
    void _systemNamesInitiallyEmptyTest();
    void _activeSystemInitiallyNullTest();
    void _timeScalesCountTest();
    void _rangeListCountTest();
};
