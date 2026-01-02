#pragma once

#include "UnitTest.h"

class StatusTextHandlerTest : public UnitTest
{
    Q_OBJECT

public:
    StatusTextHandlerTest() = default;

private slots:
    void _testGetMessageText();
    void _testHandleTextMessage();
};
