#include "AppCloseWarningUITest.h"

#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "MissionController.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "PlanMasterController.h"
#include "Vehicle.h"

UT_REGISTER_TEST(AppCloseWarningUITest, TestLabel::Integration, TestLabel::MissionManager)

bool AppCloseWarningUITest::_forcePlanViewMissionDirty()
{
    QQuickItem *planView = _window ? _window->findChild<QQuickItem *>(QStringLiteral("mainView_plan")) : nullptr;
    if (!planView) {
        QTest::qFail("Could not find Plan view item (mainView_plan)", __FILE__, __LINE__);
        return false;
    }

    auto *masterController = qvariant_cast<PlanMasterController *>(planView->property("_planMasterController"));
    if (!masterController) {
        QTest::qFail("Plan view _planMasterController property is not a PlanMasterController", __FILE__, __LINE__);
        return false;
    }

    // Marking the mission controller dirty propagates to the master controller's
    // dirtyForSave/dirtyForUpload, which is exactly what the unsaved-mission close
    // check reads. The plan need not contain real items for the check to fire.
    masterController->missionController()->setDirty(true);
    return true;
}

void AppCloseWarningUITest::_testCloseWarningMatrix_data()
{
    // Each row sets up some combination of the three close-warning conditions and
    // chooses where to reject. The three dialogs always appear in the fixed order
    // Unsaved Mission -> Pending Parameter Updates -> Active Vehicle Connections.
    //
    //   mission       (M): force the Plan view mission dirty-for-save.
    //   pendingWrites (P): force a vehicle parameter pending-write (requires a
    //                      connection, since pending writes belong to a vehicle).
    //   connection    (C): keep a MockLink connected.
    //   rejectAtStep     : 1-based index into the *shown* dialog list at which to
    //                      press No (cancel close). 0 means accept every dialog,
    //                      which must close the app.
    //
    // Rows with P set but C clear are impossible (pending writes cannot outlive
    // their vehicle) and are therefore omitted.
    QTest::addColumn<bool>("mission");
    QTest::addColumn<bool>("pendingWrites");
    QTest::addColumn<bool>("connection");
    QTest::addColumn<int>("rejectAtStep");

    // No conditions: closing must proceed immediately with no warning dialog.
    QTest::newRow("none-acceptAll")            << false << false << false << 0;

    // Connection only.
    QTest::newRow("C-acceptAll")               << false << false << true  << 0;
    QTest::newRow("C-rejectConnection")        << false << false << true  << 1;

    // Pending writes + connection.
    QTest::newRow("PC-acceptAll")              << false << true  << true  << 0;
    QTest::newRow("PC-rejectPending")          << false << true  << true  << 1;
    QTest::newRow("PC-rejectConnection")       << false << true  << true  << 2;

    // Mission only (offline, no connection).
    QTest::newRow("M-acceptAll")               << true  << false << false << 0;
    QTest::newRow("M-rejectMission")           << true  << false << false << 1;

    // Mission + connection.
    QTest::newRow("MC-acceptAll")              << true  << false << true  << 0;
    QTest::newRow("MC-rejectMission")          << true  << false << true  << 1;
    QTest::newRow("MC-rejectConnection")       << true  << false << true  << 2;

    // Mission + pending writes + connection (all three dialogs).
    QTest::newRow("MPC-acceptAll")             << true  << true  << true  << 0;
    QTest::newRow("MPC-rejectMission")         << true  << true  << true  << 1;
    QTest::newRow("MPC-rejectPending")         << true  << true  << true  << 2;
    QTest::newRow("MPC-rejectConnection")      << true  << true  << true  << 3;
}

