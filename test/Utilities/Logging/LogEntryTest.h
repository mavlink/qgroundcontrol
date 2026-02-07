#pragma once

#include "UnitTest.h"

class LogEntryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void cleanup() override;

    void _testDefaultConstruction();
    void _testToStringBasic();
    void _testToStringWithCategory();
    void _testToStringWithFunction();
    void _testToStringWithLevel();
    void _testFromQtMsgType();
    void _testLevelLabel();
};
