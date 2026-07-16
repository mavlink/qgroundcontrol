#include "PlanViewUITest.h"

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtPositioning/QGeoCoordinate>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "QGCFileDialogController.h"

UT_REGISTER_TEST(PlanViewUITest, TestLabel::Integration, TestLabel::MissionManager)

// Offline Plan view lifecycle test. No vehicle is connected for the entire
// test, so Upload must remain disabled and un-highlighted throughout.

void PlanViewUITest::_clickMap(qreal fractionX, qreal fractionY)
{
    QVERIFY2(clickItemFraction(QStringLiteral("planView_map"), fractionX, fractionY), "Failed to click planView_map");
}

int PlanViewUITest::_missionItemCount()
{
    QQuickItem *planView = findVisibleItem(_rootItem, QStringLiteral("mainView_plan"));
    if (!planView) {
        return -1;
    }
    QObject *visualItems = planView->property("_visualItems").value<QObject*>();
    if (!visualItems) {
        return -1;
    }
    return visualItems->property("count").toInt();
}

void PlanViewUITest::_verifyFullState(const PlanUIState &state, const QString &context)
{
    const QString takeoffBtn   = QStringLiteral("planToolStrip_takeoffButton");
    const QString waypointBtn  = QStringLiteral("planToolStrip_waypointButton");
    const QString roiBtn       = QStringLiteral("planToolStrip_roiButton");
    const QString patternBtn   = QStringLiteral("planToolStrip_patternButton");
    const QString landBtn      = QStringLiteral("planToolStrip_landButton");
    const QString saveBtn      = QStringLiteral("planToolbar_saveButton");
    const QString uploadBtn    = QStringLiteral("planToolbar_uploadButton");
    const QString openBtn      = QStringLiteral("planToolbar_openButton");
    const QString clearBtn     = QStringLiteral("planToolbar_clearButton");
    const QString templatesCol = QStringLiteral("planInfo_templatesColumn");

    // Templates section
    if (state.templatesVisible) {
        QVERIFY2(findVisibleItem(_rootItem, templatesCol, 2000),
                 qPrintable(context + ": templates section not visible"));
        verifyEnabled(templatesCol, state.templatesEnabled, context);
    } else {
        QVERIFY2(waitForCondition(
                     [&] { return findVisibleItem(_rootItem, templatesCol, 0) == nullptr; },
                     2000, QStringLiteral("templates section hidden")),
                 qPrintable(context + ": templates section unexpectedly visible"));
    }

    // Toolstrip
    verifyEnabled(takeoffBtn,  state.takeoffEnabled,  context);
    verifyEnabled(waypointBtn, state.waypointEnabled, context);
    verifyChecked(waypointBtn, state.waypointChecked, context);
    verifyEnabled(patternBtn,  state.patternEnabled,  context);
    verifyEnabled(landBtn,     state.landEnabled,     context);
    verifyText(landBtn,        state.landText,        context);
    if (findVisibleItem(_rootItem, roiBtn, 0)) {
        verifyEnabled(roiBtn, state.roiEnabled, context);
        verifyChecked(roiBtn, state.roiChecked, context);
        verifyText(roiBtn, state.roiCancelText ? QStringLiteral("Cancel ROI") : QStringLiteral("ROI"), context);
    }

    // Toolbar
    verifyEnabled(saveBtn, state.saveEnabled, context);
    verifyPrimary(saveBtn, state.savePrimary, context);
    verifyEnabled(openBtn,  true, context);
    verifyEnabled(clearBtn, true, context);

    // Offline invariant: Upload must never be enabled or highlighted
    verifyEnabled(uploadBtn, false, context);
    verifyPrimary(uploadBtn, false, context);

    // Visual item count
    QVERIFY2(waitForCondition([&] { return _missionItemCount() == state.itemCount; }, 2000,
                              QStringLiteral("visual item count")),
             qPrintable(QStringLiteral("%1: expected %2 visual items but found %3")
                            .arg(context).arg(state.itemCount).arg(_missionItemCount())));
}

