#pragma once

#include "UnitTest.h"

class QGCSerialPortInfoTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testLoadJsonData();
    void _testLoadJsonDataIdempotent();
    void _testBoardClassStringToType();
    void _testBoardTypeToString();
};
