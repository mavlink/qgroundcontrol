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

private:
    /// Navigates from the Fly view to the Failsafes page of the Configure view
    void _navigateToFailsafes();

    /// Sets \a fact, which needs a vehicle reboot, and dismisses the reboot-required notice
    void _setRebootRequiredFact(Fact* fact, const QVariant& rawValue);
};