void AppCloseWarningUITest::_testCloseWarningMatrix()
{
    QFETCH(bool, mission);
    QFETCH(bool, pendingWrites);
    QFETCH(bool, connection);
    QFETCH(int, rejectAtStep);

    // Pending writes belong to a vehicle, so they can only exist alongside a
    // connection. The data set never produces this combination.
    QVERIFY2(!pendingWrites || connection, "Invalid matrix row: pending writes without a connection");

    // Incidental one-time startup message when the map cache DB is upgraded.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Offline Map Cache database has been upgraded")));

    startUI();
    if (QTest::currentTestFailed()) {
        return;
    }

    QPointer<MockLink> mockLink;
    Vehicle *vehicle = nullptr;
    bool appClosed = false;

    // Teardown: on reject paths the window is still open and any MockLink is still
    // connected, so disconnect before tearing down (matching runWithMockLink). On
    // accept-all paths finishCloseProcess() has already shut the links down and
    // closed the window, so only the engine needs destroying.
    const auto guard = qScopeGuard([&] {
        if (!appClosed) {
            disconnectMockLink(mockLink);
        }
        closeUIWindow();
        destroyUIEngine();
    });

    if (connection) {
        mockLink = connectMockLinkAndWaitReady(
            [] { return MockLink::startPX4MockLink(false /* sendStatusText */, false /* enableCamera */, false /* enableGimbal */); },
            vehicle);
        if (!mockLink) {
            return;
        }
    }

    if (mission) {
        if (!_forcePlanViewMissionDirty()) {
            return;
        }
    }

    if (pendingWrites) {
        QVERIFY2(vehicle, "Pending writes requested but no vehicle is connected");
        vehicle->parameterManager()->setPendingWritesForTest(true);
    }

    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewClose")),
             "Failed to click Close button in tool select dropdown");

    // The dialogs that should appear, in their fixed presentation order.
    QStringList expectedDialogs;
    if (mission)       expectedDialogs << QStringLiteral("Unsaved Mission");
    if (pendingWrites) expectedDialogs << QStringLiteral("Pending Parameter Updates");
    if (connection)    expectedDialogs << QStringLiteral("Active Vehicle Connections");

    for (int step = 0; step < expectedDialogs.size(); ++step) {
        const QString &title = expectedDialogs.at(step);
        QVERIFY2(waitForDialog(title),
                 qPrintable(QStringLiteral("Expected close-warning dialog not shown: %1").arg(title)));

        const bool rejectHere = (rejectAtStep != 0) && (step == rejectAtStep - 1);
        if (rejectHere) {
            QVERIFY2(rejectDialog(),
                     qPrintable(QStringLiteral("Failed to reject dialog: %1").arg(title)));

            // Rejecting cancels the close: the window must stay open and no later
            // dialog in the sequence may appear.
            QVERIFY2(_window && _window->isVisible(),
                     qPrintable(QStringLiteral("Window closed after rejecting dialog: %1").arg(title)));
            if (step + 1 < expectedDialogs.size()) {
                QVERIFY2(!dialogVisible(expectedDialogs.at(step + 1)),
                         qPrintable(QStringLiteral("Later dialog shown after a reject: %1").arg(expectedDialogs.at(step + 1))));
            }
            return;
        }

        QVERIFY2(acceptDialog(),
                 qPrintable(QStringLiteral("Failed to accept dialog: %1").arg(title)));
    }

    // Every shown dialog (if any) was accepted, so the app must finish closing.
    QVERIFY2(QTest::qWaitFor([this] { return _window && !_window->isVisible(); }, 3000),
             "App did not close after accepting all close-warning dialogs");
    appClosed = true;
}

void AppCloseWarningUITest::_testNoUnsavedMissionWarningForDownloadedMission()
{
    // Incidental one-time startup message when the map cache DB is upgraded.
    ignoreLogMessage("API.QGCApplication.AppMessage", QtDebugMsg,
                     QRegularExpression(QStringLiteral("Offline Map Cache database has been upgraded")));

    // connectMockLinkAndWaitReady (used by runWithMockLink) waits for the
    // vehicle's initialConnectComplete signal. The InitialConnectStateMachine
    // requests the mission as one of its states, so by the time the body runs
    // the mission has been downloaded from the vehicle and the Plan view's
    // PlanMasterController has loaded it.
    runWithMockLink(
        [] { return MockLink::startPX4MockLinkWithMission(false /* sendStatusText */, false /* enableCamera */, false /* enableGimbal */); },
        [this](QPointer<MockLink> mockLink, Vehicle *vehicle) {
            Q_UNUSED(mockLink);
            Q_UNUSED(vehicle);

            // runWithMockLink waits for initialConnectComplete. The
            // InitialConnectStateMachine blocks on completion of the mission,
            // geofence, and rally point loads before that signal fires, so the
            // plan is fully downloaded by the time this body runs.
            QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewClose")),
                     "Failed to click Close button in tool select dropdown");

            // The active-connection check runs only after the unsaved-mission check
            // passes. Waiting for its dialog to appear proves the mission check did
            // not block — i.e. the downloaded plan was not treated as dirty.
            QVERIFY2(waitForDialog(QStringLiteral("Active Vehicle Connections")),
                     "Active vehicle connection warning dialog was not shown on close");

            // A plan that was just downloaded from the vehicle reflects exactly what is
            // on the vehicle. The user has made no edits, so the unsaved-mission warning
            // must NOT appear. Otherwise closing the app would wrongly warn about a
            // "mission edit in progress" even though nothing was edited.
            QVERIFY2(!dialogVisible(QStringLiteral("Unsaved Mission")),
                     "Unsaved mission warning shown for a freshly downloaded, unedited plan");
        });
}
