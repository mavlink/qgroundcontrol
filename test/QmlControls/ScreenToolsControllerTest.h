#pragma once

#include "UnitTest.h"

class ScreenToolsControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testNormalFontFamilyForLanguage();
    void _testFontFamiliesNotEmpty();
    void _testDefaultFontDescent();
};

