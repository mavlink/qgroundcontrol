#pragma once

#include "UnitTest.h"

class DataFlashLogParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseMinimalLogTest();
    void _parseInvalidLogTest();
};
