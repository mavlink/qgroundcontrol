#include "TopLevelViewsTest.h"

#include <QtCore/QScopeGuard>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

#include "Fact.h"
#include "SettingsManager.h"
#include "VideoSettings.h"

UT_REGISTER_TEST(TopLevelViewsTest, TestLabel::Integration)

// This test assumes Advanced UI mode is enabled (the default). All views including
// Analyze are expected to be visible.

// Section nav buttons carry a sectionIndex property; match on the label text so
// tests don't depend on section ordering in the page definition JSON.
static QQuickItem* findVisibleSectionButton(QQuickItem* root, const QString& sectionName)
{
    if (!root || !root->isVisible()) {
        return nullptr;
    }
    const auto children = root->childItems();
    if (root->property("sectionIndex").isValid()) {
        for (auto* label : children) {
            if (label->isVisible() && label->property("text").toString() == sectionName) {
                return root;
            }
        }
    }
    for (auto* child : children) {
        if (auto* found = findVisibleSectionButton(child, sectionName)) {
            return found;
        }
    }
    return nullptr;
}

static void setVideoSource(const char* source)
{
    SettingsManager::instance()->videoSettings()->videoSource()->setRawValue(QString::fromLatin1(source));
}

// Sets the video source and returns a guard restoring the original value on scope exit
[[nodiscard]] static auto setVideoSourceWithRestore(const char* source)
{
    Fact* const videoSource = SettingsManager::instance()->videoSettings()->videoSource();
    const QVariant savedVideoSource = videoSource->rawValue();
    videoSource->setRawValue(QString::fromLatin1(source));
    return qScopeGuard([videoSource, savedVideoSource] { videoSource->setRawValue(savedVideoSource); });
}

QQuickItem* TopLevelViewsTest::_clickSettingsButton(const QString& pageName)
{
    const QString objectName = QStringLiteral("settingsButton_") + pageName;
    QQuickItem* const button = findVisibleItem(_rootItem, objectName);
    if (!button) {
        QTest::qFail(qPrintable(QStringLiteral("Settings button not found: %1").arg(pageName)), __FILE__, __LINE__);
        return nullptr;
    }
    if (!scrollIntoView(button, QStringLiteral("settings_buttonList"))) {
        QTest::qFail(qPrintable(QStringLiteral("Failed to scroll settings button into view: %1").arg(pageName)),
                     __FILE__, __LINE__);
        return nullptr;
    }
    if (!_clickItemAt(button, 0.5, 0.5, objectName)) {
        return nullptr;
    }
    return button;
}

