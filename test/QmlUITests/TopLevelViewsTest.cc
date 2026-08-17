#include "TopLevelViewsTest.h"

#include <QtCore/QScopeGuard>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "Fact.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

UT_REGISTER_TEST(TopLevelViewsTest, TestLabel::Integration)

// This test assumes Advanced UI mode is enabled (the default). All views including
// Analyze are expected to be visible.

static QQuickItem* findVisibleSectionButton(QQuickItem* root, int sectionIndex)
{
    if (!root || !root->isVisible()) {
        return nullptr;
    }
    if (root->property("sectionIndex").isValid() && root->property("sectionIndex").toInt() == sectionIndex) {
        return root;
    }
    const auto children = root->childItems();
    for (auto* child : children) {
        if (auto* found = findVisibleSectionButton(child, sectionIndex)) {
            return found;
        }
    }
    return nullptr;
}

void TopLevelViewsTest::_testNavigateViews()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("toolbar_qgcLogo")), "Q logo button not found");

    // Navigate to a view and verify which main panels become visible
    auto navigateAndVerify = [this](const QString &view, bool expectFlyView, bool expectPlanView, bool expectToolDrawer) {
        const QString buttonName = QStringLiteral("toolbar_view") + view;
        QVERIFY2(clickToolSelectDropdownButton(buttonName),
                 qPrintable(QStringLiteral("Failed to navigate to %1").arg(view)));

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

void TopLevelViewsTest::_testSettingsSectionVisibility()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    constexpr int videoSettingsSectionIndex = 2;

    Fact* const videoSource = SettingsManager::instance()->videoSettings()->videoSource();
    const QVariant savedVideoSource = videoSource->rawValue();
    const auto restoreVideoSource = qScopeGuard([videoSource, savedVideoSource] {
        videoSource->setRawValue(savedVideoSource);
    });
    videoSource->setRawValue(QString::fromLatin1(VideoSettings::videoSourceRTSP));

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));

    QQuickItem* const videoButton = findVisibleItem(_rootItem, QStringLiteral("settingsButton_Video"));
    QVERIFY(videoButton);
    QVERIFY(scrollIntoView(videoButton, QStringLiteral("settings_buttonList")));
    const QPointF videoCenter = videoButton->mapToScene(QPointF(videoButton->width() / 2, videoButton->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, videoCenter.toPoint());

    QQuickItem* const videoPage = findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video"));
    QVERIFY(videoPage);
    QQuickItem* videoSection = nullptr;
    QTRY_VERIFY((videoSection = findVisibleSectionButton(videoButton->parentItem(), videoSettingsSectionIndex)));
    const QPointF sectionCenter = videoSection->mapToScene(QPointF(videoSection->width() / 2, videoSection->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, sectionCenter.toPoint());

    QVERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsGroup_Settings")));

    videoSource->setRawValue(QString::fromLatin1(VideoSettings::videoDisabled));

    QTRY_VERIFY(!findVisibleSectionButton(videoButton->parentItem(), videoSettingsSectionIndex));
    QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("settingsGroup_Settings"), 0));
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsGroup_VideoSource")));
    QVERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video")));
}
