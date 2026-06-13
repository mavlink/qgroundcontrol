#pragma once

#include "QmlUITestBase.h"

class QQuickItem;
class Vehicle;

/// Base class for UI tests which exercise the vehicle Configure view.
///
/// Adds the navigation and parameter-reset helpers shared by the vehicle
/// config tests on top of QmlUITestBase. All helpers use QVERIFY internally —
/// check QTest::currentTestFailed() on return to propagate failures to the
/// calling test slot.
class VehicleConfigUITestBase : public QmlUITestBase
{
    Q_OBJECT

protected:
    /// Navigate from the Fly view to the Configure view and wait for
    /// vehicleConfig_root to appear.
    void navigateToConfigureView();

    /// Click the vehicle config sidebar button with \a objectName (component or
    /// section): scrolls it into view, clicks its centre and waits for the page
    /// switch. Returns the button item, or null after recording a test failure
    /// if it cannot be found.
    QQuickItem *clickSidebarButton(const QString &objectName);

    /// Reset the MockLink parameters to the firmware defaults from the parameter
    /// metadata and refresh them so the test starts from a known state.
    /// \a sentinelParamName must be a parameter that is non-default in
    /// PX4MockLink.params and defaults to 0 in the metadata: it acts as the
    /// sentinel for the refresh completing.
    void resetParamsToFirmwareDefaults(Vehicle *vehicle, const QString &sentinelParamName);

    /// Reset an APM MockLink to an uncalibrated state by sending
    /// MAV_CMD_PREFLIGHT_STORAGE param1=2 (which MockLink now zeros the
    /// calibration-indicator params for ArduPilot) and waiting for
    /// COMPASS_OFS_X to read back as 0 from the vehicle.
    void resetAPMParamsToUncalibrated(Vehicle *vehicle);

    /// Click through every vehicle component in the config sidebar and verify
    /// each one loads a panel. Hidden components (e.g. optional peripherals not
    /// present) are skipped silently. When non-empty, \a vehicleName is
    /// prepended to failure messages.
    void clickThroughAllComponents(Vehicle *vehicle, const QString &vehicleName = QString());
};