void TopLevelViewsTest::_testNavigateViews()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("toolbar_qgcLogo")), "Q logo button not found");

    // Navigate to a view and verify which main panels become visible
    auto navigateAndVerify = [this](const QString& view, bool expectFlyView, bool expectPlanView,
                                    bool expectToolDrawer) {
        const QString buttonName = QStringLiteral("toolbar_view") + view;
        QVERIFY2(clickToolSelectDropdownButton(buttonName),
                 qPrintable(QStringLiteral("Failed to navigate to %1").arg(view)));

        const int flyTimeout = expectFlyView ? 1000 : 0;
        const int planTimeout = expectPlanView ? 1000 : 0;
        const int drawerTimeout = expectToolDrawer ? 1000 : 0;
        QVERIFY2((findVisibleItem(_rootItem, QStringLiteral("mainView_fly"), flyTimeout) != nullptr) == expectFlyView,
                 qPrintable(QStringLiteral("mainView_fly visibility wrong after switching to %1").arg(view)));
        QVERIFY2(
            (findVisibleItem(_rootItem, QStringLiteral("mainView_plan"), planTimeout) != nullptr) == expectPlanView,
            qPrintable(QStringLiteral("mainView_plan visibility wrong after switching to %1").arg(view)));
        QVERIFY2((findVisibleItem(_rootItem, QStringLiteral("mainView_toolDrawer"), drawerTimeout) != nullptr) ==
                     expectToolDrawer,
                 qPrintable(QStringLiteral("mainView_toolDrawer visibility wrong after switching to %1").arg(view)));
        QTest::qWait(_viewDelay);
    };

    //                      view name      flyView  planView  toolDrawer
    navigateAndVerify(QStringLiteral("Fly"), true, false, false);
    navigateAndVerify(QStringLiteral("Plan"), false, true, false);
    navigateAndVerify(QStringLiteral("Configure"), false, true, true);
    navigateAndVerify(QStringLiteral("Analyze"), false, true, true);
    navigateAndVerify(QStringLiteral("Settings"), false, true, true);

    // Navigate through settings pages that are unconditionally visible
    {
        navigateAndVerify(QStringLiteral("Settings"), false, true, true);

        const QStringList settingsPages = {
            QStringLiteral("General"),        QStringLiteral("Fly View"),   QStringLiteral("Plan View"),
            QStringLiteral("ADSB Server"),    QStringLiteral("Comm Links"), QStringLiteral("App Logging"),
            QStringLiteral("App Log Viewer"), QStringLiteral("Maps"),       QStringLiteral("Remote ID"),
            QStringLiteral("Telemetry"),      QStringLiteral("Video"),      QStringLiteral("Help"),
        };

        for (const QString& page : settingsPages) {
            if (!_clickSettingsButton(page)) {
                return;
            }

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
    if (QTest::currentTestFailed())
        return;

    const auto restoreVideoSource = setVideoSourceWithRestore(VideoSettings::videoSourceRTSP);

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));

    QQuickItem* const videoButton = _clickSettingsButton(QStringLiteral("Video"));
    if (!videoButton) {
        return;
    }
    QVERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video")));

    QQuickItem* settingsSection = nullptr;
    QTRY_VERIFY((settingsSection = findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Settings"))));
    QVERIFY(_clickItemAt(settingsSection, 0.5, 0.5, QStringLiteral("section Settings")));

    QVERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsGroup_Settings")));

    setVideoSource(VideoSettings::videoDisabled);

    QTRY_VERIFY(!findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Settings")));
    QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("settingsGroup_Settings"), 0));
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsGroup_VideoSource"), 0));
    QVERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video")));
}

// Primary #14898 regression: hidden sections must not reappear in the nav tree
// after another page is selected and the Video page contents unload.
void TopLevelViewsTest::_testSettingsHiddenSectionAfterPageSwitch()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    const auto restoreVideoSource = setVideoSourceWithRestore(VideoSettings::videoSourceRTSP);

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));

    QQuickItem* const videoButton = _clickSettingsButton(QStringLiteral("Video"));
    if (!videoButton) {
        return;
    }
    QVERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video")));
    QTRY_VERIFY(findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Settings")));

    setVideoSource(VideoSettings::videoDisabled);
    QTRY_VERIFY(!findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Settings")));

    // Select another page; the Video entry stays expanded in the nav tree,
    // which is the path that used to resurrect the hidden sections
    if (!_clickSettingsButton(QStringLiteral("General"))) {
        return;
    }
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_General"), 0));
    QTRY_VERIFY(!findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Settings")));
    QTRY_VERIFY(!findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Connection")));

    // Back to Video: panel must show the remaining content, not go blank
    if (!_clickSettingsButton(QStringLiteral("Video"))) {
        return;
    }
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video"), 0));
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsGroup_VideoSource"), 0));
    QTRY_VERIFY(!findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Settings")));
}

// When a page collapses to a single visible section while one of its sections is
// selected, the selection must normalize to the page itself so the nav still has
// a checked item.
void TopLevelViewsTest::_testSettingsSectionCollapseToSingle()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    const auto restoreVideoSource = setVideoSourceWithRestore(VideoSettings::videoSourceRTSP);

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));

    QQuickItem* const videoButton = _clickSettingsButton(QStringLiteral("Video"));
    if (!videoButton) {
        return;
    }
    QVERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video")));

    // Select the always-visible "Video Source" section
    QQuickItem* sourceSection = nullptr;
    QTRY_VERIFY((sourceSection = findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Video Source"))));
    QVERIFY(_clickItemAt(sourceSection, 0.5, 0.5, QStringLiteral("section Video Source")));
    QTRY_VERIFY(!videoButton->property("checked").toBool());

    // Disabling the source hides all other sections, leaving only "Video Source"
    setVideoSource(VideoSettings::videoDisabled);

    QTRY_VERIFY(!findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Video Source")));
    QTRY_VERIFY(videoButton->property("checked").toBool());
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsGroup_VideoSource"), 0));
}

// When the selected page becomes entirely unavailable the view must fall back to
// the first available page instead of leaving stale content or a blank panel.
void TopLevelViewsTest::_testSettingsPageUnavailableFallback()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    VideoSettings* const videoSettings = SettingsManager::instance()->videoSettings();
    const auto restoreUserVisible = qScopeGuard([videoSettings] { videoSettings->setUserVisible(true); });

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));

    if (!_clickSettingsButton(QStringLiteral("Video"))) {
        return;
    }
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video"), 0));

    videoSettings->setUserVisible(false);

    QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("settingsButton_Video"), 0));
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsPage_General"), 0));
    QQuickItem* const generalButton = findVisibleItem(_rootItem, QStringLiteral("settingsButton_General"));
    QVERIFY(generalButton);
    QTRY_VERIFY(generalButton->property("checked").toBool());
}

