#include "PlanViewLayerUITest.h"

#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtTest/QTest>

UT_REGISTER_TEST(PlanViewLayerUITest, TestLabel::Integration, TestLabel::MissionManager)

// Editing layer switching test. Offline for the entire test — layer state is
// purely a UI concern so no vehicle is needed.

namespace {
constexpr int kLayerMission = 1;
constexpr int kLayerFence   = 2;
constexpr int kLayerRally   = 3;

const char *kLayerSwitcher      = "planView_layerSwitcher";
const char *kSwitcherActive     = "layerSwitcher_activeButton";
const char *kMissionHeader      = "planTree_missionGroupHeader";
const char *kFenceHeader        = "planTree_fenceGroupHeader";
const char *kRallyHeader        = "planTree_rallyGroupHeader";
const char *kPlanFileHeader     = "planTree_planFileGroupHeader";
const char *kMissionItemEditor  = "planTree_missionItemEditor";
const char *kFenceEditor        = "planTree_fenceEditor";
const char *kRallyEditorHeader  = "planTree_rallyHeader";
const char *kFenceMapVisuals    = "planView_geoFenceMapVisuals";
const char *kRallyMapVisuals    = "planView_rallyPointMapVisuals";
} // namespace

void PlanViewLayerUITest::_clickMap(qreal fractionX, qreal fractionY)
{
    QVERIFY2(clickItemFraction(QStringLiteral("planView_map"), fractionX, fractionY), "Failed to click planView_map");
}

int PlanViewLayerUITest::_editingLayer()
{
    QQuickItem *planView = findVisibleItem(_rootItem, QStringLiteral("mainView_plan"));
    if (!planView) {
        return -1;
    }
    return planView->property("_editingLayer").toInt();
}

bool PlanViewLayerUITest::_clickTreeRow(const QString &objectName)
{
    return clickButtonScrolled(objectName, QStringLiteral("planView_planTree"));
}

void PlanViewLayerUITest::_verifyLayerState(const LayerUIState &state, const QString &context)
{
    QVERIFY2(waitForCondition([&] { return _editingLayer() == state.editingLayer; }, 2000,
                              QStringLiteral("editing layer")),
             qPrintable(QStringLiteral("%1: expected editing layer %2 but found %3")
                            .arg(context).arg(state.editingLayer).arg(_editingLayer())));

    if (!verifyVisibility(QLatin1String(kLayerSwitcher), state.switcherVisible, context)) return;
    if (!verifyVisibility(QLatin1String(kMissionItemEditor), state.missionEditorVisible, context)) return;
    if (!verifyVisibility(QLatin1String(kFenceEditor), state.fenceEditorVisible, context)) return;
    if (!verifyVisibility(QLatin1String(kRallyEditorHeader), state.rallyHeaderVisible, context)) return;

    // Map visuals interactive state follows the active layer
    if (!verifyProperty(QLatin1String(kFenceMapVisuals), "interactive", state.fenceInteractive, context)) return;
    if (!verifyProperty(QLatin1String(kRallyMapVisuals), "interactive", state.rallyInteractive, context)) return;
}

