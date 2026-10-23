#pragma once

#include "VehicleConfigUITestBase.h"

class Fact;

/// UI tests for the PX4 Failsafes setup page with a PX4 MockLink vehicle connected
class PX4FailsafesUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

private slots:
    void _testRebootChip();
    void _testBitmaskSlider();
    void _activeFilterHidesInactiveFailsafes();

private:
    /// Navigates from the Fly view to the Failsafes page of the Configure view
    void _navigateToFailsafes();

    /// Sets \a fact and waits for the vehicle to accept it. Run with the page open: it marks reboot-required settings
    /// itself instead of showing the reboot notice.
    void _setFactAndWait(Fact* fact, const QVariant& rawValue);
};
