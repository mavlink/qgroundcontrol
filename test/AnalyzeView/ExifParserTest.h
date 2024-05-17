#pragma once

#include "UnitTest.h"

class ExifParserTest : public UnitTest
{
    Q_OBJECT

public:
    ExifParserTest() = default;

private slots:
	void _readTimeTest();
	void _writeTest();
};