void PlanViewLayerUITest::_testEditingLayerSwitching()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    // Navigate to Plan view
    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewPlan")), "Failed to navigate to Plan view");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_plan"), 2000), "Plan view did not appear");
    QTest::qWait(_viewDelay);

    // Position the map at a known location so map clicks are deterministic
    QQuickItem *map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY2(map, "planView_map not found");
    const QGeoCoordinate mapCenter(47.397742, 8.545594); // Zurich
    map->setProperty("center", QVariant::fromValue(mapCenter));
    map->setProperty("zoomLevel", 15.0);
    QVERIFY2(waitForCondition(
                 [&] {
                     const QGeoCoordinate c = map->property("center").value<QGeoCoordinate>();
                     return c.isValid() && (c.distanceTo(mapCenter) < 1.0);
                 },
                 1000),
             "Map did not settle on the requested center");

    // ------------------------------------------------------------------
    // 2.1 Create-from-template mode: layer switcher hidden, layer group
    //     headers disabled, Plan Info header enabled
    // ------------------------------------------------------------------
    _verifyLayerState({
        .editingLayer         = kLayerMission,
        .switcherVisible      = false,
        .missionEditorVisible = false,  // Empty plan
        .fenceEditorVisible   = false,
        .rallyHeaderVisible   = false,
        .fenceInteractive     = false,
        .rallyInteractive     = false,
    }, QStringLiteral("2.1 create mode"));
    if (QTest::currentTestFailed()) return;

    // The expanded Plan Info section pushes the layer group headers below the
    // viewport where their virtualized delegates do not exist. Scroll them into
    // existence before checking their disabled state.
    QVERIFY2(findVisibleItemScrolled(QLatin1String(kMissionHeader), QStringLiteral("planView_planTree")),
             "Mission group header not found");
    verifyEnabled(QLatin1String(kMissionHeader), false, QStringLiteral("2.1 create mode"));
    verifyEnabled(QLatin1String(kFenceHeader),   false, QStringLiteral("2.1 create mode"));
    verifyEnabled(QLatin1String(kRallyHeader),   false, QStringLiteral("2.1 create mode"));
    QVERIFY2(findVisibleItemScrolled(QLatin1String(kPlanFileHeader), QStringLiteral("planView_planTree")),
             "Plan Info header not found");
    verifyEnabled(QLatin1String(kPlanFileHeader), true, QStringLiteral("2.1 create mode"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 2.2 Set home and insert takeoff: exits create mode, mission group
    //     auto-expands and Mission is the active layer
    // ------------------------------------------------------------------
    _clickMap(0.5, 0.5);
    if (QTest::currentTestFailed()) return;
    QVERIFY2(clickButton(QStringLiteral("planToolStrip_takeoffButton")), "Failed to click Takeoff button");
    QTest::qWait(_pageDelay);

    _verifyLayerState({
        .editingLayer         = kLayerMission,
        .switcherVisible      = true,
        .missionEditorVisible = true,   // Takeoff item editor, mission group auto-expanded
        .fenceEditorVisible   = false,
        .rallyHeaderVisible   = false,
        .fenceInteractive     = false,
        .rallyInteractive     = false,
    }, QStringLiteral("2.2 takeoff added"));
    if (QTest::currentTestFailed()) return;

    verifyEnabled(QLatin1String(kMissionHeader), true, QStringLiteral("2.2 takeoff added"));
    verifyEnabled(QLatin1String(kFenceHeader),   true, QStringLiteral("2.2 takeoff added"));
    verifyEnabled(QLatin1String(kRallyHeader),   true, QStringLiteral("2.2 takeoff added"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 2.3 Click GeoFence header: fence becomes the active layer, mission
    //     group collapses (accordion)
    // ------------------------------------------------------------------
    QVERIFY2(_clickTreeRow(QLatin1String(kFenceHeader)), "Failed to click GeoFence header");

    _verifyLayerState({
        .editingLayer         = kLayerFence,
        .switcherVisible      = true,
        .missionEditorVisible = false,  // Accordion collapsed mission group
        .fenceEditorVisible   = true,
        .rallyHeaderVisible   = false,
        .fenceInteractive     = true,
        .rallyInteractive     = false,
    }, QStringLiteral("2.3 fence layer"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 2.4 Click GeoFence header again: layer group headers do not collapse,
    //     state unchanged
    // ------------------------------------------------------------------
    QVERIFY2(_clickTreeRow(QLatin1String(kFenceHeader)), "Failed to click GeoFence header (second)");

    _verifyLayerState({
        .editingLayer         = kLayerFence,
        .switcherVisible      = true,
        .missionEditorVisible = false,
        .fenceEditorVisible   = true,   // Still expanded
        .rallyHeaderVisible   = false,
        .fenceInteractive     = true,
        .rallyInteractive     = false,
    }, QStringLiteral("2.4 fence header re-click"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 2.5 Click Rally header: rally becomes the active layer, fence group
    //     collapses
    // ------------------------------------------------------------------
    QVERIFY2(_clickTreeRow(QLatin1String(kRallyHeader)), "Failed to click Rally header");

    _verifyLayerState({
        .editingLayer         = kLayerRally,
        .switcherVisible      = true,
        .missionEditorVisible = false,
        .fenceEditorVisible   = false,  // Accordion collapsed fence group
        .rallyHeaderVisible   = true,
        .fenceInteractive     = false,
        .rallyInteractive     = true,
    }, QStringLiteral("2.5 rally layer"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 2.6 Layer switcher: expand choices and pick Mission
    // ------------------------------------------------------------------
    QVERIFY2(clickButton(QLatin1String(kSwitcherActive)), "Failed to click layer switcher active button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("layerSwitcher_choice_missionGroup"), 2000),
             "Mission choice button did not appear");
    QVERIFY2(clickButton(QStringLiteral("layerSwitcher_choice_missionGroup")), "Failed to click Mission choice button");

    _verifyLayerState({
        .editingLayer         = kLayerMission,
        .switcherVisible      = true,
        .missionEditorVisible = true,   // selectLayer expanded mission group
        .fenceEditorVisible   = false,
        .rallyHeaderVisible   = false,  // Accordion collapsed rally group
        .fenceInteractive     = false,
        .rallyInteractive     = false,
    }, QStringLiteral("2.6 switcher to mission"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 2.7 Plan Info header: toggles expansion without affecting the layer
    // ------------------------------------------------------------------
    QVERIFY2(_clickTreeRow(QLatin1String(kPlanFileHeader)), "Failed to click Plan Info header");
    QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, QStringLiteral("planTree_planFileInfo"), 0) != nullptr; },
                              2000, QStringLiteral("plan info expanded")),
             "2.7: Plan Info section did not expand");

    _verifyLayerState({
        .editingLayer         = kLayerMission,  // Unchanged
        .switcherVisible      = true,
        .missionEditorVisible = true,           // Unchanged: Plan Info is not a layer group
        .fenceEditorVisible   = false,
        .rallyHeaderVisible   = false,
        .fenceInteractive     = false,
        .rallyInteractive     = false,
    }, QStringLiteral("2.7 plan info toggle"));
    if (QTest::currentTestFailed()) return;

    QVERIFY2(_clickTreeRow(QLatin1String(kPlanFileHeader)), "Failed to click Plan Info header (collapse)");
    QVERIFY2(waitForCondition([&] { return findVisibleItem(_rootItem, QStringLiteral("planTree_planFileInfo"), 0) == nullptr; },
                              2000, QStringLiteral("plan info collapsed")),
             "2.7: Plan Info section did not collapse");

    stopUI();
}
