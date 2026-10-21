#include "SettingsPageWidthUITest.h"

#include <QtCore/QRegularExpression>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

UT_REGISTER_TEST(SettingsPageWidthUITest, TestLabel::Integration)

namespace {

constexpr int kHerelinkWidth = 731;
constexpr int kHerelinkHeight = 411;
constexpr qreal kWidthTolerancePx = 1;

}  // namespace

void SettingsPageWidthUITest::_testPageFitsHerelinkWidth_data()
{
    QTest::addColumn<QString>("pageName");

    // Pages generated from *.SettingsUI.json that are always available
    QTest::newRow("General") << QStringLiteral("General");
    QTest::newRow("Fly View") << QStringLiteral("Fly View");
    QTest::newRow("Plan View") << QStringLiteral("Plan View");
    QTest::newRow("ADSB Server") << QStringLiteral("ADSB Server");
    QTest::newRow("Comm Links") << QStringLiteral("Comm Links");
    QTest::newRow("App Logging") << QStringLiteral("App Logging");
    QTest::newRow("Maps") << QStringLiteral("Maps");
    QTest::newRow("GNSS Receiver") << QStringLiteral("GNSS Receiver");
    QTest::newRow("RTK Corrections") << QStringLiteral("RTK Corrections");
    QTest::newRow("Remote ID") << QStringLiteral("Remote ID");
    QTest::newRow("Telemetry") << QStringLiteral("Telemetry");
}

void SettingsPageWidthUITest::_testPageFitsHerelinkWidth()
{
    QFETCH(QString, pageName);

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    // MainWindowSavedState sets a minimum size that onscreen platforms enforce
    _window->setMinimumWidth(0);
    _window->setMinimumHeight(0);
    _window->resize(kHerelinkWidth, kHerelinkHeight);
    QTRY_COMPARE_WITH_TIMEOUT(_rootItem->width(), qreal(kHerelinkWidth), TestTimeout::shortMs());

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));

    const QString buttonName = QStringLiteral("settingsButton_") + pageName;
    QQuickItem* const button = findVisibleItem(_rootItem, buttonName);
    QVERIFY2(button, qPrintable(QStringLiteral("Settings page button not found: %1").arg(buttonName)));
    scrollIntoView(button, QStringLiteral("settings_buttonList"));

    const QPointF center = button->mapToScene(QPointF(button->width() / 2, button->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());

    // Page root objectNames are sanitized to [A-Za-z0-9_]
    QString wrapperName = QStringLiteral("settingsPage_") + pageName;
    wrapperName.remove(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]")));
    QVERIFY2(findVisibleItem(_rootItem, wrapperName),
             qPrintable(QStringLiteral("Settings page not found: %1").arg(wrapperName)));

    QQuickItem* const flickable = findVisibleItem(_rootItem, QStringLiteral("settingsPageFlickable"));
    QVERIFY2(flickable, "settingsPageFlickable not found");

    QTRY_VERIFY2_WITH_TIMEOUT(flickable->property("contentWidth").toReal() <= flickable->width() + kWidthTolerancePx,
                              qPrintable(QStringLiteral("contentWidth %1 exceeds flickable width %2")
                                             .arg(flickable->property("contentWidth").toReal())
                                             .arg(flickable->width())),
                              TestTimeout::shortMs());

    // contentWidth is capped by construction, so also check no child is wider than the layout
    QQuickItem* const layout = findItem(_rootItem, QStringLiteral("settingsPageLayout"));
    QVERIFY2(layout, "settingsPageLayout not found");
    const qreal rightEdge = flickable->mapToScene(QPointF(flickable->width(), 0)).x();
    for (const QQuickItem* child : layout->childItems()) {
        if (!child->isVisible()) {
            continue;
        }
        const qreal childRight = child->mapToScene(QPointF(child->width(), 0)).x();
        QVERIFY2(
            childRight <= rightEdge + kWidthTolerancePx,
            qPrintable(QStringLiteral("%1 extends %2 px past the page")
                           .arg(child->objectName().isEmpty() ? QStringLiteral("<unnamed child>") : child->objectName())
                           .arg(childRight - rightEdge)));
    }

    stopUI();
}