void PlanViewUITest::_testPlanViewStates()
{
    startUI();
    if (QTest::currentTestFailed()) return;

    // Navigate to Plan view
    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewPlan")), "Failed to navigate to Plan view");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("mainView_plan"), 2000), "Plan view did not appear");
    QTest::qWait(_viewDelay);

    // Position the map at a known location and reasonable zoom so map clicks
    // are deterministic and offset clicks produce nearby waypoints.
    QQuickItem *map = findVisibleItem(_rootItem, QStringLiteral("planView_map"));
    QVERIFY2(map, "planView_map not found");
    const QGeoCoordinate mapCenter(47.397742, 8.545594); // Zurich
    map->setProperty("center", QVariant::fromValue(mapCenter));
    map->setProperty("zoomLevel", 15.0);
    QVERIFY2(waitForCondition(
                 [&] {
                     const QGeoCoordinate c = map->property("center").value<QGeoCoordinate>();
                     return c.isValid() && (c.distanceTo(mapCenter) < 1.0)
                         && qFuzzyCompare(map->property("zoomLevel").toReal(), 15.0);
                 },
                 1000),
             "Map did not settle on the requested center/zoom");

    const QString takeoffBtn  = QStringLiteral("planToolStrip_takeoffButton");
    const QString waypointBtn = QStringLiteral("planToolStrip_waypointButton");
    const QString saveBtn     = QStringLiteral("planToolbar_saveButton");
    const QString clearBtn    = QStringLiteral("planToolbar_clearButton");

    // ------------------------------------------------------------------
    // 1.1 Initial state: no home position, empty plan
    // ------------------------------------------------------------------
    _verifyFullState({
        .templatesVisible = true,
        .templatesEnabled = false,  // No home position
        .takeoffEnabled   = false,
        .waypointEnabled  = false,
        .waypointChecked  = false,
        .patternEnabled   = false,
        .roiEnabled       = false,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = false,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = false,
        .savePrimary      = false,
        .itemCount        = 1,      // Mission settings item only
    }, QStringLiteral("1.1 initial"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 1.2 Click map center to set home position
    // ------------------------------------------------------------------
    _clickMap(0.5, 0.5);
    if (QTest::currentTestFailed()) return;

    _verifyFullState({
        .templatesVisible = true,
        .templatesEnabled = true,   // Home position now set
        .takeoffEnabled   = true,   // Plan empty: takeoff insertable
        .waypointEnabled  = false,  // Takeoff required first
        .waypointChecked  = false,
        .patternEnabled   = false,
        .roiEnabled       = false,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = false,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = false,  // Still no items
        .savePrimary      = false,
        .itemCount        = 1,      // Setting home adds no item
    }, QStringLiteral("1.2 home set"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 1.3 Insert takeoff item
    // ------------------------------------------------------------------
    QVERIFY2(clickButton(takeoffBtn), "Failed to click Takeoff button");
    QTest::qWait(_pageDelay);

    _verifyFullState({
        .templatesVisible = false,
        .templatesEnabled = false,
        .takeoffEnabled   = false,  // Plan no longer empty
        .waypointEnabled  = true,
        .waypointChecked  = false,
        .patternEnabled   = true,
        .roiEnabled       = true,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = true,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = true,
        .savePrimary      = true,   // Unsaved changes
        .itemCount        = 2,      // Settings + takeoff
    }, QStringLiteral("1.3 takeoff"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 1.4 Insert waypoint at an offset from center
    // ------------------------------------------------------------------
    QVERIFY2(clickButton(waypointBtn), "Failed to click Waypoint button");
    _clickMap(0.35, 0.4);
    if (QTest::currentTestFailed()) return;
    QTest::qWait(_pageDelay);

    _verifyFullState({
        .templatesVisible = false,
        .templatesEnabled = false,
        .takeoffEnabled   = false,
        .waypointEnabled  = true,
        .waypointChecked  = true,   // Add-waypoint mode stays active after insert
        .patternEnabled   = true,
        .roiEnabled       = true,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = true,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = true,
        .savePrimary      = true,
        .itemCount        = 3,      // + waypoint
    }, QStringLiteral("1.4 waypoint"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 1.5 Save to disk via file dialog shim. First arm a rejection (user
    //     cancels the dialog): nothing is written, plan stays dirty.
    // ------------------------------------------------------------------
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString planFile = tempDir.filePath(QStringLiteral("PlanViewUITest.plan"));

    QGCFileDialogController::setTestRejectNext();
    QVERIFY2(clickButton(saveBtn), "Failed to click Save button (cancelled save)");

    QVERIFY2(waitForCondition([] { return !QGCFileDialogController::testHookArmed(); }, 5000,
                              QStringLiteral("file dialog shim consumed")),
             "1.5 cancelled save: file dialog shim was not consumed");

    _verifyFullState({
        .templatesVisible = false,
        .templatesEnabled = false,
        .takeoffEnabled   = false,
        .waypointEnabled  = true,
        .waypointChecked  = false,  // Toolbar click cancels add-waypoint mode
        .patternEnabled   = true,
        .roiEnabled       = true,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = true,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = true,
        .savePrimary      = true,   // Cancelled save: still dirty
        .itemCount        = 3,      // Cancelled save changes nothing
    }, QStringLiteral("1.5 cancelled save"));
    if (QTest::currentTestFailed()) return;
    QVERIFY2(!QFile::exists(planFile), "1.5 cancelled save: plan file was unexpectedly written");

    // Now the accepted save
    QGCFileDialogController::setTestNextFileForAccept(planFile);
    QVERIFY2(clickButton(saveBtn), "Failed to click Save button");

    QVERIFY2(waitForCondition([&] { return QFile::exists(planFile); }, 5000,
                              QStringLiteral("plan file written")),
             "1.5 save: plan file was not written");
    QVERIFY2(!QGCFileDialogController::testHookArmed(), "1.5 save: file dialog shim was not consumed");

    _verifyFullState({
        .templatesVisible = false,
        .templatesEnabled = false,
        .takeoffEnabled   = false,
        .waypointEnabled  = true,
        .waypointChecked  = false,  // Toolbar click cancels add-waypoint mode
        .patternEnabled   = true,
        .roiEnabled       = true,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = true,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = true,
        .savePrimary      = false,  // dirtyForSave cleared
        .itemCount        = 3,      // Save changes nothing
    }, QStringLiteral("1.5 save"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 1.6 Modify plan again: Save highlight returns
    // ------------------------------------------------------------------
    QVERIFY2(clickButton(waypointBtn), "Failed to click Waypoint button (second)");
    _clickMap(0.65, 0.6);
    if (QTest::currentTestFailed()) return;
    QTest::qWait(_pageDelay);

    _verifyFullState({
        .templatesVisible = false,
        .templatesEnabled = false,
        .takeoffEnabled   = false,
        .waypointEnabled  = true,
        .waypointChecked  = true,   // Add-waypoint mode active again
        .patternEnabled   = true,
        .roiEnabled       = true,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = true,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = true,
        .savePrimary      = true,   // Save highlight returns
        .itemCount        = 4,      // + second waypoint
    }, QStringLiteral("1.6 modify"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 1.7 Toolstrip checked state is mutually exclusive: clicking ROI
    //     while in add-waypoint mode moves the checkmark to ROI
    // ------------------------------------------------------------------
    const QString roiBtn = QStringLiteral("planToolStrip_roiButton");
    const bool roiSupported = findVisibleItem(_rootItem, roiBtn, 0) != nullptr;
    if (roiSupported) {
        QVERIFY2(clickButton(roiBtn), "Failed to click ROI button");

        _verifyFullState({
            .templatesVisible = false,
            .templatesEnabled = false,
            .takeoffEnabled   = false,
            .waypointEnabled  = true,
            .waypointChecked  = false,  // Checkmark moved from waypoint to ROI
            .patternEnabled   = true,
            .roiEnabled       = true,
            .roiChecked       = true,
            .roiCancelText    = false,
            .landEnabled      = true,
            .landText         = QStringLiteral("Return"),
            .saveEnabled      = true,
            .savePrimary      = true,
            .itemCount        = 4,      // Entering ROI mode adds no item
        }, QStringLiteral("1.7 roi mode"));
        if (QTest::currentTestFailed()) return;

        // ROI is one-shot: adding the ROI clears the mode (unchecked) and
        // the button relabels to "Cancel ROI" while an ROI is active.
        _clickMap(0.6, 0.35);
        if (QTest::currentTestFailed()) return;
        QTest::qWait(_pageDelay);

        _verifyFullState({
            .templatesVisible = false,
            .templatesEnabled = false,
            .takeoffEnabled   = false,
            .waypointEnabled  = true,
            .waypointChecked  = false,
            .patternEnabled   = true,
            .roiEnabled       = true,
            .roiChecked       = false,  // One-shot mode cleared
            .roiCancelText    = true,   // ROI active in plan
            .landEnabled      = true,
            .landText         = QStringLiteral("Return"),
            .saveEnabled      = true,
            .savePrimary      = true,
            .itemCount        = 5,      // + ROI
        }, QStringLiteral("1.7 roi added"));
        if (QTest::currentTestFailed()) return;
    }

    // ------------------------------------------------------------------
    // 1.8 Insert land/return item: land no longer insertable and no fly
    //     through commands are allowed after it
    // ------------------------------------------------------------------
    const QString landBtn = QStringLiteral("planToolStrip_landButton");
    QVERIFY2(clickButton(landBtn), "Failed to click Land button");
    QTest::qWait(_pageDelay);

    _verifyFullState({
        .templatesVisible = false,
        .templatesEnabled = false,
        .takeoffEnabled   = false,
        .waypointEnabled  = false,  // No fly through commands after land item
        .waypointChecked  = false,
        .patternEnabled   = false,  // No fly through commands after land item
        .roiEnabled       = true,
        .roiChecked       = false,
        .roiCancelText    = true,   // ROI from 1.7 still active
        .landEnabled      = false,  // Only one land item allowed
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = true,
        .savePrimary      = true,
        .itemCount        = roiSupported ? 6 : 5,   // + land/return item
    }, QStringLiteral("1.8 land added"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 1.9 Clear plan: full round trip back to initial state
    // ------------------------------------------------------------------
    QVERIFY2(clickButton(clearBtn), "Failed to click Clear button");
    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("popupDialog_acceptButton"), 2000),
             "1.9 clear: confirmation dialog did not appear");
    QVERIFY2(clickButton(QStringLiteral("popupDialog_acceptButton")), "Failed to confirm Clear");
    QTest::qWait(_pageDelay);

    // Clear removes the plan items and the home position: back to initial state
    _verifyFullState({
        .templatesVisible = true,
        .templatesEnabled = false,  // Home position cleared
        .takeoffEnabled   = false,
        .waypointEnabled  = false,
        .waypointChecked  = false,
        .patternEnabled   = false,
        .roiEnabled       = false,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = false,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = false,
        .savePrimary      = false,
        .itemCount        = 1,      // Back to settings item only
    }, QStringLiteral("1.9 after clear"));
    if (QTest::currentTestFailed()) return;

    // ------------------------------------------------------------------
    // 1.10 Open the plan saved in 1.5: save -> clear -> open round trip.
    //      First arm a rejection (user cancels the dialog): nothing loads.
    // ------------------------------------------------------------------
    QGCFileDialogController::setTestRejectNext();
    QVERIFY2(clickButton(QStringLiteral("planToolbar_openButton")), "Failed to click Open button (cancelled open)");

    QVERIFY2(waitForCondition([] { return !QGCFileDialogController::testHookArmed(); }, 5000,
                              QStringLiteral("file dialog shim consumed")),
             "1.10 cancelled open: file dialog shim was not consumed");

    // Cancelled open: still in the post-clear initial state
    _verifyFullState({
        .templatesVisible = true,
        .templatesEnabled = false,
        .takeoffEnabled   = false,
        .waypointEnabled  = false,
        .waypointChecked  = false,
        .patternEnabled   = false,
        .roiEnabled       = false,
        .roiChecked       = false,
        .roiCancelText    = false,
        .landEnabled      = false,
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = false,
        .savePrimary      = false,
        .itemCount        = 1,      // Cancelled open loads nothing
    }, QStringLiteral("1.10 cancelled open"));
    if (QTest::currentTestFailed()) return;

    // Now the accepted open
    QGCFileDialogController::setTestNextFileForAccept(planFile);
    QVERIFY2(clickButton(QStringLiteral("planToolbar_openButton")), "Failed to click Open button");

    QVERIFY2(waitForCondition([] { return !QGCFileDialogController::testHookArmed(); }, 5000,
                              QStringLiteral("file dialog shim consumed")),
             "1.10 open: file dialog shim was not consumed");
    QTest::qWait(_pageDelay);

    // Loaded plan is the 1.5 snapshot: takeoff + one waypoint, home position
    // restored from the file, not dirty for save. After load the current item
    // is mission settings (seq 0), so fly-through/land inserts are disabled
    // because they would be placed before the takeoff item.
    _verifyFullState({
        .templatesVisible = false,
        .templatesEnabled = false,
        .takeoffEnabled   = false,  // Takeoff already in plan
        .waypointEnabled  = false,  // Insert would precede takeoff
        .waypointChecked  = false,
        .patternEnabled   = false,  // Insert would precede takeoff
        .roiEnabled       = true,
        .roiChecked       = false,
        .roiCancelText    = false,  // Saved plan predates the ROI
        .landEnabled      = false,  // Insert would precede takeoff
        .landText         = QStringLiteral("Return"),
        .saveEnabled      = true,
        .savePrimary      = false,  // Freshly loaded plan is not dirty
        .itemCount        = 3,      // Settings + takeoff + waypoint
    }, QStringLiteral("1.10 open round trip"));

    stopUI();
}