// Search must not match or offer navigation to sections that are hidden.
void TopLevelViewsTest::_testSettingsSearchExcludesHiddenSections()
{
    startUI();
    if (QTest::currentTestFailed())
        return;

    const auto restoreVideoSource = setVideoSourceWithRestore(VideoSettings::videoSourceRTSP);

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));

    QQuickItem* const searchField = findVisibleItem(_rootItem, QStringLiteral("settings_searchField"));
    QVERIFY(searchField);

    // "zero-copy" is a keyword of the Video page's hideable "Settings" section
    searchField->setProperty("text", QStringLiteral("zero-copy"));
    QQuickItem* videoButton = nullptr;
    QTRY_VERIFY((videoButton = findVisibleItem(_rootItem, QStringLiteral("settingsButton_Video"), 0)));
    QTRY_VERIFY(findVisibleSectionButton(videoButton->parentItem(), QStringLiteral("Settings")));

    setVideoSource(VideoSettings::videoDisabled);

    QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("settingsButton_Video"), 0));

    // The page vanished because its only search match is in the now-hidden section,
    // not because the page became unavailable: clearing the search brings it back
    searchField->setProperty("text", QString());
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsButton_Video"), 0));
}

void TopLevelViewsTest::_testDiscoveredCameraReceiverSettingsVisible()
{
#ifndef QGC_GST_STREAMING
    QSKIP("GStreamer receiver settings are unavailable in this build");
#else
    const auto restoreVideoSource = setVideoSourceWithRestore(VideoSettings::videoSourceRTSP);

    Fact* const lowLatencyMode = SettingsManager::instance()->videoSettings()->lowLatencyMode();
    const QVariant savedLowLatencyMode = lowLatencyMode->rawValue();
    const auto restoreLowLatencyMode =
        qScopeGuard([lowLatencyMode, savedLowLatencyMode] { lowLatencyMode->setRawValue(savedLowLatencyMode); });
    lowLatencyMode->setRawValue(false);

    startUI();
    if (QTest::currentTestFailed())
        return;

    QVERIFY(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewSettings")));
    if (!_clickSettingsButton(QStringLiteral("Video"))) {
        return;
    }

    QQuickItem* const videoPage = findVisibleItem(_rootItem, QStringLiteral("settingsPage_Video"));
    QVERIFY(videoPage);
    QVERIFY(videoPage->property("isStreamSource").toBool());

    // autoStreamConfig mirrors the read-only VideoManager property. Override the
    // page binding to isolate these UI visibility rules from camera transport.
    QVERIFY(videoPage->setProperty("autoStreamConfig", true));

    QQuickItem* const sourceGroup = findVisibleItem(_rootItem, QStringLiteral("settingsGroup_VideoSource"));
    QVERIFY(sourceGroup);
    QTRY_VERIFY(!sourceGroup->isEnabled());
    QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("settingsGroup_Connection"), 0));
    QTRY_VERIFY(!findVisibleItem(_rootItem, QStringLiteral("settingsTextField_aspectRatio"), 0));

    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsCheckBox_disableWhenDisarmed"), 0));
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsCheckBox_lowLatencyMode"), 0));
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsTextField_rtpJitterLatencyMs"), 0));
    QTRY_VERIFY(findVisibleItem(_rootItem, QStringLiteral("settingsCheckBox_rtspAutoReconnect"), 0));

    stopUI();
#endif
}
