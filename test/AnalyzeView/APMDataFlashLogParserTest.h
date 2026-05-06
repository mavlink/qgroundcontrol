#pragma once

#include "UnitTest.h"

class APMDataFlashLogParserTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _parseMinimalLogTest();
    void _parseInvalidLogTest();
};
