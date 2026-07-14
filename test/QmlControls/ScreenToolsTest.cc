#include "ScreenToolsTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QScopeGuard>
#include <QtCore/QScopedPointer>
#include <QtGui/QFont>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <array>
#include <utility>

#include "AppSettings.h"
#include "Fact.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

void ScreenToolsTest::_compatibilityFacade()
{
    static constexpr char qml[] = R"(
import QtQuick
import QGCStyle as QGCStyle
import QGroundControl
import QGroundControl.Controls

QtObject {
    readonly property bool isDebugBuild: QGroundControl.isDebugBuild
    readonly property QtObject mockScreen: QtObject {
        property real height: 678
        property real pixelDensity: 3.25
        property real width: 1234
    }
    readonly property string platformOs: Qt.platform.os
    readonly property var screenTools: ScreenTools
    readonly property var styleEnvironment: QGCStyleEnvironment
    readonly property var styleMetrics: QGCStyle.StyleMetrics
    readonly property var styleTypography: QGCStyle.StyleTypography
}
)";

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/qml"));
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("qrc:/tests/ScreenToolsCompatibilityTest.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    QObject* const screenTools = root->property("screenTools").value<QObject*>();
    QObject* const mockScreen = root->property("mockScreen").value<QObject*>();
    QObject* const styleMetrics = root->property("styleMetrics").value<QObject*>();
    QObject* const styleTypography = root->property("styleTypography").value<QObject*>();
    QVERIFY(screenTools);
    QVERIFY(mockScreen);
    QVERIFY(styleMetrics);
    QVERIFY(styleTypography);

    static constexpr std::array metricMappings = {
        std::pair{"implicitButtonWidth", "implicitButtonWidth"},
        std::pair{"implicitButtonHeight", "implicitButtonHeight"},
        std::pair{"implicitCheckBoxHeight", "implicitCheckBoxHeight"},
        std::pair{"implicitTextFieldWidth", "implicitTextFieldWidth"},
        std::pair{"implicitTextFieldHeight", "implicitTextFieldHeight"},
        std::pair{"implicitComboBoxHeight", "implicitComboBoxHeight"},
        std::pair{"implicitComboBoxWidth", "implicitComboBoxWidth"},
        std::pair{"comboBoxPadding", "comboBoxPadding"},
        std::pair{"implicitSliderHeight", "implicitSliderHeight"},
        std::pair{"defaultBorderRadius", "defaultBorderRadius"},
        std::pair{"defaultDialogControlSpacing", "defaultDialogControlSpacing"},
        std::pair{"radioButtonIndicatorSize", "radioButtonIndicatorSize"},
        std::pair{"minTouchMillimeters", "minimumTouchMillimeters"},
        std::pair{"toolbarHeight", "toolbarHeight"},
    };
    for (const auto& [screenToolsProperty, styleProperty] : metricMappings) {
        QCOMPARE(screenTools->property(screenToolsProperty), styleMetrics->property(styleProperty));
    }

    static constexpr std::array typographyMappings = {
        std::pair{"defaultFontDescent", "bodyDescent"},          std::pair{"defaultFontPixelHeight", "bodyPixelHeight"},
        std::pair{"defaultFontPixelWidth", "bodyPixelWidth"},    std::pair{"defaultFontPointSize", "bodyPointSize"},
        std::pair{"largeFontPixelHeight", "titlePixelHeight"},   std::pair{"largeFontPixelWidth", "titlePixelWidth"},
        std::pair{"largeFontPointRatio", "titleScale"},          std::pair{"largeFontPointSize", "titlePointSize"},
        std::pair{"mediumFontPointRatio", "headingScale"},       std::pair{"mediumFontPointSize", "headingPointSize"},
        std::pair{"platformFontPointSize", "platformPointSize"}, std::pair{"smallFontPointRatio", "captionScale"},
        std::pair{"smallFontPointSize", "captionPointSize"},     std::pair{"fixedFontFamily", "fixedFontFamily"},
        std::pair{"normalFontFamily", "normalFontFamily"},
    };
    for (const auto& [screenToolsProperty, styleProperty] : typographyMappings) {
        QTRY_COMPARE_WITH_TIMEOUT(screenTools->property(screenToolsProperty), styleTypography->property(styleProperty),
                                  5000);
    }

    QScreen* const expectedScreen = QGuiApplication::primaryScreen();
    QVERIFY(expectedScreen);
    const bool fakeMobile = screenTools->property("isFakeMobile").toBool();
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    const bool expectedFakeMobile = false;
#else
    const bool expectedFakeMobile = QCoreApplication::arguments().contains(QStringLiteral("--fake-mobile")) ||
                                    QCoreApplication::arguments().contains(QStringLiteral("-fake-mobile"));
#endif
    QCOMPARE(fakeMobile, expectedFakeMobile);
    QCOMPARE(screenTools->property("screenWidth").toInt(), fakeMobile ? 731 : expectedScreen->size().width());
    QCOMPARE(screenTools->property("screenHeight").toInt(), fakeMobile ? 411 : expectedScreen->size().height());
    const qreal expectedPixelDensity = expectedScreen->physicalDotsPerInch() / 25.4;
    QCOMPARE(screenTools->property("_screenPixelDensity").toReal(),
             expectedPixelDensity > 0 ? expectedPixelDensity : 1);

    QObject* const layoutProfile = screenTools->property("_layoutProfile").value<QObject*>();
    QVERIFY(layoutProfile);
    QCOMPARE(screenTools->property("isShortScreen"), layoutProfile->property("isShort"));
    QCOMPARE(screenTools->property("isTinyScreen"), layoutProfile->property("isTiny"));
    QCOMPARE(screenTools->property("minTouchPixels"), layoutProfile->property("minimumTouchTarget"));
    QCOMPARE(screenTools->property("realPixelDensity"), layoutProfile->property("pixelDensity"));

    if (!fakeMobile) {
        const QVariant originalWindowScreen = screenTools->property("_windowScreen");
        const auto restoreWindowScreen = qScopeGuard(
            [screenTools, originalWindowScreen]() { screenTools->setProperty("_windowScreen", originalWindowScreen); });

        QVERIFY(screenTools->setProperty("_windowScreen", QVariant::fromValue(mockScreen)));
        QTRY_COMPARE_WITH_TIMEOUT(screenTools->property("screenWidth").toReal(), 1234., 5000);
        QTRY_COMPARE_WITH_TIMEOUT(screenTools->property("screenHeight").toReal(), 678., 5000);
        QTRY_COMPARE_WITH_TIMEOUT(screenTools->property("_screenPixelDensity").toReal(), 3.25, 5000);

        QVERIFY(screenTools->setProperty("_windowScreen", QVariant::fromValue(static_cast<QObject*>(nullptr))));
        QTRY_COMPARE_WITH_TIMEOUT(screenTools->property("screenWidth").toInt(), expectedScreen->size().width(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(screenTools->property("screenHeight").toInt(), expectedScreen->size().height(), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(screenTools->property("_screenPixelDensity").toReal(),
                                  expectedPixelDensity > 0 ? expectedPixelDensity : 1, 5000);
    }

    const QString platformOs = root->property("platformOs").toString();
    QCOMPARE(screenTools->property("isAndroid").toBool(), platformOs == QStringLiteral("android"));
    QCOMPARE(screenTools->property("isiOS").toBool(), platformOs == QStringLiteral("ios"));
    QCOMPARE(screenTools->property("isMobile").toBool(),
             screenTools->property("isAndroid").toBool() || screenTools->property("isiOS").toBool() || fakeMobile);
    QCOMPARE(screenTools->metaObject()->indexOfProperty("hasTouch"), -1);
    QCOMPARE(screenTools->metaObject()->indexOfProperty("isDebug"), -1);
    QCOMPARE(screenTools->metaObject()->indexOfProperty("isSerialAvailable"), -1);
    QCOMPARE(screenTools->metaObject()->indexOfProperty("_systemFixedFontFamily"), -1);
#ifdef QT_DEBUG
    QVERIFY(root->property("isDebugBuild").toBool());
#else
    QVERIFY(!root->property("isDebugBuild").toBool());
#endif
    QCOMPARE(styleTypography->property("normalFontFamily").toString(), QGuiApplication::font().family());
}

void ScreenToolsTest::_runtimeLanguageFontSwitch()
{
    Fact* const languageFact = SettingsManager::instance()->appSettings()->qLocaleLanguage();
    QVERIFY(languageFact);
    const QVariant originalLanguage = languageFact->rawValue();
    ignoreLogMessage("API.QGCApplication", QtWarningMsg,
                     QRegularExpression(R"(Qt lib localization for ".+" is not present)"));
    ignoreLogMessage("API.QGCApplication", QtWarningMsg,
                     QRegularExpression(R"(Error loading source localization for ".+")"));
    ignoreLogMessage("API.QGCApplication", QtWarningMsg,
                     QRegularExpression(R"(Error loading json localization for ".+")"));
    const auto restoreLanguage =
        qScopeGuard([languageFact, originalLanguage]() { languageFact->setRawValue(originalLanguage); });

    languageFact->setRawValue(QLocale::English);
    QTRY_COMPARE_WITH_TIMEOUT(qgcApp()->getCurrentLanguage().language(), QLocale::English, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(QGuiApplication::font().family(), QStringLiteral("Open Sans"), 5000);

    languageFact->setRawValue(QLocale::Korean);
    QTRY_COMPARE_WITH_TIMEOUT(qgcApp()->getCurrentLanguage().language(), QLocale::Korean, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(QGuiApplication::font().family(), QStringLiteral("NanumGothic"), 5000);

    languageFact->setRawValue(QLocale::English);
    QTRY_COMPARE_WITH_TIMEOUT(QGuiApplication::font().family(), QStringLiteral("Open Sans"), 5000);
}

UT_REGISTER_TEST(ScreenToolsTest, TestLabel::Unit)
