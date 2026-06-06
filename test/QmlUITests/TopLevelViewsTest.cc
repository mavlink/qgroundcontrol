#include "TopLevelViewsTest.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

UT_REGISTER_TEST(TopLevelViewsTest, TestLabel::Integration)

// This test assumes Advanced UI mode is enabled (the default). All views including
// Analyze are expected to be visible.

void TopLevelViewsTest::_testNavigateViews()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("toolbar_qgcLogo")), "Q logo button not found");

    // Navigate to a view and verify which main panels become visible
    auto navigateAndVerify = [this](const QString &view, bool expectFlyView, bool expectPlanView, bool expectToolDrawer) {
        QVERIFY2(clickButton(QStringLiteral("toolbar_qgcLogo")),
                 qPrintable(QStringLiteral("Failed to click Q logo button before %1").arg(view)));

        const QString buttonName = QStringLiteral("toolbar_view") + view;
        QVERIFY2(findVisibleItem(_rootItem, buttonName, 2000),
                 qPrintable(QStringLiteral("View button not found: %1").arg(buttonName)));
        QVERIFY2(clickButton(buttonName),
                 qPrintable(QStringLiteral("Failed to click %1").arg(buttonName)));

        const int flyTimeout    = expectFlyView    ? 1000 : 0;
        const int planTimeout   = expectPlanView   ? 1000 : 0;
        const int drawerTimeout = expectToolDrawer ? 1000 : 0;
        QVERIFY2((findVisibleItem(_rootItem, QStringLiteral("mainView_fly"),        flyTimeout)    != nullptr) == expectFlyView,
                 qPrintable(QStringLiteral("mainView_fly visibility wrong after switching to %1").arg(view)));
        QVERIFY2((findVisibleItem(_rootItem, QStringLiteral("mainView_plan"),       planTimeout)   != nullptr) == expectPlanView,
                 qPrintable(QStringLiteral("mainView_plan visibility wrong after switching to %1").arg(view)));
        QVERIFY2((findVisibleItem(_rootItem, QStringLiteral("mainView_toolDrawer"), drawerTimeout) != nullptr) == expectToolDrawer,
                 qPrintable(QStringLiteral("mainView_toolDrawer visibility wrong after switching to %1").arg(view)));
        QTest::qWait(_viewDelay);
    };

    //                      view name      flyView  planView  toolDrawer
    navigateAndVerify(QStringLiteral("Fly"),       true,    false,    false);
    navigateAndVerify(QStringLiteral("Plan"),      false,   true,     false);
    navigateAndVerify(QStringLiteral("Configure"), false,   true,     true);
    navigateAndVerify(QStringLiteral("Analyze"),   false,   true,     true);
    navigateAndVerify(QStringLiteral("Settings"),  false,   true,     true);

    // Navigate through settings pages that are unconditionally visible
    {
        navigateAndVerify(QStringLiteral("Settings"), false, true, true);

        const QStringList settingsPages = {
            QStringLiteral("General"),
            QStringLiteral("Fly View"),
            QStringLiteral("Plan View"),
            QStringLiteral("ADSB Server"),
            QStringLiteral("Comm Links"),
            QStringLiteral("App Logging"),
            QStringLiteral("App Log Viewer"),
            QStringLiteral("Maps"),
            QStringLiteral("Remote ID"),
            QStringLiteral("Telemetry"),
            QStringLiteral("Video"),
            QStringLiteral("Help"),
        };

        for (const QString &page : settingsPages) {
            const QString buttonName = QStringLiteral("settingsButton_") + page;

            QQuickItem *btn = findVisibleItem(_rootItem, buttonName);
            QVERIFY2(btn, qPrintable(QStringLiteral("Settings page button not found: %1").arg(buttonName)));

            scrollIntoView(btn, QStringLiteral("settings_buttonList"));

            const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
            QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
            QTest::qWait(_pageDelay);

            const QString expectedObjectName = QStringLiteral("settingsPage_") + QString(page).remove(' ');
            QVERIFY2(findVisibleItem(_rootItem, expectedObjectName),
                     qPrintable(QStringLiteral("Settings page objectName not found: %1").arg(expectedObjectName)));
        }
    }

    stopUI();
}
