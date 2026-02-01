#pragma once

#include "UnitTest.h"

class StatusTextHandlerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGetMessageText();
    void _testHandleTextMessage();
};
