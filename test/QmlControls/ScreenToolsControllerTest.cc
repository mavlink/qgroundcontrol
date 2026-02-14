#include "ScreenToolsControllerTest.h"

#include "ScreenToolsController.h"

#include <QtCore/QLocale>

void ScreenToolsControllerTest::_testNormalFontFamilyForLanguage()
{
    QCOMPARE(ScreenToolsController::normalFontFamilyForLanguage(QLocale::Korean), QStringLiteral("NanumGothic"));
    QCOMPARE(ScreenToolsController::normalFontFamilyForLanguage(QLocale::English), QStringLiteral("Open Sans"));
    QCOMPARE(ScreenToolsController::normalFontFamilyForLanguage(QLocale::AnyLanguage), QStringLiteral("Open Sans"));
}

void ScreenToolsControllerTest::_testFontFamiliesNotEmpty()
{
    const ScreenToolsController controller;

    QVERIFY(!controller.normalFontFamily().isEmpty());
    QVERIFY(!controller.fixedFontFamily().isEmpty());
}

void ScreenToolsControllerTest::_testDefaultFontDescent()
{
    QVERIFY(ScreenToolsController::defaultFontDescent(10) >= 0.0);
    QVERIFY(ScreenToolsController::defaultFontDescent(20) >= 0.0);
}

#include "UnitTest.h"

UT_REGISTER_TEST(ScreenToolsControllerTest, TestLabel::Unit, TestLabel::QmlControls)
