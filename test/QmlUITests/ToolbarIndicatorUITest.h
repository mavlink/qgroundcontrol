#pragma once

#include "QmlUITestBase.h"

#include <functional>

class MockLink;

/// UI smoke test that boots the full QML UI, connects each MockLink vehicle
/// type, and exercises every toolbar indicator: opens the drop-down drawer,
/// expands it when supported, then closes it by pressing Escape.
///
/// Covered vehicles:
///   - PX4 generic (PX4MockLink)
///   - ArduCopter (APM MockLink)
class ToolbarIndicatorUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    ToolbarIndicatorUITest() = default;

private slots:
    void _testPX4Indicators();
    void _testAPMCopterIndicators();
    void _testEmergencyStopReplacesDisarmInFlight_data();
    void _testEmergencyStopReplacesDisarmInFlight();
    void _testArmRequiresEnforcedChecklist();
    void _testDisarmReachableWithoutParameters();
    void _testRebootRequiredIndicator_data();
    void _testRebootRequiredIndicator();
    void _testIndicatorDrawerClosesOnVehicleDisconnect();
    void _testVehicleMessagesMarkReadAndClear();
    void _testMockLinkBootStatusTextShowsNonCriticalIndicator();
    void _testHealthChecksDriveToolbarSeverity();
    void _testChecklistProgressPersistsAcrossDropdownCloseAndReconnect();
    void _testStockChecklistLocksLaterSections();

private:
    /// Shared implementation: connect a MockLink via \a factory and cycle
    /// through every visible toolbar indicator.
    /// \a vehicleName is used only in QVERIFY2 failure messages.
    void _runIndicatorTest(const std::function<MockLink *()> &factory,
                           const QString &vehicleName);

    /// Open the drawer for \a indicatorItem, verify it opens, expand it when
    /// \a expectExpand is true (failing if the button is absent), then close
    /// it with Escape.  Returns true on success.
    bool _exerciseIndicator(QQuickItem *indicatorItem, const QString &indicatorName, bool expectExpand);

    /// Starts the UI and connects a PX4 MockLink for the checklist tests. Returns false if setup failed.
    bool _startChecklistUI(QPointer<MockLink>& mockLink, Vehicle*& vehicle);

    /// Tears down what _startChecklistUI set up.
    void _stopChecklistUI(QPointer<MockLink>& mockLink);
};
