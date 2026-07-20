#include "QGCPaletteTest.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QPalette>
#include <cmath>

namespace {
qreal linearChannel(qreal channel)
{
    return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
}

qreal relativeLuminance(const QColor& color)
{
    return 0.2126 * linearChannel(color.redF()) + 0.7152 * linearChannel(color.greenF()) +
           0.0722 * linearChannel(color.blueF());
}

qreal contrastRatio(const QColor& first, const QColor& second)
{
    const qreal firstLuminance = relativeLuminance(first);
    const qreal secondLuminance = relativeLuminance(second);
    const qreal lighter = std::max(firstLuminance, secondLuminance);
    const qreal darker = std::min(firstLuminance, secondLuminance);

    return (lighter + 0.05) / (darker + 0.05);
}
}  // namespace

void QGCPaletteTest::init()
{
    _originalTheme = QGCPalette::globalTheme();
}

void QGCPaletteTest::cleanup()
{
    QGCPalette::setGlobalTheme(_originalTheme);
}

void QGCPaletteTest::_initializesApplicationPalette()
{
    QPalette applicationPalette = QGuiApplication::palette();
    applicationPalette.setColor(QPalette::Active, QPalette::Window, Qt::magenta);
    QGuiApplication::setPalette(applicationPalette);

    QGCPalette::initializeApplicationPalette();

    QGCPalette qgcPalette;
    QCOMPARE(QGuiApplication::palette().color(QPalette::Active, QPalette::Window), qgcPalette.window());
}

void QGCPaletteTest::_syncsApplicationPalette_data()
{
    QTest::addColumn<int>("theme");

    QTest::newRow("light") << static_cast<int>(QGCPalette::Light);
    QTest::newRow("dark") << static_cast<int>(QGCPalette::Dark);
}

void QGCPaletteTest::_syncsApplicationPalette()
{
    QFETCH(int, theme);

    QGCPalette enabledPalette;
    QGCPalette disabledPalette;
    disabledPalette.setColorGroupEnabled(false);

    QGCPalette::setGlobalTheme(static_cast<QGCPalette::Theme>(theme));

    const QPalette applicationPalette = QGuiApplication::palette();
    QCOMPARE(applicationPalette.color(QPalette::Active, QPalette::Window), enabledPalette.window());
    QCOMPARE(applicationPalette.color(QPalette::Active, QPalette::WindowText), enabledPalette.text());
    QCOMPARE(applicationPalette.color(QPalette::Active, QPalette::Button), enabledPalette.button());
    QCOMPARE(applicationPalette.color(QPalette::Active, QPalette::ButtonText), enabledPalette.buttonText());
    QCOMPARE(applicationPalette.color(QPalette::Active, QPalette::Highlight), enabledPalette.buttonHighlight());
    QCOMPARE(applicationPalette.color(QPalette::Active, QPalette::HighlightedText),
             enabledPalette.buttonHighlightText());
    QCOMPARE(applicationPalette.color(QPalette::Disabled, QPalette::WindowText), disabledPalette.text());
    QCOMPARE(applicationPalette.color(QPalette::Disabled, QPalette::Button), disabledPalette.button());
    QCOMPARE(applicationPalette.color(QPalette::Disabled, QPalette::ButtonText), disabledPalette.buttonText());
}

void QGCPaletteTest::_textContrast_data()
{
    QTest::addColumn<int>("theme");

    QTest::newRow("light") << static_cast<int>(QGCPalette::Light);
    QTest::newRow("dark") << static_cast<int>(QGCPalette::Dark);
}

void QGCPaletteTest::_textContrast()
{
    QFETCH(int, theme);

    QGCPalette::setGlobalTheme(static_cast<QGCPalette::Theme>(theme));

    const QPalette palette = QGuiApplication::palette();
    const auto verifyContrast = [&palette](QPalette::ColorRole foregroundRole, QPalette::ColorRole backgroundRole,
                                           const char* pairName) {
        const qreal ratio = contrastRatio(palette.color(QPalette::Active, foregroundRole),
                                          palette.color(QPalette::Active, backgroundRole));
        QVERIFY2(ratio >= 4.5, qPrintable(QStringLiteral("%1 contrast is %2:1; expected at least 4.5:1")
                                              .arg(QString::fromLatin1(pairName))
                                              .arg(ratio, 0, 'f', 2)));
    };

    verifyContrast(QPalette::WindowText, QPalette::Window, "window text");
    verifyContrast(QPalette::ButtonText, QPalette::Button, "button text");
    verifyContrast(QPalette::Text, QPalette::Base, "field text");
}

void QGCPaletteTest::_semanticContrast_data()
{
    QTest::addColumn<int>("theme");

    QTest::newRow("light") << static_cast<int>(QGCPalette::Light);
    QTest::newRow("dark") << static_cast<int>(QGCPalette::Dark);
}

void QGCPaletteTest::_semanticContrast()
{
    QFETCH(int, theme);

    QGCPalette::setGlobalTheme(static_cast<QGCPalette::Theme>(theme));

    QGCPalette enabledPalette;
    QGCPalette disabledPalette;
    disabledPalette.setColorGroupEnabled(false);

    const auto verifyContrast = [](const QColor& foreground, const QColor& background, qreal minimumRatio,
                                   const char* pairName) {
        const qreal ratio = contrastRatio(foreground, background);
        QVERIFY2(ratio >= minimumRatio, qPrintable(QStringLiteral("%1 contrast is %2:1; expected at least %3:1")
                                                       .arg(QString::fromLatin1(pairName))
                                                       .arg(ratio, 0, 'f', 2)
                                                       .arg(minimumRatio, 0, 'f', 1)));
    };

    verifyContrast(enabledPalette.warningText(), enabledPalette.window(), 4.5, "warning text");
    verifyContrast(enabledPalette.alertText(), enabledPalette.alertBackground(), 4.5, "alert text");
    verifyContrast(enabledPalette.statusPassedText(), enabledPalette.window(), 4.5, "success status text");
    verifyContrast(enabledPalette.statusFailedText(), enabledPalette.window(), 4.5, "failure status text");
    verifyContrast(enabledPalette.colorGreen(), enabledPalette.window(), 3., "success indicator");
    verifyContrast(enabledPalette.colorRed(), enabledPalette.textField(), 3., "validation indicator");

    verifyContrast(disabledPalette.text(), disabledPalette.window(), 2., "disabled window text");
    verifyContrast(disabledPalette.buttonText(), disabledPalette.button(), 2., "disabled button text");
    verifyContrast(disabledPalette.textFieldText(), disabledPalette.textField(), 3., "disabled field text");
}

UT_REGISTER_TEST(QGCPaletteTest, TestLabel::Unit)
