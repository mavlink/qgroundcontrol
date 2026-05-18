#include "TopLevelViewsTest.h"

#include <QtCore/QRegularExpression>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuickControls2/QQuickStyle>
#include <QtTest/QTest>

#include "ColoredSvgImageProvider.h"
#include "LinkManager.h"
#include "MAVLinkProtocol.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "QGCImageProvider.h"
#include "JoystickManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"

UT_REGISTER_TEST(TopLevelViewsTest, TestLabel::Integration)

// This test assumes Advanced UI mode is enabled (the default). All views including
// Analyze are expected to be visible.

void TopLevelViewsTest::_testNavigateViews()
{
    setStrictLogCheck(true);

    // Initialize subsystems needed for full QML UI
    QQuickStyle::setStyle("Basic");
    QGCCorePlugin::instance()->init();
    MAVLinkProtocol::instance()->init();
    MultiVehicleManager::instance()->init();

    // Suppress first-run prompts so they don't block the UI
    AppSettings *appSettings = SettingsManager::instance()->appSettings();
    const QList<int> promptIds = QGCCorePlugin::instance()->firstRunPromptStdIds();
    for (int id : promptIds) {
        appSettings->firstRunPromptIdsMarkIdAsShown(id);
    }

    // Verify we are in Advanced UI mode as this test requires it
    QVERIFY2(QGCCorePlugin::instance()->showAdvancedUI(), "Test requires Advanced UI mode");

    // Ignore benign Qt platform warnings that can't be avoided in offscreen mode
    ignoreLogMessage(QRegularExpression(QStringLiteral("^default$")), QtWarningMsg, QRegularExpression(QStringLiteral("This plugin does not support propagateSizeHints")));
    ignoreLogMessage(QRegularExpression(QStringLiteral("^qt\\.qpa\\.fonts$")), QtWarningMsg, QRegularExpression(QStringLiteral("Populating font family aliases")));
    ignoreLogMessage(QRegularExpression(QStringLiteral("^default$")), QtWarningMsg, QRegularExpression(QStringLiteral("QRhiGles2")));

    // Create QML engine (mirrors _initForNormalAppBoot)
    QQmlApplicationEngine *engine = QGCCorePlugin::instance()->createQmlApplicationEngine(this);
    QVERIFY(engine);

    engine->addImageProvider(QStringLiteral("QGCImages"), new QGCImageProvider());
    engine->addImageProvider(QLatin1String(ColoredSvgImageProvider::ProviderId), new ColoredSvgImageProvider());

    // Load MainWindow
    engine->load(QUrl(QStringLiteral("qrc:/qml/QGroundControl/MainWindow.qml")));
    QVERIFY(!engine->rootObjects().isEmpty());

    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine->rootObjects().first());
    QVERIFY(window);

    // Wait for window to be fully rendered
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickItem *rootItem = window->contentItem();

    // Verify the Q logo button exists
    QQuickItem *qgcButton = findVisibleItem(rootItem, QStringLiteral("toolbar_qgcLogo"));
    QVERIFY2(qgcButton, "Q logo button not found");

    // Helper to click a button and wait
    auto clickButton = [&](const QString &objectName) -> bool {
        QQuickItem *btn = findVisibleItem(rootItem, objectName);
        if (!btn) {
            return false;
        }
        const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
        return true;
    };

    // Helper to navigate to a view and verify the switch
    auto navigateAndVerify = [&](const QString &view, bool expectFlyView, bool expectPlanView, bool expectToolDrawer) {
        QVERIFY2(clickButton(QStringLiteral("toolbar_qgcLogo")),
                 qPrintable(QStringLiteral("Failed to click Q logo button before %1").arg(view)));

        const QString buttonName = QStringLiteral("toolbar_view") + view;
        QVERIFY2(findVisibleItem(rootItem, buttonName, 2000),
                 qPrintable(QStringLiteral("View button not found: %1").arg(buttonName)));
        QVERIFY2(clickButton(buttonName),
                 qPrintable(QStringLiteral("Failed to click %1").arg(buttonName)));

        const int flyTimeout = expectFlyView ? 1000 : 0;
        const int planTimeout = expectPlanView ? 1000 : 0;
        const int drawerTimeout = expectToolDrawer ? 1000 : 0;
        QVERIFY2((findVisibleItem(rootItem, QStringLiteral("mainView_fly"), flyTimeout) != nullptr) == expectFlyView,
                 qPrintable(QStringLiteral("mainView_fly visibility wrong after switching to %1").arg(view)));
        QVERIFY2((findVisibleItem(rootItem, QStringLiteral("mainView_plan"), planTimeout) != nullptr) == expectPlanView,
                 qPrintable(QStringLiteral("mainView_plan visibility wrong after switching to %1").arg(view)));
        QVERIFY2((findVisibleItem(rootItem, QStringLiteral("mainView_toolDrawer"), drawerTimeout) != nullptr) == expectToolDrawer,
                 qPrintable(QStringLiteral("mainView_toolDrawer visibility wrong after switching to %1").arg(view)));
    };

    //                   view name      flyView  planView  toolDrawer
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
            QStringLiteral("Logging"),
            QStringLiteral("Log Viewer"),
            QStringLiteral("Maps"),
            QStringLiteral("Remote ID"),
            QStringLiteral("Telemetry"),
            QStringLiteral("Video"),
            QStringLiteral("Help"),
        };

        for (const QString &page : settingsPages) {
            const QString buttonName = QStringLiteral("settingsButton_") + page;

            QQuickItem *btn = findVisibleItem(rootItem, buttonName);
            QVERIFY2(btn, qPrintable(QStringLiteral("Settings page button not found: %1").arg(buttonName)));

            // Scroll the flickable if the button is out of view
            QQuickItem *flickable = findVisibleItem(rootItem, QStringLiteral("settings_buttonList"));
            if (flickable) {
                // Map button position to flickable coordinates and scroll to it
                const QPointF btnPos = btn->mapToItem(flickable, QPointF(0, 0));
                const qreal contentY = flickable->property("contentY").toReal();
                const qreal flickHeight = flickable->height();
                if (btnPos.y() < 0 || btnPos.y() + btn->height() > flickHeight) {
                    flickable->setProperty("contentY", contentY + btnPos.y() - flickHeight / 4);
                }
            }

            const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center.toPoint());

            // Verify the correct page is showing by checking its objectName
            const QString expectedObjectName = QStringLiteral("settingsPage_") + QString(page).remove(' ');
            QVERIFY2(findVisibleItem(rootItem, expectedObjectName),
                     qPrintable(QStringLiteral("Settings page objectName not found: %1").arg(expectedObjectName)));
        }
    }

    // Close the window before destroying the engine to avoid QML teardown issues
    window->close();
    QTest::qWait(100);

    // Clean up the engine before the test ends
    delete engine;
    QTest::qWait(100);
}

QQmlApplicationEngine *TopLevelViewsTest::_createTestEngine()
{
    return nullptr; // unused, kept for future expansion
}
